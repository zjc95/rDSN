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

#pragma once

#include <dsn/tool_api.h>

namespace dsn { namespace tools {

    class sim_network_provider;
    class sim_client_session : public rpc_session
    {
    public:
        sim_client_session(
            sim_network_provider& net, 
            ::dsn::rpc_address remote_addr, 
            std::shared_ptr<message_parser>& parser
            );

        virtual void connect();
        virtual void send(uint64_t signature) override;
    };

    class sim_server_session : public rpc_session
    {
    public:
        sim_server_session(
            sim_network_provider& net, 
            ::dsn::rpc_address remote_addr, 
            rpc_session_ptr& client, 
            std::shared_ptr<message_parser>& parser
            );

        virtual void send(uint64_t signature) override;

        virtual void connect() {}

    private:
        rpc_session_ptr _client;
    };

    class sim_network_provider : public connection_oriented_network
    {
    public:
        sim_network_provider(rpc_engine* rpc, network* inner_provider);
        ~sim_network_provider(void) {}

        virtual error_code start(rpc_channel channel, int port, bool client_only, io_modifer& ctx);
    
        virtual ::dsn::rpc_address address() { return _address; }

        virtual rpc_session_ptr create_client_session(::dsn::rpc_address server_addr)
        {
            auto parser = new_message_parser();
            return rpc_session_ptr(new sim_client_session(*this, server_addr, parser));
        }

        uint32_t net_delay_milliseconds() const;

    private:
        ::dsn::rpc_address    _address;
        uint32_t     _min_message_delay_microseconds;
        uint32_t     _max_message_delay_microseconds;
    };
        
    //------------- inline implementations -------------


}} // end namespace

