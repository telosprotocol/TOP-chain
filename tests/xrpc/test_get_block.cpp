#include "gtest/gtest.h"
#include "xblockstore/xblockstore_face.h"
#include "xstore/xstore_face.h"
#include "xrpc/xgetblock/get_block.h"
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

class test_get_block : public testing::Test {
 protected:
    void SetUp() override {
        mock::xvchain_creator creator;
        m_store = creator.get_xstore();
        m_block_store = creator.get_blockstore();
        block_handle_ptr = new chain_info::get_block_handle(m_store.get(), m_block_store, nullptr);

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
    xobject_ptr_t<store::xstore_face_t> m_store;
    base::xvblockstore_t* m_block_store;
    xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr;
    chain_info::get_block_handle* block_handle_ptr;
    std::string m_account{"T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30"};
    std::string m_null_account{"T00000LdD549VCMVVzS2m2RCgkT9errUXdSjJZbb"};
};

xJson::Value parse_res(const std::string& res){
    xJson::Reader reader;
    xJson::Value jv;
    if (!reader.parse(res, jv)) {
        EXPECT_EQ(1, 0);
    }
    return jv;
}

TEST_F(test_get_block, illegal_request) {
    auto ret = block_handle_ptr->handle("illegal");
    EXPECT_EQ(true, ret);
    auto res = block_handle_ptr->get_response();
    auto jres = parse_res(res);
    EXPECT_EQ("json parse error", jres["result"].asString());
}

TEST_F(test_get_block, get_latest_block) {
    xJson::Value jr;
    jr["action"] = "getBlock";
    jr["type"] = "last";
    jr["account_addr"] = m_account;
    jr["height"] = 0;
    std::string request = jr.toStyledString();

    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
    auto res = block_handle_ptr->get_response();
    auto jres = parse_res(res);
    EXPECT_EQ(m_account, jres["value"]["owner"].asString());
    EXPECT_EQ(0, jres["value"]["height"].asUInt64());

    jr["account_addr"] = m_null_account;
    request = jr.toStyledString();
    ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
    res = block_handle_ptr->get_response();
    jres = parse_res(res);
    // EXPECT_EQ(0, jres["value"].asString().size());
}

TEST_F(test_get_block, get_table_block) {
    xJson::Value jr;
    jr["action"] = "getBlock";
    jr["type"] = "last";
    jr["account_addr"] = m_account;
    std::string request = jr.toStyledString();

    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
    auto res = block_handle_ptr->get_response();
    auto jres = parse_res(res);
    // EXPECT_EQ(0, jres["value"].asString().size());
}

TEST_F(test_get_block, get_block_by_height) {
    xJson::Value jr;
    jr["action"] = "getBlock";
    jr["type"] = "height";
    jr["account_addr"] = m_null_account;
    jr["height"] = 1;
    std::string request = jr.toStyledString();

    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
}

TEST_F(test_get_block, get_property) {
    xJson::Value jr;
    jr["action"] = "getBlock";
    jr["type"] = "prop";
    jr["account_addr"] = m_account;

    jr["prop"] = "aaa";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
    auto res = block_handle_ptr->get_response();
    auto jres = parse_res(res);
    EXPECT_EQ("", jres["value"]["aaa"].asString());

    jr["prop"] = XPROPERTY_PLEDGE_VOTE_KEY;
    request = jr.toStyledString();
    ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
    res = block_handle_ptr->get_response();
    jres = parse_res(res);
    EXPECT_EQ(0, jres["value"].asString().size());
}

TEST_F(test_get_block, getGeneralInfos) {
    xJson::Value jr;
    jr["action"] = "getGeneralInfos";
    std::string request = jr.toStyledString();

    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);

    auto res = block_handle_ptr->get_response();
    xJson::Reader reader;
    xJson::Value jres;
    if (!reader.parse(res, jres)) {
        EXPECT_EQ(1, 0);
    }

    // auto shard_num = jres["value"]["shard_num"].asUInt64();
    // EXPECT_EQ(shard_num, 2);
}

TEST_F(test_get_block, get_transaction) {
    xJson::Value jr;
    jr["action"] = "getTransaction";
    jr["tx_hash"] = "0x7fcf50e425b4ac9c13268505cb3dfac32045457cc6e90500357d00c8cf85f5b9";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);

    auto res = block_handle_ptr->get_response();
    auto jres = parse_res(res);
    EXPECT_EQ(0, jres["value"].asString().size());
}

TEST_F(test_get_block, getAccount) {
    xJson::Value jr;
    jr["account_addr"] = m_account;
    jr["action"] = "getAccount";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
}

TEST_F(test_get_block, getEdges) {
    xJson::Value jr;
    jr["action"] = "getEdges";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
}

TEST_F(test_get_block, getArcs) {
    xJson::Value jr;
    jr["action"] = "getArcs";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
}

TEST_F(test_get_block, getConsensus) {
    xJson::Value jr;
    jr["action"] = "getConsensus";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
}

TEST_F(test_get_block, getStandbys) {
    xJson::Value jr;
    jr["action"] = "getStandbys";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
}

TEST_F(test_get_block, queryNodeInfo) {
    xJson::Value jr;
    jr["action"] = "queryNodeInfo";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
}

TEST_F(test_get_block, getTimerInfo) {
    xJson::Value jr;
    jr["action"] = "getTimerInfo";
    std::string request = jr.toStyledString();
    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(true, ret);
}

// TEST_F(test_get_block, get_sync) {
//     auto sync_status = std::make_shared<syncbase::xsync_status_t>();
//     std::unordered_map<std::string, syncbase::xaccount_sync_status_t> accounts;
//     syncbase::xaccount_sync_status_t account_sync_status;
//     accounts["aaa"] = account_sync_status;
//     sync_status->update_accounts(accounts);
//     auto sync = std::make_shared<sync::xsync_t>(sync_status.get());
//     auto block_handle = std::make_shared<chain_info::get_block_handle>(nullptr, sync.get());

//     xJson::Value jr;
//     jr["action"] = "get_sync_overview";
//     std::string request = jr.toStyledString();
//     auto ret = block_handle->handle(request);
//     EXPECT_EQ(true, ret);
//     auto res = block_handle->get_response();
//     xJson::Value jres = parse_res(res);
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

TEST_F(test_get_block, get_error) {
    xJson::Value jr;
    jr["action"] = "get_error";
    std::string request = jr.toStyledString();

    auto ret = block_handle_ptr->handle(request);
    EXPECT_EQ(false, ret);
}
