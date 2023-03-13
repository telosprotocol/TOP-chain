// #include "xbasic/xasio_io_context_wrapper.h"
// #include "xvnetwork/xaddress.h"
// #include "xvnetwork/xversion.h"
// #include "xvnetwork/tests/xdummy_vhost.h"

// #include <gtest/gtest.h>
// #include <unistd.h>
// #include <sstream>
// #include <iostream>

// #ifndef ASIO_STANDALONE
// #define ASIO_STANDALONE
// #endif//ASIO_STANDALONE
// #ifndef USE_STANDALONE_ASIO
// #define USE_STANDALONE_ASIO
// #endif//ASIO_STANDALONE
// #include "xrpc/xrpc_define.h"
// #include "xrpc/xhttp/xhttp_server.h"
// #include "xrpc/xshard/xshard_rpc_handler.h"
// #include "simplewebserver/client_http.hpp"
// #include "xbase/xobject_ptr.h"
// #include "xcommon/xsharding_info.h"
// #include "xvnetwork/xversion.h"
// #include "xvnetwork/xaddress.h"
// #include "xcommon/xnode_id.h"
// #include "vrouter_mock.h"

// NS_BEG3(top, xrpc, test)
// using namespace std;
// using namespace common;
// using namespace vnetwork;
// using namespace network;
// using vnetwork::tests::xdummy_vhost_t;
// using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

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

// class xrpc_mock_api_test : public testing::Test
// {
// public:
//     void SetUp() override {
//         using namespace top;
//         using namespace vnetwork;
//         using namespace network;
//         xinit_log("/tmp", true, true);
//         xset_log_level((enum_xlog_level)0);
//         m_vedgehost_ptr = top::make_unique<xmock_vhost>();
//         m_vshardhost_ptr = top::make_unique<xmock_vhost>();

//         m_router_ptr = std::make_shared<xmock_router>();
//         m_edge_vhost_ptr = std::make_shared<xrpc_edge_vhost>(m_vedgehost_ptr.get(), m_router_ptr);
//         m_http_server_ptr = std::make_shared<xhttp_server>(m_edge_vhost_ptr);
//         m_http_server_ptr->start(19528);
//         m_shard_handler_ptr = top::make_unique<xshard_rpc_handler>(m_vshardhost_ptr.get());

//     }
//     void TearDown() override {
//     }
// public:
//     shared_ptr<xrouter_face> m_router_ptr;
//     unique_ptr<xdummy_vhost_t> m_vedgehost_ptr;
//     unique_ptr<xdummy_vhost_t> m_vshardhost_ptr;
//     shared_ptr<xrpc_edge_vhost> m_edge_vhost_ptr;
//     shared_ptr<xhttp_server> m_http_server_ptr;
//     unique_ptr<xshard_rpc_handler> m_shard_handler_ptr;
//     xjson_proc_t m_json_proc;
// };

// TEST_F(xrpc_mock_api_test, token)
// {
//     usleep(100);
//     string mock_account_token;
//     HttpClient client("localhost:19528");
//     string json_string = "version=1.0&account_address=T-123456789012345678901234567890123&method=request_token&sequence_id=1";
//     int16_t count = 0;
//     client.request("POST", "/", json_string, [&count, &mock_account_token](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
//         if (!ec)
//         {
//             ++count;
//             string result = response->content.string();
//             printf("%s\n", result.c_str());
//             Json::Reader reader;
//             Json::Value result_json;
//             if (reader.parse(result, result_json)) {
//                 mock_account_token = result_json["data"]["token"].asString();
//             }
//             EXPECT_EQ(true, result.find("0") != string::npos);
//         }
//     });
//     std::thread io_thread([&client] {
//         client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
//     });
//     io_thread.detach();
//     usleep(1000000);

//     //request account_info
//     HttpClient client2("localhost:19528");
//     string json_string2 = "version=1.0&account_address=T-123456789012345678901234567890123&token="+mock_account_token+"&method=account_info&sequence_id=2&body={\"version\": \"1.0\",\"method\": \"account_info\", \"params\":{\"account\":\"T-123456789012345678901234567890123\"}}";
//     count = 0;
//     client2.request("POST", "/", json_string2, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
//         if (!ec)
//         {
//             ++count;
//             string result = response->content.string();
//             printf("%s\n", result.c_str());
//             EXPECT_EQ(true, result.find("0") != string::npos);
//         }
//     });
//     std::thread io_thread2([&client2] {
//         client2.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
//     });
//     io_thread2.detach();
//     usleep(1000000);
//     xedge_handler_base<http_response_t>* edge_handler = m_http_server_ptr->get_edge_method()->get_edge_handler();
//     xrpc_msg_response_t shard_msg;
//     shard_msg.m_type = enum_xrpc_type::enum_xrpc_http_type;
//     shard_msg.m_uuid = edge_handler->get_seq_id();
//     shard_msg.m_message_body = "{\"errno\": \"0\",\"errmsg\": \"success\"}";
//     shard_msg.m_signature_address = m_router_ptr->get_shard(0,"T-123456789012345678901234567890123");
//     //xmessage_t msg(codec::xmsgpack_codec_t<xrpc_msg_t>::encode(shard_msg), rpc_msg_response);
//     edge_handler->on_message({}, shard_msg);
//     usleep(1005000);
//     EXPECT_EQ(1, count);

