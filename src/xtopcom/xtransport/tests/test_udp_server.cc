// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string.h>
#include <memory>
#include <string>

#include "xpbase/base/top_log.h"

#include <gtest/gtest.h>

#define private public
#include "xtransport/udp_transport/udp_transport.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xtransport/udp_transport/xudp_socket.h"
#include "xtransport/src/message_manager.h"

namespace top {
namespace kadmlia {
namespace test {

class TestUdpServer : public testing::Test {
public:
	static void SetUpTestCase() {
	}

	static void TearDownTestCase() {
	}

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}
};

TEST_F(TestUdpServer, Handshake) {
    top::transport::UdpTransportPtr udp_transport;
    udp_transport.reset(new top::transport::UdpTransport());
    auto thread_message_handler = std::make_shared<transport::MultiThreadHandler>();
    thread_message_handler->Init();
    {
        top::transport::on_dispatch_callback_t cb;
        thread_message_handler->register_on_dispatch_callback(cb);
    }
    {
        base::xiothread_t* io_thread_;
        io_thread_ = top::base::xiothread_t::create_thread(
            top::base::xcontext_t::instance(), 0, -1);
        top::transport::ThreadHandler* handle = new top::transport::ThreadHandler(io_thread_, 0);
        handle->unregister_on_dispatch_callback();
        delete handle;
    }
    {
        base::xiothread_t* io_thread_;
        io_thread_ = top::base::xiothread_t::create_thread(
            top::base::xcontext_t::instance(), 0, -1);
        std::shared_ptr<top::transport::ThreadHandler> handle;
        handle = std::make_shared<top::transport::ThreadHandler>(io_thread_, 0);
    }
    
    ASSERT_TRUE(udp_transport->Start(
            "127.0.0.1",
            0,
            thread_message_handler.get()) == top::transport::kTransportSuccess);
    auto local_port = udp_transport->local_port();
    transport::protobuf::RoutingMessage message;
    base::xpacket_t packet;
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        TOP_INFO("RoutingMessage SerializeToString failed!");
        return;
    }
    xbyte_buffer_t xdata{msg.begin(), msg.end()};

    udp_transport->socket_connected_ = false;
    udp_transport->SendData(xdata, "127.0.0.1", local_port);
    udp_transport->SendData(packet);
    udp_transport->SendToLocal(packet);
    udp_transport->SendToLocal(xdata);
    top::transport::UdpPropertyPtr prop;
    udp_transport->SendDataWithProp(packet, prop);
    udp_transport->local_port_ = 19876;
    udp_transport->ReStartServer();    
    udp_transport->socket_connected_ = false;
    udp_transport->get_socket_status();
    udp_transport->SendPing(xdata, "127.0.0.1", local_port);
    udp_transport->SendPing(packet);

    udp_transport->socket_connected_ = true;
    udp_transport->SendData(xdata, "127.0.0.1", local_port);
    udp_transport->SendData(packet);
    udp_transport->SendToLocal(packet);
    udp_transport->SendToLocal(xdata);
    udp_transport->SendDataWithProp(packet, prop);
    udp_transport->ReStartServer();    
    udp_transport->get_socket_status();
    top::transport::on_receive_callback_t cb;
    udp_transport->register_on_receive_callback(cb);
    udp_transport->unregister_on_receive_callback();
    udp_transport->SendPing(xdata, "127.0.0.1", local_port);
    udp_transport->SendPing(packet);
    std::function<void(const std::string& ip, const uint16_t port)> cb2;
    udp_transport->RegisterOfflineCallback(cb2);
    std::function<int32_t(std::string const& node_addr, std::string const& node_sign)> cb3;
    udp_transport->RegisterNodeCallback(cb3);
    udp_transport->CheckRatelimitMap("127.0.0.1:19999");

    thread_message_handler->m_worker_threads.clear();
    thread_message_handler->HandleMessage(packet);
    thread_message_handler->unregister_on_dispatch_callback();
    udp_transport->Stop();

    top::transport::MessageManager manager;
    top::transport::HandlerProc proc;
    manager.RegisterMessageProcessor(0, proc);
    manager.UnRegisterMessageProcessor(0);
    manager.RegisterMessageProcessor(4095, proc);
}

}  // namespace test
}  // namespace kadmlia
}  // namespace top
