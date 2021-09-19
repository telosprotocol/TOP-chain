#include "xbasic/xasio_io_context_wrapper.h"
#include "xvnetwork/xaddress.h"

#include <gtest/gtest.h>
#include <unistd.h>
#include <sstream>
#include <iostream>

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif//ASIO_STANDALONE
#ifndef USE_STANDALONE_ASIO
#define USE_STANDALONE_ASIO
#endif//ASIO_STANDALONE
#include "xrpc/xrpc_define.h"
#include "xrpc/xws/xws_server.h"
#include "xrpc/xshard/xshard_rpc_handler.h"
#include "simplewebsocketserver/client_ws.hpp"
#include "xbase/xobject_ptr.h"
#include "xcommon/xsharding_info.h"
#include "xvnetwork/xaddress.h"
#include "xcommon/xnode_id.h"

NS_BEG3(top, xrpc, test)
using namespace std;
using namespace common;
using namespace vnetwork;
using namespace network;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

// class xmock_vhost : public xdummy_vhost_t {
//     xvnode_address_t last_address() const override {
//         uint32_t id = 1;
//         xsharding_info_t si{ xnetwork_id_t{ id }, xzone_id_t{ id }, xcluster_id_t{ id } };
//         xversion_t const v
//         {
//             top::xid_t<xversion_t>{ id }
//         };
//         xsharding_address_t sa{ si, v, xvnode_type_t::cluster };
//         xaccount_address_t aa{ xnode_id_t{std::to_string(id)}};
//         xvnode_address_t node{ sa, aa};
//         return node;
//     }
//     xnetwork_info_t const & network_info() const noexcept override {
//         return m_network_info;
//     }
//     protected:
//     xnetwork_info_t m_network_info;
// };

// class test_ws_server_mock
// {
// public:
//     test_ws_server_mock()
//     {
//         using namespace top;
//         using namespace vnetwork;
//         using namespace network;
//         xinit_log("/tmp", true, true);
//         xset_log_level((enum_xlog_level)0);
//         m_vedgehost_ptr = top::make_unique<xmock_vhost>();
//         m_vshardhost_ptr = top::make_unique<xmock_vhost>();

//         m_router_ptr = std::make_shared<xmock_router>();
//         m_edge_vhost_ptr = std::make_shared<xrpc_edge_vhost>(m_vedgehost_ptr.get(), m_router_ptr);
//         m_ws_server_ptr = std::make_shared<xws_server>(m_edge_vhost_ptr);
//         m_ws_server_ptr->start(19530);
//         m_shard_handler_ptr = top::make_unique<xshard_rpc_handler>(m_vshardhost_ptr.get());

//     }
//     ~test_ws_server_mock()
//     {
//     }
// public:
//     shared_ptr<xrouter_face> m_router_ptr;
//     unique_ptr<xdummy_vhost_t> m_vedgehost_ptr;
//     unique_ptr<xdummy_vhost_t> m_vshardhost_ptr;
//     shared_ptr<xrpc_edge_vhost> m_edge_vhost_ptr;
//     shared_ptr<xws_server> m_ws_server_ptr;
//     unique_ptr<xshard_rpc_handler> m_shard_handler_ptr;
//     xjson_proc_t m_json_proc;
// };
// static test_ws_server_mock g_server_api_test;

// TEST(test_ws_server_mock, forward)
// {
//     usleep(100);

//     int16_t count = 0;

//     WsClient client("localhost:19530/");
//     client.on_message = [&count](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
//         cout << "Client: Message received: \"" << in_message->string() << "\"" << endl;
//         ++count;
//         cout << "Client: Sending close connection" << endl;
//         connection->send_close(1000);
//     };

//     client.on_open = [](shared_ptr<WsClient::Connection> connection) {
//         cout << "Client: Opened connection" << endl;
//         string json_string = "{\"version\": \"1.0\",\"method\": \"account_balance\",\"id\":1, \"params\":{\"account\":\"T - 1\"}}";
//         string out_message(json_string);
//         cout << "Client: Sending message: \"" << out_message << "\"" << endl;

//         connection->send(out_message);
//     };

//     client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
//         cout << "Client: Closed connection with status code " << status << endl;
//     };

//     // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
//     client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
//         cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
//     };
//     std::thread client_thread([&client]{
//         client.start();
//     });
//     usleep(1000000);
//     xedge_handler_base<ws_connection_t>* edge_handler = g_server_api_test.m_ws_server_ptr->get_edge_method()->get_edge_handler();
//     xrpc_msg_response_t shard_msg;
//     shard_msg.m_type = enum_xrpc_type::enum_xrpc_ws_type;
//     shard_msg.m_uuid = edge_handler->get_seq_id();
//     shard_msg.m_message_body = "{\"errno\": \"0\",\"errmsg\": \"success\"}";
//     shard_msg.m_signature_address = g_server_api_test.m_router_ptr->get_shard(0,"T - 1");
//     //xmessage_t msg(codec::xmsgpack_codec_t<xrpc_msg_t>::encode(shard_msg), rpc_msg_response);
//     edge_handler->on_message({}, shard_msg);
//     client_thread.join();
//     EXPECT_EQ(1, count);


//     //cout <<"forward:" << r1->content.rdbuf() << endl; // Alternatively, use the convenience function r1->content.string()
// }

// TEST(test_ws_server_mock, timeout)
// {
//     usleep(100);
//     int16_t count = 0;

//     WsClient client("localhost:19530/");
//     client.on_message = [&count](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
//         cout << "Client: Message received: \"" << in_message->string() << "\"" << endl;
//         ++count;
//         cout << "Client: Sending close connection" << endl;
//         connection->send_close(1000);
//     };

//     client.on_open = [](shared_ptr<WsClient::Connection> connection) {
//         cout << "Client: Opened connection" << endl;
//         string json_string = "{\"version\": \"1.0\",\"method\": \"account_balance\",\"id\":1, \"params\":{\"account\":\"T - 1\"}}";
//         string out_message(json_string);
//         cout << "Client: Sending message: \"" << out_message << "\"" << endl;

//         connection->send(out_message);
//     };

//     client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
//         cout << "Client: Closed connection with status code " << status << endl;
//     };

//     client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
//         cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
//     };

//     std::thread client_thread([&client] {
//         client.start();
//     });
//     client_thread.detach();
//     sleep(2 * TIME_OUT + 1);
//     EXPECT_EQ(1, count);
// }
NS_END3
