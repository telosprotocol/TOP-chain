// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define protected public
#define private public
// need to close BUILD_METRICS
// option(BUILD_METRICS "build metrics" ON)

#include <vector>
#include <thread>

#include <stdio.h>

#include "gtest/gtest.h"
#include "xtransport/udp_transport/xudp_socket.h"
#include "xbase/xcontext.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_log.h"
#include "xtransport/utils/transport_utils.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xtransport/udp_config.h"
#include "xbase/xutl.h"


using namespace top;
using namespace top::base;
using namespace top::transport;
using namespace std;


static XudpSocket* udp_socket_;
static shared_ptr<transport::MultiThreadHandler> thread_message_handler;
class test_xudp_socket : public testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {
    }

    static void TearDownTestCase() {
    }
    static void SetUpTestCase() {
        xfd_handle_t udp_handle_;
        base::xiothread_t* io_thread_;

        xset_log_level(enum_xlog_level_debug);
        TOP_INFO("enter SetUpTestCase");
        uint16_t port = 19999;
        
        io_thread_ = top::base::xiothread_t::create_thread(
            top::base::xcontext_t::instance(), 0, -1);
        if (io_thread_ == NULL) {
            TOP_ERROR("create xio thread failed!");
            return;
        }
        udp_handle_ = base::xsocket_utl::udp_listen(
                std::string("0.0.0.0"), port);
        if (udp_handle_ <= 0) {
            TOP_ERROR("udp listen failed!");
            return;
        }

        thread_message_handler = std::make_shared<transport::MultiThreadHandler>();
        thread_message_handler->Init();
        udp_socket_ = new XudpSocket(
            base::xcontext_t::instance(),
            io_thread_->get_thread_id(),
            udp_handle_,
            thread_message_handler.get());
        port = udp_socket_->GetLocalPort();
        udp_socket_->StartRead();    
        
    }
