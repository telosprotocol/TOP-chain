// #include "xbasic/xasio_io_context_wrapper.h"
// #include "xvnetwork/xaddress.h"
// #include "xvnetwork/xversion.h"
// #include "vhost_network.hpp"

// #include <gtest/gtest.h>

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
// #include "xrpc/xcluster/xcluster_rpc_handler.h"
// #include "simplewebserver/client_http.hpp"
// #include "xrpc/xws/xws_server.h"

// NS_BEG3(top, xrpc, test)
// using namespace std;
// using namespace vnetwork::tests;
// using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;
/*
// #define DEFINE_VERSION_AND_VNET_ID(NUM)         \
//     xversion_t const version ## NUM {           \
//         top::xsimple_id_t<xversion_t>{ NUM }           \
//     };                                          \
//     xvnetwork_id_t const vnetwork_id ## NUM {   \
//         version ## NUM, this->m_network_info          \
//     }
*/
// template <std::size_t N, std::size_t TimerDriverCount, std::size_t IoContextCount>
// class xrpc_server_test : public xrpc_vhost_fixture_t<N, TimerDriverCount, IoContextCount>
// {
//     using base_t = xrpc_vhost_fixture_t<N, TimerDriverCount, IoContextCount>;
// protected:
//     void SetUp() override {
//         xinit_log("/tmp", true, true);
//         xset_log_level((enum_xlog_level)0);
//         base_t::SetUp();
//         using namespace top;
//         using namespace common;
//         using namespace vnetwork;
//         using namespace network;


//         DEFINE_VERSION_AND_VNET_ID(0);

//         std::size_t const zone_count{ 2 };
//         std::size_t const advance_cluster_count{ 1 };
//         std::size_t const consensus_cluster_count{ 1 };

//         xzone_id_t edge_zone_id{ 0 };
//         xcluster_id_t edge_cluster_id{ 0 };
//         xcluster_id_t archive_cluster_id{ 0 };

//         std::vector<xzone_data_t> zone_data;
//         for (auto i = 0u; i < zone_count; ++i) {
//             zone_data.push_back({ xzone_id_t{ i } });

//             switch (i) {
//                 case 0:
//                 {
//                     assert(zone_data[i].cluster_data.empty());
//                     zone_data[i].cluster_data.push_back({
//                         edge_cluster_id,
//                         xvnode_type_t::edge,
//                         std::vector<xnode_id_t>{ this->m_network_driver_manager.object(0).host_node_id() }
//                     });
//                     break;
//                 }

//                 default:
//                 {
//                     zone_data[i].cluster_data.push_back({ archive_cluster_id, xvnode_type_t::archive });
//                     for (auto j = 0u; j < advance_cluster_count; ++j) {
//                         zone_data[i].cluster_data.push_back({
//                             common::xcluster_id_t{ static_cast<std::uint64_t>(advance_cluster_id_base) + j },
//                             common::xnode_type_t::consensus_auditor
//                         });
//                     }
//                     for (auto j = 0u; j < consensus_cluster_count; ++j) {
//                         common::xcluster_id_t consensus_cluster_id{ static_cast<std::uint64_t>(consensus_cluster_id_base) + j };
//                         zone_data[i].cluster_data.push_back({
//                             consensus_cluster_id,
//                             common::xnode_type_t::consensus_validator
//                         });

//                         zone_data[i].cluster_relationship.insert({
//                             consensus_cluster_id,
//                             common::xcluster_id_t{ static_cast<std::uint64_t>(advance_cluster_id_base) + j % advance_cluster_count }
//                         });
//                     }

//                     break;
//                 }
//             }
//         }
//         this->m_vhost_manager->object(0).build_vnetwork({ version0, { this->m_network_info.network_id, zone_data } });
//         for (auto i = 1u; i < this->m_vhost_manager->object_count(); ++i)
//         {
//             this->m_vhost_manager->object(i).bootstrap(this->m_network_driver_manager.object(0).host_node(),
//                                                 { this->m_network_driver_manager.object(0).dht_host().host_dht_node() });
//             std::this_thread::sleep_for(std::chrono::seconds{1});
//         }
//         std::this_thread::sleep_for(std::chrono::seconds{60});

//         m_router_ptr = std::make_shared<xrouter>(&(this->m_vhost_manager->object(0)));
//         m_edge_vhost_ptr = std::make_shared<xrpc_edge_vhost>(&(this->m_vhost_manager->object(0)), m_router_ptr);
//         m_http_server_ptr = std::make_shared<xhttp_server>(m_edge_vhost_ptr);
//         m_http_server_ptr->start(19527);

