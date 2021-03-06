/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 * 
 * -=- Robust Distributed System Nucleus (rDSN) -=- 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Description:
 *     What is this file about?
 *
 * Revision history:
 *     xxxx-xx-xx, author, first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

#include "meta_service.h"
#include "server_state.h"
#include "load_balancer.h"
#include "meta_server_failure_detector.h"
#include <sys/stat.h>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "meta.service"

meta_service::meta_service(server_state* state)
: _state(state), serverlet("meta_service")
{
    _balancer = nullptr;
    _failure_detector = nullptr;
    _started = false;

    _opts.initialize();
}

meta_service::~meta_service(void)
{
}

void meta_service::start()
{
    dassert(!_started, "meta service is already started");

    _balancer = new load_balancer(_state);            
    _failure_detector = new meta_server_failure_detector(_state, this);    
    
    // become leader
    while (!_failure_detector->acquire_leader_lock()) {}
    dassert(_failure_detector->is_primary(), "must be primary at this point");

    // sync meta state
    while (_state->on_become_leader() != ERR_OK) {}

    // make sure the delay is larger than fd.grace to ensure 
    // all machines are in the correct state (assuming connected initially)
    tasking::enqueue(LPC_LBM_START, this, &meta_service::on_load_balance_start, 0, 
        _opts.fd_grace_seconds * 1000);

    auto err = _failure_detector->start(
        _opts.fd_check_interval_seconds,
        _opts.fd_beacon_interval_seconds,
        _opts.fd_lease_seconds,
        _opts.fd_grace_seconds,
        false
        );

    dassert(err == ERR_OK, "FD start failed, err = %s", err.to_string());

    register_rpc_handler(
        RPC_CM_QUERY_NODE_PARTITIONS,
        "RPC_CM_QUERY_NODE_PARTITIONS",
        &meta_service::on_query_configuration_by_node
        );

    register_rpc_handler(
        RPC_CM_QUERY_PARTITION_CONFIG_BY_INDEX,
        "RPC_CM_QUERY_PARTITION_CONFIG_BY_INDEX",
        &meta_service::on_query_configuration_by_index
        );

    register_rpc_handler(
        RPC_CM_UPDATE_PARTITION_CONFIGURATION,
        "RPC_CM_UPDATE_PARTITION_CONFIGURATION",
        &meta_service::on_update_configuration
        );

    register_rpc_handler(
        RPC_CM_MODIFY_REPLICA_CONFIG_COMMAND,
        "RPC_CM_MODIFY_REPLICA_CONFIG_COMMAND",
        &meta_service::on_modify_replica_config_explictly
        );
}

bool meta_service::stop()
{
    if (!_started || _balancer_timer == nullptr) return false;

    _started = false;
    _failure_detector->stop();
    delete _failure_detector;
    _failure_detector = nullptr;

    if (_balancer_timer == nullptr)
    {
        _balancer_timer->cancel(true);
    }
    
    unregister_rpc_handler(RPC_CM_QUERY_NODE_PARTITIONS);
    unregister_rpc_handler(RPC_CM_QUERY_PARTITION_CONFIG_BY_INDEX);
    unregister_rpc_handler(RPC_CM_UPDATE_PARTITION_CONFIGURATION);
    unregister_rpc_handler(RPC_CM_MODIFY_REPLICA_CONFIG_COMMAND);

    delete _balancer;
    _balancer = nullptr;
    return true;
}

void meta_service::on_load_balance_start()
{
    dassert(_balancer_timer == nullptr, "");

    _state->unfree_if_possible_on_start();
    _balancer_timer = tasking::enqueue(LPC_LBM_RUN, this, &meta_service::on_load_balance_timer, 
        0,
        1,
        10000
        );

    _started = true;
}

