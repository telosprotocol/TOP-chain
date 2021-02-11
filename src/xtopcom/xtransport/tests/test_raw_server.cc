// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string.h>
#include <string>
#include <gtest/gtest.h>

#define private public
#define protected public
#include "xpbase/base/top_log.h"
#include "xtransport/udp_transport/udp_transport.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xtransport/udp_transport/raw_udp_socket.h"

namespace top {
namespace kadmlia {
namespace test {

class TestRawServer : public testing::Test {
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
typedef std::shared_ptr<top::transport::RawUdpSocket> RawUdpSocketPtr;

TEST_F(TestRawServer, Handshake) {
    RawUdpSocketPtr udp_transport;
    base::xiothread_t* io_thread_;
    io_thread_ = top::base::xiothread_t::create_thread(
        top::base::xcontext_t::instance(), 0, -1);

    auto thread_message_handler = std::make_shared<transport::MultiThreadHandler>();
    thread_message_handler->Init();
    uint16_t port = 20000;
    xfd_handle_t udp_handle_;
    udp_handle_ = base::xsocket_utl::udp_listen(
            std::string("0.0.0.0"), port);
    udp_transport.reset(new top::transport::RawUdpSocket(base::xcontext_t::instance(),
            io_thread_->get_thread_id(),
            udp_handle_,
            thread_message_handler.get()));

    auto local_port = udp_transport->local_port_;
    transport::protobuf::RoutingMessage message;
    base::xpacket_t packet;
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        TOP_INFO("RoutingMessage SerializeToString failed!");
        return;
    }
    xbyte_buffer_t xdata{msg.begin(), msg.end()};

    udp_transport->SendData(xdata, "127.0.0.1", local_port);
    udp_transport->SendData(packet);
    udp_transport->SendToLocal(packet);
    udp_transport->SendToLocal(xdata);
    xpacket_t test_packet(top::base::xcontext_t::instance()); //assume now it is connected
    test_packet.set_to_ip_addr("192.168.50.217");
    test_packet.set_to_ip_port(1000);
    std::string test_raw_data(1024, 'a');
    test_packet.get_body().push_back((uint8_t*)test_raw_data.data(), (int)test_raw_data.size());
    udp_transport->recv(0,0,0,0, test_packet, 0,0, NULL);

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
    udp_transport->GetSocketStatus();
    top::transport::UdpPropertyPtr prop;
    udp_transport->SendDataWithProp(packet, prop);

    udp_transport->Stop();
}

}  // namespace test
}  // namespace kadmlia
}  // namespace top
