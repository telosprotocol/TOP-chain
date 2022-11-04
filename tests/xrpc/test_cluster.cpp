#include "gtest/gtest.h"
#include "xblockstore/xblockstore_face.h"
#include "xrpc/xrpc_query_manager.h"
#include "xrpc/xrpc_init.h"

#include "tests/xvnetwork/xdummy_vnetwork_driver.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::xrpc;
using namespace top::store;

class test_cluster : public testing::Test {
protected:
    void SetUp() override {
        m_vhost = std::make_shared<top::tests::vnetwork::xdummy_vnetwork_driver_t>();
        m_router_ptr = top::make_observer<router::xrouter_t>(new router::xrouter_t);
        m_thread = top::make_observer(base::xiothread_t::create_thread(base::xcontext_t::instance(), 0, -1));
        mock::xvchain_creator creator;
        m_store = make_observer<store::xstore_face_t>(creator.get_xstore());
        m_block_store = make_observer<base::xvblockstore_t>(creator.get_blockstore());
        m_txstore = make_observer<base::xvtxstore_t>(creator.get_txstore());
    }

    void TearDown() override {}

public:
    std::shared_ptr<top::tests::vnetwork::xdummy_vnetwork_driver_t> m_vhost;
    observer_ptr<xrouter_face_t> m_router_ptr{nullptr};
    xtxpool_service_v2::xtxpool_proxy_face_ptr m_unit_service;
    observer_ptr<store::xstore_face_t> m_store;
    observer_ptr<base::xvblockstore_t> m_block_store;
    observer_ptr<base::xvtxstore_t> m_txstore;
    std::string m_account{"T00000LdD549VCMVVzS2m2RCgkT9errUXdSjJZbF"};
    std::string m_null_account{"T00000LdD549VCMVVzS2m2RCgkT9errUXdSjJZbb"};
    observer_ptr<top::base::xiothread_t> m_thread;
};

TEST_F(test_cluster, basic) {
    auto m_cluster_handler = std::make_shared<xrpc_handler>(m_vhost, m_router_ptr, nullptr, nullptr, nullptr, m_thread);
    xvnode_address_t edge_sender;
    xmessage_t message;
    m_cluster_handler->on_message(edge_sender, message);

    xrpc_msg_request_t edge_message;
    edge_message.m_tx_type = enum_xrpc_tx_type::enum_xrpc_query_type;
    m_cluster_handler->cluster_process_query_request(edge_message, edge_sender, message);

    int32_t cnt{0};
    try {
        m_cluster_handler->cluster_process_response(message, edge_sender);
    } catch (...) {
        cnt++;
    }

    EXPECT_EQ(1, cnt);
}

#if 0
TEST_F(test_cluster, methods) {
    int32_t cnt{0};

    // illegal account query
    xjson_proc_t json_proc;
    json_proc.m_request_json["params"]["account_addr"] = "account_addr";
    auto cluster_method_manager = std::make_shared<xrpc_query_manager>(m_store, m_block_store, m_txstore, m_unit_service);
    try {
        cluster_method_manager->getAccount(json_proc);
    } catch (xrpc_error & e) {
        EXPECT_EQ(string("account not found on chain"), string(e.what()));
        cnt++;
    }
    EXPECT_EQ(1, cnt);
    // legal account query
    json_proc.m_request_json["params"]["account_addr"] = m_account;
    try {
        cluster_method_manager->getAccount(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(2, cnt);

    // illegal query by tx hash
    json_proc.m_request_json["params"]["tx_hash"] = "0x7ae7bbe2f230f0f116512d0092464e023600da585552280e04640dc8d55cf98a";
    try {
        cluster_method_manager->getTransaction(json_proc);
    } catch (xrpc_error & e) {
        EXPECT_EQ(string("account address or transaction hash error/does not exist"), string(e.what()));
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    // query property
    json_proc.m_request_json["params"]["type"] = "string";
    json_proc.m_request_json["params"]["data"] = "@30";
    try {
        cluster_method_manager->get_property(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    // query block
    json_proc.m_request_json["params"]["account_addr"] = m_null_account;
    json_proc.m_request_json["params"]["height"] = "latest";
    try {
        cluster_method_manager->getBlock(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    json_proc.m_request_json["params"]["account_addr"] = m_account;
    json_proc.m_request_json["params"]["height"] = 5;
    try {
        cluster_method_manager->getBlock(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    try {
        cluster_method_manager->getChainInfo(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    try {
        cluster_method_manager->queryNodeInfo(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    try {
        cluster_method_manager->queryNodeReward(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    try {
        cluster_method_manager->listVoteUsed(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    try {
        cluster_method_manager->queryVoterDividend(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);

    try {
        cluster_method_manager->queryProposal(json_proc);
    } catch (xrpc_error & e) {
        cnt++;
    }
    EXPECT_EQ(3, cnt);
}
#endif