// partition server & client => meta server
void meta_service::on_query_configuration_by_node(dsn_message_t msg)
{
    if (!_started)
    {
        configuration_query_by_node_response response;
        response.err = ERR_SERVICE_NOT_ACTIVE;
        reply(msg, response);
        return;
    }

    if (!_failure_detector->is_primary())
    {
        dsn_rpc_forward(msg, _failure_detector->get_primary().c_addr());
        return;
    }

    configuration_query_by_node_response response;
    configuration_query_by_node_request request;
    ::unmarshall(msg, request);
    _state->query_configuration_by_node(request, response);
    reply(msg, response);    
}

void meta_service::on_query_configuration_by_index(dsn_message_t msg)
{
    if (!_started)
    {
        configuration_query_by_index_response response;
        response.err = ERR_SERVICE_NOT_ACTIVE;
        reply(msg, response);
        return;
    }

    if (!_failure_detector->is_primary())
    {
        dsn_rpc_forward(msg, _failure_detector->get_primary().c_addr());
        return;
    }
        
    configuration_query_by_index_response response;
    configuration_query_by_index_request request;
    ::unmarshall(msg, request);
    _state->query_configuration_by_index(request, response);
    reply(msg, response);
}

void meta_service::on_modify_replica_config_explictly(dsn_message_t req)
{
    // TODO: implement modify config with reply
    if (!_started)
    {
        configuration_query_by_index_response response;
        response.err = ERR_SERVICE_NOT_ACTIVE;
        reply(req, response);
        return;
    }

    if (!_failure_detector->is_primary())
    {
        dsn_rpc_forward(req, _failure_detector->get_primary().c_addr());
        return;
    }

    dsn::replication::global_partition_id gpid;
    int role;
    dsn::replication::config_type cfg_command;

    ::unmarshall(req, gpid);
    ::unmarshall(req, role);
    ::unmarshall(req, cfg_command);

    switch (cfg_command)
    {
    // ignore all other cfgs as the command is triggerd currently in meta
    case CT_DOWNGRADE_TO_INACTIVE:
    case CT_DOWNGRADE_TO_SECONDARY:
    case CT_REMOVE:
        _balancer->explictly_send_proposal(gpid, role, cfg_command);
        break;
    default:
        break;
    }
}

void meta_service::on_update_configuration(dsn_message_t req)
{
    if (!_started)
    {
        configuration_update_response response;
        response.err = ERR_SERVICE_NOT_ACTIVE;
        reply(req, response);
        return;
    }

    if (!_failure_detector->is_primary())
    {
        dsn_rpc_forward(req, _failure_detector->get_primary().c_addr());
        return;
    }
    
    std::shared_ptr<configuration_update_request> request(new configuration_update_request);
    ::unmarshall(req, *request);

    if (_state->freezed())
    {
        configuration_update_response response;
        
        response.err = ERR_STATE_FREEZED;
        _state->query_configuration_by_gpid(request->config.gpid, response.config);

        reply(req, response);
        return;
    }
  
    global_partition_id gpid = request->config.gpid;
    _state->update_configuration(request, req, [this, gpid](){
        if (_started)
        {
            tasking::enqueue(LPC_LBM_RUN, this, std::bind(&meta_service::on_config_changed, this, gpid));
        }
    });
}

void meta_service::update_configuration_on_machine_failure(std::shared_ptr<configuration_update_request>& update)
{
    global_partition_id gpid = update->config.gpid;
    _state->update_configuration(update, nullptr, [this, gpid](){
        if (_started)
        {
            tasking::enqueue(LPC_LBM_RUN, this, std::bind(&meta_service::on_config_changed, this, gpid));
        }  
    });
}

// local timers
void meta_service::on_load_balance_timer()
{
    if (!_started)
        return;

    if (_state->freezed())
        return;

    if (_failure_detector->is_primary())
    {
        _balancer->run();
    }
}

void meta_service::on_config_changed(global_partition_id gpid)
{
    if (_failure_detector->is_primary())
    {
        _balancer->run(gpid);
    }
}