//     // request transfer
//     HttpClient client3("localhost:19528");
//     string json_string3 = "version=1.0&account_address=T-123456789012345678901234567890123&token="+mock_account_token+"&method=transfer&sequence_id=4&body={\"version\": \"1.0\",\"method\": \"transfer\", \"params\":{\"from\":\"T-123456789012345678901234567890123\", \"to\":\"T-123456789012345678901234567890124\",\"amount\":10,\"nonce\":0,\"last_hash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\"}}";
//     count = 0;
//     client3.request("POST", "/", json_string3, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
//         if (!ec)
//         {
//             ++count;
//             string result = response->content.string();
//             printf("%s\n", result.c_str());
//             EXPECT_EQ(true, result.find("0") != string::npos);
//         }
//     });
//     std::thread io_thread3([&client3] {
//         client3.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
//     });
//     io_thread3.detach();
//     usleep(1000000);
//     //xedge_handler_base<http_response_t>* edge_handler = m_http_server_ptr->get_edge_method()->get_edge_handler();
//     //xrpc_msg_response_t shard_msg;
//     shard_msg.m_type = enum_xrpc_type::enum_xrpc_http_type;
//     shard_msg.m_uuid = edge_handler->get_seq_id();
//     shard_msg.m_message_body = "{\"errno\": \"0\",\"errmsg\": \"success\"}";
//     shard_msg.m_signature_address = m_router_ptr->get_shard(0,"T-123456789012345678901234567890123");
//     //xmessage_t msg(codec::xmsgpack_codec_t<xrpc_msg_t>::encode(shard_msg), rpc_msg_response);
//     edge_handler->on_message({}, shard_msg);
//     usleep(1005000);
//     EXPECT_EQ(1, count);

// }
/*
TEST(xrpc_mock_api_test, forward)
{
    usleep(100);
    HttpClient client("localhost:19528");
    string json_string = "account_address=T-123456789012345678901234567890123&token="+mock_account_token+"&method=account_info&sequence_id=2&body={\"version\": \"1.0\",\"method\": \"account_info\", \"params\":{\"account\":\"T-123456789012345678901234567890123\"}}";
    int16_t count = 0;
    client.request("POST", "/", json_string, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
        if (!ec)
        {
            ++count;
            string result = response->content.string();
            EXPECT_EQ(true, result.find("0") != string::npos);
        }
    });
    std::thread io_thread([&client] {
        client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
    });
    io_thread.detach();
    usleep(1000000);
    xedge_handler_base<http_response_t>* edge_handler = g_server_api_test.m_http_server_ptr->get_edge_method()->get_edge_handler();
    xrpc_msg_response_t shard_msg;
    shard_msg.m_type = enum_xrpc_type::enum_xrpc_http_type;
    shard_msg.m_uuid = edge_handler->get_seq_id();
    shard_msg.m_message_body = "{\"errno\": \"0\",\"errmsg\": \"success\"}";
    shard_msg.m_signature_address = g_server_api_test.m_router_ptr->get_shard(0,"T-123456789012345678901234567890123");
    //xmessage_t msg(codec::xmsgpack_codec_t<xrpc_msg_t>::encode(shard_msg), rpc_msg_response);
    edge_handler->on_message({}, shard_msg);
    usleep(1005000);
    EXPECT_EQ(1, count);
    //cout <<"forward:" << r1->content.rdbuf() << endl; // Alternatively, use the convenience function r1->content.string()
}

TEST(xrpc_mock_api_test, timeout)
{
    usleep(100);
    HttpClient client("localhost:19528");
    string json_string = "account_address=T-123456789012345678901234567890123&token="+mock_account_token+"&method=account_info&sequence_id=2&body={\"version\": \"1.0\",\"method\": \"account_info\", \"params\":{\"account\":\"T-123456789012345678901234567890123\"}}";
    int16_t count = 0;
    client.request("POST", "/", json_string, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
        if (!ec)
        {
            ++count;
            string result = response->content.string();
            EXPECT_EQ(true, result.find("99") != string::npos);
        }
    });
    std::thread io_thread([&client] {
        client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2* TIME_OUT +1));
    });
    io_thread.detach();
    sleep(2 * TIME_OUT + 1);
    EXPECT_EQ(1, count);
}*/
// NS_END3