//         m_cluster_handler_ptr = top::make_unique<xcluster_rpc_handler>(&(this->m_vhost_manager->object(1)), m_router_ptr);
//         for(int i = 3; i < 7; ++i)
//         {
//             m_shard_handler_ptr_vec.emplace_back(top::make_unique<xshard_rpc_handler>(&(this->m_vhost_manager->object(i))));
//         }
//     }
//     void TearDown() override {
//         base_t::TearDown();
//     }
//     public:
//     shared_ptr<xrouter_face> m_router_ptr;
//     shared_ptr<xhttp_server> m_http_server_ptr;
//     shared_ptr<xrpc_edge_vhost> m_edge_vhost_ptr;
//     vector<unique_ptr<xshard_rpc_handler>> m_shard_handler_ptr_vec;
//     unique_ptr<xcluster_rpc_handler> m_cluster_handler_ptr;
//     xjson_proc_t m_json_proc;
// };

// static string account_token;
// using x32_vhost_fixture = xrpc_server_test<7, 2, 2>;
// TEST_F(x32_vhost_fixture, bootstrap) {
//     usleep(100);
//     HttpClient client("localhost:19527");
//     string json_string = "version=1.0&account_address=T-123456789012345678901234567890123&method=request_token&sequence_id=1";
//     int16_t count = 0;
//     client.request("POST", "/", json_string, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
//         if (!ec)
//         {
//             ++count;
//             string result = response->content.string();
//             cout << result << endl;
//             Json::Reader reader;
//             Json::Value result_json;
//             if (reader.parse(result, result_json)) {
//                 account_token = result_json["data"]["token"].asString();
//             }
//             EXPECT_EQ(true, result.find("0") != string::npos);
//         }
//     });
//     std::thread io_thread([&client] {
//         client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
//     });
//     io_thread.detach();
//     usleep(2000000);

//     usleep(100);
//     HttpClient client2("localhost:19527");
//     json_string = "version=1.0&account_address=T-123456789012345678901234567890123&token="+account_token+"&method=account_info&sequence_id=2&body={\"version\": \"1.0\",\"method\": \"account_info\", \"params\":{\"account\":\"T-123456789012345678901234567890123\"}}";
//     int16_t count1 = 0;
//     client2.request("POST", "/", json_string, [&count1](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
//         if (!ec)
//         {
//             ++count1;
//             string result = response->content.string();
//             cout << result << endl;
//             EXPECT_EQ(true, result.find("0") != string::npos);
//         }
//     });
//     std::thread io_thread2([&client2] {
//         client2.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
//     });
//     io_thread2.detach();


//     HttpClient client3("localhost:19527");
//     json_string = "version=1.0&account_address=T-123456789012345678901234567890123&token="+account_token+"&method=transfer&sequence_id=4&body={\"version\": \"1.0\",\"method\": \"transfer\", \"params\":{\"from\":\"T-123456789012345678901234567890123\", \"to\":\"T-123456789012345678901234567890124\",\"amount\":10,\"nonce\":0,\"last_hash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\"}}";
//     int16_t count2 = 0;
//     client3.request("POST", "/", json_string, [&count2](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
//         if (!ec)
//         {
//             ++count2;
//             string result = response->content.string();
//             cout << result << endl;
//             EXPECT_EQ(true, result.find("0") != string::npos);
//         }
//     });
//     std::thread io_thread3([&client3] {
//         client3.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
//     });
//     io_thread3.detach();
//     usleep(5001000);
//     EXPECT_EQ(1, count1);
//     EXPECT_EQ(1, count2);
// }

