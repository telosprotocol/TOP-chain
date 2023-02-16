#include "gtest/gtest.h"
#include "xblockstore/xblockstore_face.h"

#include "xrpc/xrpc_query_manager.h"
#include "xvm/manager/xcontract_manager.h"
#include "xelection/xvnode_house.h"

#include "xdata/xnative_contract_address.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;

// #define NODE_ID "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu"
// #define SIGN_KEY "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo="

class xvnode_house_mock : public base::xvnodesrv_t {
public:
    virtual bool                       add_group(const base::xvnodegroup_t* group_ptr)   override {return false;}
    virtual bool                             remove_group(const xvip2_t & target_group)  override {return false;}
};

class test_xrpc_query_manager : public testing::Test {
 protected:
    void SetUp() override {
        m_store = creator.get_xstore();
        m_block_store = creator.get_blockstore();
        xrpc_query_manager_ptr = new xrpc::xrpc_query_manager(make_observer(m_block_store), nullptr, nullptr);
        contract::xcontract_manager_t::instance().init(nullptr);

        // uint64_t count = 5;
        // mock::xdatamock_table mocktable;
        // mocktable.genrate_table_chain(count);
        // const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
        // xassert(tables.size() == count + 1);

        // std::string address = mocktable.get_account();
        // xvaccount_t account(address);

        // for (uint64_t i = 1; i < count; i++) {
        //     auto curr_block = tables[i].get();
        //     ASSERT_TRUE(m_block_store->store_block(account, curr_block));
        // }

        // m_account = mocktable.get_account();

        // xobject_ptr_t<base::xvblockstore_t> blockstore;
        // blockstore.attach(m_block_store);
        // auto m_nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(common::xnode_id_t{NODE_ID}, SIGN_KEY, blockstore, make_observer(creator.get_mbus().get()));
        // contract::xcontract_manager_t::instance().init(make_observer(m_store), xobject_ptr_t<store::xsyncvstore_t>{});
        // contract::xcontract_manager_t::set_nodesrv_ptr(m_nodesvr_ptr);
    }

    void TearDown() override {
        // delete block_handle_ptr;
    }
 public:
    mock::xvchain_creator creator{true};
    xobject_ptr_t<store::xstore_face_t> m_store;
    base::xvblockstore_t* m_block_store;
    xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr;
    xrpc::xrpc_query_manager* xrpc_query_manager_ptr;
    std::string m_account{"T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30"};
    std::string m_null_account{"T00000LdD549VCMVVzS2m2RCgkT9errUXdSjJZbb"};
};

Json::Value parse_res(const std::string& res){
    Json::Reader reader;
    Json::Value jv;
    if (!reader.parse(res, jv)) {
        EXPECT_EQ(1, 0);
    }
    return jv;
}

TEST_F(test_xrpc_query_manager, illegal_request) {
    Json::Value js_req;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    std::string strRequet = "illegal";
    auto ret = xrpc_query_manager_ptr->handle(strRequet, js_req, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(false, ret);
    js_rsp["result"] = strResult;
    auto jres = parse_res(js_rsp.toStyledString());
    EXPECT_EQ("Method not Found!", jres["result"].asString());
}

TEST_F(test_xrpc_query_manager, get_latest_block) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getBlock";
    jr["type"] = "last";
    jr["account_addr"] = m_account;
    jr["height"] = 0;
    std::string request = jr.toStyledString();

    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
    js_rsp["result"] = strResult;
    auto jres = parse_res(js_rsp.toStyledString());
    EXPECT_EQ(m_account, jres["value"]["owner"].asString());
    EXPECT_EQ(0, jres["value"]["height"].asUInt64());

    js_rsp.clear();
    strResult = "ok";
    nErrorCode = 0;

    jr["account_addr"] = m_null_account;
    request = jr.toStyledString();
    ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
    js_rsp["result"] = strResult;
    jres = parse_res(js_rsp.toStyledString());
    // EXPECT_EQ(0, jres["value"].asString().size());
}

TEST_F(test_xrpc_query_manager, get_table_block) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getBlock";
    jr["type"] = "last";
    jr["account_addr"] = m_account;
    std::string request = jr.toStyledString();
    js_rsp["result"] = strResult;
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
    js_rsp["result"] = strResult;
    auto jres = parse_res(js_rsp.toStyledString());
    // EXPECT_EQ(0, jres["value"].asString().size());
}

