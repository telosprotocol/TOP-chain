#include "xbasic/xasio_io_context_wrapper.h"
#include "xvnetwork/xaddress.h"

#include <gtest/gtest.h>

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
#include "xrpc/xcluster/xcluster_rpc_handler.h"
#include "simplewebsocketserver/client_ws.hpp"
#include "xbase/xobject_ptr.h"

NS_BEG3(top, xrpc, test)
/*using namespace std;
#define SIZE xnetwork_fixture_t<6>::size
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

class test_ws_server
{
public:
    test_ws_server()
    {
        using namespace top;
        using namespace vnetwork;
        using namespace network;
        xinit_log("/tmp", true, true);
        xset_log_level((enum_xlog_level)0);
        m_network_fixture.init();
        xinfo("=test_ws_server==");
        std::array<xnode_id_t, SIZE> node_ids;
        for (auto i = 0u; i < SIZE; ++i)
        {
            node_ids[i] = m_network_fixture.m_network_driver_manager.object(i).host_node_id();
        }

        for (auto i = 0u; i < m_network_fixture.m_network_driver_manager.object_count() - 1; ++i) {
            auto const j = i + 1;
            auto & dht_host = m_network_fixture.m_network_driver_manager.object(j).dht_host();
            m_network_fixture.m_network_driver_manager.object(i).p2p_bootstrap({ dht_host.host_dht_node() });
        }

        std::array<std::shared_ptr<xvnetwork_t>, SIZE> vnetworks;
        xversion_t const version
        {
            top::xid_t<xversion_t>{ 0 }
        };

        for (auto i = 0u; i < SIZE; ++i)
        {
            auto vnetwork = m_network_fixture.m_vhost_mgr.object(i).add_vnetwork({ version, m_network_fixture.m_vhost_mgr.object(i).network_info() });
            auto zone = vnetwork->add_zone_vnode(common::xzone_id_t{ 0 });
            //auto advance_cluster = zone->add_cluster_vnode(common::xcluster_id_t{ 0 }, common::xnode_type_t::consensus_auditor);
            //[[maybe_unused]]auto advance_vnode = advance_cluster->add_vnode(node_ids[0]);
            auto consensus_cluster = zone->add_cluster_vnode(common::xcluster_id_t{ 0 }, common::xnode_type_t::consensus_validator);
            consensus_cluster->add_vnode(node_ids[0]);
            consensus_cluster->add_vnode(node_ids[1]);
            consensus_cluster = zone->add_cluster_vnode(common::xcluster_id_t{ 1 }, common::xnode_type_t::consensus_validator);
            consensus_cluster->add_vnode(node_ids[2]);
            consensus_cluster->add_vnode(node_ids[3]);

            auto advance_cluster = zone->add_cluster_vnode(common::xcluster_id_t{ 0x00010000 }, common::xnode_type_t::consensus_auditor);
            auto advance_vnode = advance_cluster->add_vnode(node_ids[4]);
            auto edge_cluster = zone->add_cluster_vnode(common::xcluster_id_t{ 255 }, xvnode_type_t::edge);
            auto edge_vnode = edge_cluster->add_vnode(node_ids[5]);
        }

        m_router_ptr = std::make_shared<xrouter>(std::addressof(m_network_fixture.m_vhost_mgr.object(5)));;
        m_edge_vhost_ptr = std::make_shared<xrpc_edge_vhost>(std::addressof(m_network_fixture.m_vhost_mgr.object(5)), m_router_ptr);
        m_ws_server_ptr = std::make_shared<xws_server>(m_edge_vhost_ptr);
        m_ws_server_ptr->start(19529);

        m_cluster_handler_ptr = std::make_unique<xcluster_rpc_handler>(std::addressof(m_network_fixture.m_vhost_mgr.object(4)), m_router_ptr);
        for(int i = 0; i < SIZE - 2; ++i)
        {
            m_shard_handler_ptr_vec.emplace_back(std::make_unique<xshard_rpc_handler>(std::addressof(m_network_fixture.m_vhost_mgr.object(i))));
        }

    }
    ~test_ws_server()
    {
        xinfo("=~test_ws_server==");
        m_network_fixture.close();
    }
public:
    top::xrpc::test::xnetwork_fixture_t<6> m_network_fixture;
    shared_ptr<xrouter_face> m_router_ptr;
    shared_ptr<xws_server> m_ws_server_ptr;
    shared_ptr<xrpc_edge_vhost> m_edge_vhost_ptr;
    vector<unique_ptr<xshard_rpc_handler>> m_shard_handler_ptr_vec;
    unique_ptr<xcluster_rpc_handler> m_cluster_handler_ptr;
    xjson_proc_t m_json_proc;
};
static test_ws_server* g_server_api_test = new test_ws_server();
static string ws_account_token;

TEST(test_ws_server, token)
{
    usleep(100);
    int16_t count = 0;

    WsClient client("localhost:19529/");
    client.on_message = [&count](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        string result = in_message->string();
        cout << "Client: Message received: \"" << result << "\"" << endl;
        ++count;
        Json::Reader reader;
        Json::Value result_json;
        if (reader.parse(result, result_json)) {
            ws_account_token = result_json["data"]["token"].asString();
        }
        EXPECT_EQ(true, result.find("0") != string::npos);
        cout << "Client: Sending close connection" << endl;
        connection->send_close(1000);
    };

    client.on_open = [](shared_ptr<WsClient::Connection> connection) {
        cout << "Client: Opened connection" << endl;
        string json_string = "account_address=T-123456789012345678901234567890123&method=request_token&sequence_id=1";
        string out_message(json_string);
        cout << "Client: Sending message: \"" << out_message << "\"" << endl;

        connection->send(out_message);
    };

    client.on_close = [](shared_ptr<WsClient::Connection> , int status, const string & ) {
        cout << "Client: Closed connection with status code " << status << endl;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> , const SimpleWeb::error_code &ec) {
        cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
    };

    client.start();
    sleep(1);

    EXPECT_GE(count, 1);
    //cout <<"timeout:" << r1->content.rdbuf() << endl; // Alternatively, use the convenience function r1->content.string()
    //string result = r1->content.string();
 //   EXPECT_EQ(true, result.find("99") != string::npos);
}

TEST(test_ws_server, forward)
{
    usleep(100);
    int16_t count = 0;

    WsClient client("localhost:19529/");
    client.on_message = [&count](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        string result = in_message->string();
        cout << "Client: Message received: \"" << result << "\"" << endl;
        ++count;
        EXPECT_EQ(true, result.find("0") != string::npos);
        cout << "Client: Sending close connection" << endl;
        connection->send_close(1000);
    };

    client.on_open = [](shared_ptr<WsClient::Connection> connection) {
        cout << "Client: Opened connection" << endl;
        string json_string = "account_address=T-123456789012345678901234567890123&token="+ ws_account_token +"&method=create_account&sequence_id=2&body={\"version\": \"1.0\",\"action\": \"create_account\", \"params\":{\"account\":\"T-123456789012345678901234567890129\"}}";
        string out_message(json_string);
        cout << "Client: Sending message: \"" << out_message << "\"" << endl;

        connection->send(out_message);
    };

    client.on_close = [](shared_ptr<WsClient::Connection> , int status, const string & ) {
        cout << "Client: Closed connection with status code " << status << endl;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> , const SimpleWeb::error_code &ec) {
        cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
    };

    client.start();
    sleep(1);

    EXPECT_GE(count, 1);
    //cout <<"timeout:" << r1->content.rdbuf() << endl; // Alternatively, use the convenience function r1->content.string()
    //string result = r1->content.string();
 //   EXPECT_EQ(true, result.find("99") != string::npos);
}

TEST(test_ws_server, timeout)
{
    //// g_server_api_test.m_shard_handler_ptr = std::make_unique<xshard_rpc_handler>(g_server_api_test.m_network_fixture.m_vhost_mgr->vhost(0).get(), g_server_api_test.m_network_fixture.m_vhost_mgr->vhost(1).get());
    //HttpClient client("localhost:9527");
    //string json_string = "{\"version\": \"1.0\",\"action\": \"account_balance\"}";
    //auto r1 = client.request("POST", "/", json_string);
    //cout <<"forward:" << r1->content.rdbuf() << endl; // Alternatively, use the convenience function r1->content.string()
    EXPECT_EQ(0, 0);
}

TEST(test_ws_server, exit)
{
    delete g_server_api_test;
    EXPECT_EQ(0, 0);
    // g_server_api_test.m_http_server_ptr->m_server_thread.detach();
}*/
NS_END3