/*
class xrpc_server_api_test
{
public:
    xrpc_server_api_test()
    {
        using namespace top;
        using namespace vnetwork;
        using namespace network;
        xinit_log("/tmp", true, true);
        xset_log_level((enum_xlog_level)0);
        xinfo("==xrpc_server_api_test==");
        m_network_fixture.init();

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

        m_router_ptr = std::make_shared<xrouter>(std::addressof(m_network_fixture.m_vhost_mgr.object(5)));
        m_edge_vhost_ptr = std::make_shared<xrpc_edge_vhost>(std::addressof(m_network_fixture.m_vhost_mgr.object(5)), m_router_ptr);
        m_http_server_ptr = std::make_shared<xhttp_server>(m_edge_vhost_ptr);
        m_http_server_ptr->start(19527);

        m_cluster_handler_ptr = top::make_unique<xcluster_rpc_handler>(std::addressof(m_network_fixture.m_vhost_mgr.object(4)), m_router_ptr);
        for(int i = 0; i < SIZE - 2; ++i)
        {
            m_shard_handler_ptr_vec.emplace_back(top::make_unique<xshard_rpc_handler>(std::addressof(m_network_fixture.m_vhost_mgr.object(i))));
        }

    }
    ~xrpc_server_api_test()
    {
        xinfo("==~xrpc_server_api_test==");
        m_network_fixture.close();
    }
public:
    top::xrpc::test::xnetwork_fixture_t<6> m_network_fixture;
    shared_ptr<xrouter_face> m_router_ptr;
    shared_ptr<xhttp_server> m_http_server_ptr;
    shared_ptr<xrpc_edge_vhost> m_edge_vhost_ptr;
    vector<unique_ptr<xshard_rpc_handler>> m_shard_handler_ptr_vec;
    unique_ptr<xcluster_rpc_handler> m_cluster_handler_ptr;
    xjson_proc_t m_json_proc;
};
static xrpc_server_api_test* g_server_api_test = new xrpc_server_api_test();
static string account_token;

TEST(xrpc_server_api_test, token)
{
    usleep(100);
    HttpClient client("localhost:19527");
    string json_string = "account_address=T-123456789012345678901234567890123&method=request_token&sequence_id=1";
    int16_t count = 0;
    client.request("POST", "/", json_string, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
        if (!ec)
        {
            ++count;
            string result = response->content.string();
            Json::Reader reader;
            Json::Value result_json;
            if (reader.parse(result, result_json)) {
                account_token = result_json["data"]["token"].asString();
            }
            EXPECT_EQ(true, result.find("0") != string::npos);
        }
    });
    std::thread io_thread([&client] {
        client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
    });
    io_thread.detach();
    usleep(1000000);
}

TEST(xrpc_server_api_test, forward)
{
    usleep(100);
    HttpClient client("localhost:19527");
    string json_string = "account_address=T-123456789012345678901234567890123&token="+account_token+"&method=account_info&sequence_id=2&body={\"version\": \"1.0\",\"method\": \"account_info\", \"params\":{\"account\":\"T-123456789012345678901234567890123\"}}";
    int16_t count = 0;
    client.request("POST", "/", json_string, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
        if (!ec)
        {
            ++count;
            string result = response->content.string();
            cout << result << endl;
            EXPECT_EQ(true, result.find("errno") != string::npos);
        }
    });
    std::thread io_thread([&client] {
        client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
    });
    io_thread.detach();
    usleep(2001000);
    EXPECT_EQ(1, count);

    //cout <<"timeout:" << r1->content.rdbuf() << endl; // Alternatively, use the convenience function r1->content.string()
    //string result = r1->content.string();
 //   EXPECT_EQ(true, result.find("99") != string::npos);
}

TEST(xrpc_server_api_test, elect_dump)
{
    usleep(100);
    HttpClient client("localhost:19527");
    string json_string = "account_address=T-123456789012345678901234567890123&token="+account_token+"&method=elect_dump&sequence_id=3&body={\"version\": \"1.0\",\"method\": \"elect_dump\",\"params\":{\"account\":\"T-123456789012345678901234567890123\"}}";
    int16_t count = 0;
    client.request("POST", "/", json_string, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
        if (!ec)
        {
            ++count;
            string result = response->content.string();
            cout << result << endl;
            EXPECT_EQ(true, result.find("errno") != string::npos);
        }
    });
    std::thread io_thread([&client] {
        client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
    });
    io_thread.detach();
    usleep(2001000);
    EXPECT_EQ(1, count);

    //cout <<"timeout:" << r1->content.rdbuf() << endl; // Alternatively, use the convenience function r1->content.string()
    //string result = r1->content.string();
 //   EXPECT_EQ(true, result.find("99") != string::npos);
}

TEST(xrpc_server_api_test, transfer_two_shard)
{
    HttpClient client("localhost:19527");
    string json_string = "account_address=T-123456789012345678901234567890123&token="+account_token+"&method=transfer&sequence_id=4&body={\"version\": \"1.0\",\"method\": \"transfer\", \"params\":{\"from\":\"T-123456789012345678901234567890123\", \"to\":\"T-123456789012345678901234567890124\",\"amount\":10,\"nonce\":0,\"last_hash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\"}}";
    int16_t count = 0;
    client.request("POST", "/", json_string, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
        if (!ec)
        {
            ++count;
            string result = response->content.string();
            cout << result << endl;
            EXPECT_EQ(true, result.find("errno") != string::npos);
        }
    });
    std::thread io_thread([&client] {
        client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
    });
    io_thread.detach();
    usleep(2001000);
    EXPECT_EQ(1, count);
}

TEST(xrpc_server_api_test, transfer_one_shard)
{
    HttpClient client("localhost:19527");
    string json_string = "account_address=T-123456789012345678901234567890123&token="+account_token+"&method=transfer&sequence_id=5&body={\"version\": \"1.0\",\"method\": \"transfer\", \"params\":{\"from\":\"T-123456789012345678901234567890123\", \"to\":\"T-123456789012345678901234567890124\",\"amount\":10,\"nonce\":0,\"last_hash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\"}}";
    int16_t count = 0;
    client.request("POST", "/", json_string, [&count](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
        if (!ec)
        {
            ++count;
            string result = response->content.string();
            cout << result << endl;
            EXPECT_EQ(true, result.find("errno") != string::npos);
        }
    });
    std::thread io_thread([&client] {
        client.io_service->run_until(chrono::steady_clock::now() + std::chrono::seconds(2));
    });
    io_thread.detach();
    usleep(2001000);
    EXPECT_EQ(1, count);
}

TEST(xrpc_server_api_test, exit)
{
    delete g_server_api_test;
    EXPECT_EQ(0, 0);
    // g_server_api_test.m_http_server_ptr->m_server_thread.detach();
}*/
// NS_END3