TEST_F(test_xrpc_query_manager, get_block_by_height) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getBlock";
    jr["type"] = "height";
    jr["account_addr"] = m_null_account;
    jr["height"] = 1;
    std::string request = jr.toStyledString();

    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, getProperty) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getProperty";
    jr["type"] = "prop";
    jr["account_addr"] = m_account;

    jr["prop"] = "aaa";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
    js_rsp["result"] = strResult;
    auto jres = parse_res(js_rsp.toStyledString());
    EXPECT_EQ("", jres["value"]["aaa"].asString());

    js_rsp.clear();
    strResult = "ok";
    nErrorCode = 0;
    jr["prop"] = top::data::XPROPERTY_PLEDGE_VOTE_KEY;
    request = jr.toStyledString();
    ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
    js_rsp["result"] = strResult;
    jres = parse_res(js_rsp.toStyledString());
    EXPECT_EQ(0, jres["value"].asString().size());
}

#if 0
TEST_F(test_xrpc_query_manager, getGeneralInfos) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getGeneralInfos";
    std::string request = jr.toStyledString();

    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);

    js_rsp["result"] = strResult;
    Json::Reader reader;
    Json::Value jres;
    if (!reader.parse(js_rsp.toStyledString(), jres)) {
        EXPECT_EQ(1, 0);
    }

    // auto shard_num = jres["value"]["shard_num"].asUInt64();
    // EXPECT_EQ(shard_num, 2);
}
#endif

TEST_F(test_xrpc_query_manager, get_transaction) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getTransaction";
    jr["tx_hash"] = "0x7fcf50e425b4ac9c13268505cb3dfac32045457cc6e90500357d00c8cf85f5b9";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);

    js_rsp["result"] = strResult;
    auto jres = parse_res(js_rsp.toStyledString());
    EXPECT_EQ(0, jres["value"].asString().size());
}

TEST_F(test_xrpc_query_manager, getAccount) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["account_addr"] = m_account;
    jr["action"] = "getAccount";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, getEdges) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getEdges";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, getArcs) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getArcs";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, getConsensus) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getConsensus";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, getStandbys) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getStandbys";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, queryNodeInfo) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "queryNodeInfo";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, getTimerInfo) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getTimerInfo";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, getGeneralInfos) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getGeneralInfos";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

TEST_F(test_xrpc_query_manager, getChainId) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "getChainId";
    std::string request = jr.toStyledString();
    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
}

// TEST_F(test_xrpc_query_manager, get_sync) {
//     auto sync_status = std::make_shared<syncbase::xsync_status_t>();
//     std::unordered_map<std::string, syncbase::xaccount_sync_status_t> accounts;
//     syncbase::xaccount_sync_status_t account_sync_status;
//     accounts["aaa"] = account_sync_status;
//     sync_status->update_accounts(accounts);
//     auto sync = std::make_shared<sync::xsync_t>(sync_status.get());
//     auto block_handle = std::make_shared<chain_info::get_block_handle>(nullptr, sync.get());

//     Json::Value jr;
//     jr["action"] = "get_sync_overview";
//     std::string request = jr.toStyledString();
//     auto ret = block_handle->handle(request);
//     EXPECT_EQ(true, ret);
//     auto res = block_handle->get_response();
//     Json::Value jres = parse_res(res);
//     EXPECT_EQ(0, jres["value"]["syncing"].asUInt64());

//     jr["action"] = "get_sync_detail_all_table";
//     request = jr.toStyledString();
//     ret = block_handle->handle(request);
//     EXPECT_EQ(true, ret);
//     res = block_handle->get_response();
//     jres = parse_res(res);
//     EXPECT_EQ(0, jres["value"]["aaa"][0].asUInt64());

//     jr["action"] = "get_sync_detail_processing_table";
//     request = jr.toStyledString();
//     ret = block_handle->handle(request);
//     EXPECT_EQ(true, ret);
//     res = block_handle->get_response();
//     jres = parse_res(res);
//     EXPECT_EQ(0, jres["value"]["aaa"][0].asUInt64());
// }

TEST_F(test_xrpc_query_manager, get_error) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["action"] = "get_error";
    std::string request = jr.toStyledString();

    auto ret = xrpc_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(false, ret);
}