public:
    
};
TEST_F(test_xudp_socket, UdpProperty_test) {
/*    top::base::xiothread_t * t1 = top::base::xiothread_t::create_thread(top::base::xcontext_t::instance(),0,-1);
    XudpSocket *   client_listener_ptr =  NULL;
    uint16_t     udp_listen_port_1 = 0;
    std::string ip = "0.0.0.0";
    uint16_t port = 19999;
    xfd_handle_t udp_handle_1 = base::xsocket_utl::udp_listen(ip, port);
    
    client_listener_ptr = new XudpSocket(top::base::xcontext_t::instance(),t1->get_thread_id(),udp_handle_1);
    client_listener_ptr->start_read(t1->get_thread_id());*/
    xp2pudp_t * peer_xudp_socket_1 = NULL;
    xfd_handle_t handle;
    uint16_t port = 19999;
    handle = base::xsocket_utl::udp_listen(std::string("0.0.0.0"), port);
    xsocket_property property;

    peer_xudp_socket_1 = (xp2pudp_t*)udp_socket_->create_xslsocket(NULL, handle, property, 0, 0);

    UdpProperty  prop;
    std::string node_sign;
    prop.SetXudp(peer_xudp_socket_1);

    peer_xudp_socket_1->m_peer_account_payload = "aaaaaaa";
    peer_xudp_socket_1->encode_msg_type();
    peer_xudp_socket_1->m_peer_account_payload = "{\"msg_encode_type\": 0}";
    peer_xudp_socket_1->encode_msg_type();

    peer_xudp_socket_1->on_endpoint_open(0,0,0,NULL);
    peer_xudp_socket_1->on_endpoint_close(0,0,0,NULL);
    std::string payload;
    peer_xudp_socket_1->on_endpoint_keepalive(payload,0,0,NULL);
    udp_socket_->on_xslsocket_accept(handle,property,0,0);

    xpacket_t test_packet(top::base::xcontext_t::instance()); //assume now it is connected
    test_packet.set_to_ip_addr("192.168.50.217");
    test_packet.set_to_ip_port(1000);
    std::string test_raw_data(1024, 'a');
    test_packet.get_body().push_back((uint8_t*)test_raw_data.data(), (int)test_raw_data.size());
    peer_xudp_socket_1->recv(0,0,0,0, test_packet, 0,0, NULL);
}
TEST_F(test_xudp_socket, SendData) {
    uint16_t peer_port = 50002;
    xpacket_t test_packet(top::base::xcontext_t::instance()); //assume now it is connected
    test_packet.set_to_ip_addr("192.168.50.217");
    test_packet.set_to_ip_port(peer_port);
    std::string test_raw_data(1024, 'a');
    std::cout<<"send 2 packets"<<std::endl;
    UdpPropertyPtr ptr = nullptr;
//  ptr.reset(new top::transport::UdpProperty());   
    for (int i = 0;i< 1; i++) {
        test_packet.reset();
        test_packet.set_to_ip_addr("192.168.50.217");
        test_packet.set_to_ip_port(peer_port);
        test_packet.get_body().push_back((uint8_t*)test_raw_data.data(), (int)test_raw_data.size());
        udp_socket_->SendDataWithProp(test_packet, ptr);
        // std::this_thread::sleep_for(chrono::seconds(1));
    }
    UdpPropertyPtr  prop = make_shared<UdpProperty>();
    udp_socket_->SendDataWithProp(test_packet, prop);
    test_packet.set_to_ip_addr("0.0.0.0");
    test_packet.set_to_ip_port(19999);
    udp_socket_->SendDataWithProp(test_packet, ptr);

}
TEST_F(test_xudp_socket, CheckRatelimitMap) {
    std::list<int64_t> l;
    udp_socket_->xudp_ratelimit_map["192.1.1.1:10000"] = l;
    udp_socket_->CheckRatelimitMap("192.1.1.1:10000");
    l.push_back(1);
    udp_socket_->xudp_ratelimit_map["192.1.1.1:10000"] = l;
    udp_socket_->CheckRatelimitMap("192.1.1.1:10000");
}
TEST_F(test_xudp_socket, RegisterOfflineCallback) {
    std::function<void(const std::string& ip, const uint16_t port)> cb;
    udp_socket_->RegisterOfflineCallback(cb);
    std::function<int32_t(std::string const& node_addr, std::string const& node_sign)> cb2;
    udp_socket_->RegisterNodeCallback(cb2);
}
TEST_F(test_xudp_socket, SendToLocal) {
    xpacket_t test_packet(top::base::xcontext_t::instance()); //assume now it is connected
    test_packet.set_to_ip_addr("192.168.50.217");
    test_packet.set_to_ip_port(1000);
    std::string test_raw_data(1024, 'a');
    test_packet.get_body().push_back((uint8_t*)test_raw_data.data(), (int)test_raw_data.size());
    udp_socket_->SendToLocal(test_packet);

}
TEST_F(test_xudp_socket, register_on_receive_callback) {
    on_receive_callback_t callback;
    udp_socket_->register_on_receive_callback(callback);
    udp_socket_->unregister_on_receive_callback();
}
TEST_F(test_xudp_socket, ParserXip2Header) {
    xpacket_t test_packet(top::base::xcontext_t::instance()); //assume now it is connected
    udp_socket_->SendToLocal(test_packet);
    test_packet.set_to_ip_addr("192.168.50.217");
    test_packet.set_to_ip_port(1000);
    udp_socket_->ParserXip2Header(test_packet);
    std::string test_raw_data(1024, 'a');
    test_packet.get_body().push_back((uint8_t*)test_raw_data.data(), (int)test_raw_data.size());
    udp_socket_->ParserXip2Header(test_packet);
}
TEST_F(test_xudp_socket, GetSocketStatus) {
    udp_socket_->GetSocketStatus();
    udp_socket_->AddXudp("1.1.1.1:1000", NULL);
}
TEST_F(test_xudp_socket, SendPing) {
    transport::protobuf::RoutingMessage message;
    std::string msg;
    message.SerializeToString(&msg);
    xbyte_buffer_t xdata{msg.begin(), msg.end()};
    udp_socket_->SendPing(xdata, "192.168.50.217", 50002);
    udp_socket_->SendToLocal(xdata);
}
TEST_F(test_xudp_socket, on_ping_packet_recv) {
    xpacket_t test_packet(top::base::xcontext_t::instance()); //assume now it is connected
    base::xlink_ping_pdu _pdu(top::base::xcontext_t::instance(),0);
    _pdu.serialize_to(test_packet);
    udp_socket_->on_ping_packet_recv(test_packet, udp_socket_->get_current_thread_id(), 0, NULL);
}
TEST_F(test_xudp_socket, stop_read) {
//  udp_socket_->on_endpoint_close(0,udp_socket_->get_current_thread_id(),1,NULL);
//  std::this_thread::sleep_for(chrono::seconds(10));
    udp_socket_->stop_read(0);
    udp_socket_->Stop();
    udp_socket_->close();
    udp_socket_->release_ref();
}
