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

#include "asio_net_provider.h"
#include "asio_rpc_session.h"

namespace dsn {
    namespace tools{

        asio_network_provider::asio_network_provider(rpc_engine* srv, network* inner_provider)
            : connection_oriented_network(srv, inner_provider)
        {
            _acceptor = nullptr;
        }

        error_code asio_network_provider::start(rpc_channel channel, int port, bool client_only, io_modifer& ctx)
        {
            if (_acceptor != nullptr)
                return ERR_SERVICE_ALREADY_RUNNING;

            int io_service_worker_count = config()->get_value<int>("network", "io_service_worker_count", 1,
                "thread number for io service (timer and boost network)");
            for (int i = 0; i < io_service_worker_count; i++)
            {
                _workers.push_back(std::shared_ptr<std::thread>(new std::thread([this, ctx, i]()
                {
                    task::set_tls_dsn_context(node(), nullptr, ctx.queue);

                    const char* name = ::dsn::tools::get_service_node_name(node());
                    char buffer[128];
                    sprintf(buffer, "%s.asio.%d", name, i);
                    task_worker::set_name(buffer);

                    boost::asio::io_service::work work(_io_service);
                    _io_service.run();
                })));
            }

            _acceptor = nullptr;
                        
            dassert(channel == RPC_CHANNEL_TCP || channel == RPC_CHANNEL_UDP, "invalid given channel %s", channel.to_string());

            _address.assign_ipv4(get_local_ipv4(), port);

            if (!client_only)
            {
                auto v4_addr = boost::asio::ip::address_v4::any(); //(ntohl(_address.ip));
                ::boost::asio::ip::tcp::endpoint ep(v4_addr, _address.port());

                try
                {
                    _acceptor.reset(new boost::asio::ip::tcp::acceptor(_io_service, ep, true));
                    do_accept();
                }
                catch (boost::system::system_error& err)
                {
                    printf("boost asio listen on port %u failed, err: %s\n", port, err.what());
                    return ERR_ADDRESS_ALREADY_USED;
                }
            }            

            return ERR_OK;
        }

        rpc_session_ptr asio_network_provider::create_client_session(::dsn::rpc_address server_addr)
        {
            auto parser = new_message_parser();
            auto sock = std::shared_ptr<boost::asio::ip::tcp::socket>(new boost::asio::ip::tcp::socket(_io_service));
            return rpc_session_ptr(new asio_rpc_session(*this, server_addr, sock, parser, true));
        }

        void asio_network_provider::do_accept()
        {
            auto socket = std::shared_ptr<boost::asio::ip::tcp::socket>(
                new boost::asio::ip::tcp::socket(_io_service));

            _acceptor->async_accept(*socket,
                [this, socket](boost::system::error_code ec)
            {
                if (!ec)
                {
                    auto ip = socket->remote_endpoint().address().to_v4().to_ulong();
                    auto port = socket->remote_endpoint().port();
                    ::dsn::rpc_address client_addr(ip, port);

                    auto parser = new_message_parser();
                    rpc_session_ptr s = new asio_rpc_session(*this, client_addr, 
                        (std::shared_ptr<boost::asio::ip::tcp::socket>&)socket, parser, false);
                    this->on_server_session_accepted(s);
                }

                do_accept();
            });
        }
    }
}
