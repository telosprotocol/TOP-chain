#include "gtest/gtest.h"
#define private public
#define protected public
#include "xblockstore/xblockstore_face.h"

#include "xrpc/xrpc_eth_query_manager.h"
#include "xvm/manager/xcontract_manager.h"
#include "xelection/xvnode_house.h"
#include "xdata/xunit_bstate.h"

#include "xdata/xnative_contract_address.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;

class test_xrpc_top_relay_query_manager : public testing::Test {
 protected:
    void SetUp() override {
        m_store = creator.get_xstore();
        m_block_store = creator.get_blockstore();
        xrpc_eth_query_manager_ptr = new xrpc::xrpc_eth_query_manager(make_observer(m_block_store));
        contract::xcontract_manager_t::instance().init(nullptr);
    }

    void TearDown() override {
        // delete block_handle_ptr;
    }
 public:
    mock::xvchain_creator creator{true};
    xobject_ptr_t<store::xstore_face_t> m_store;
    base::xvblockstore_t* m_block_store;
    xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr;
    xrpc::xrpc_eth_query_manager* xrpc_eth_query_manager_ptr;
    std::string m_account{"T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30"};
    std::string m_null_account{"T00000LdD549VCMVVzS2m2RCgkT9errUXdSjJZbb"};
};

TEST_F(test_xrpc_top_relay_query_manager, topRelay_getBlockByHash) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "topRelay_getBlockByHash";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append(false);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "topRelay_getBlockByHash";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append(true);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
}

TEST_F(test_xrpc_top_relay_query_manager, topRelay_getBlockByNumber) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "topRelay_getBlockByNumber";
        jr["params"].append("0x1");
        jr["params"].append(false);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "topRelay_getBlockByNumber";
        jr["params"].append("0x1");
        jr["params"].append(true);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
}
TEST_F(test_xrpc_top_relay_query_manager, topRelay_blockNumber) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["id"] = "12345678";
    jr["jsonrpc"] = "2.0";
    jr["method"] = "topRelay_blockNumber";
    jr["params"].resize(0);
    std::string request = jr.toStyledString();
    xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
    EXPECT_EQ(js_rsp.isNull(), false);
}
TEST_F(test_xrpc_top_relay_query_manager, topRelay_getTransactionByHash) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "topRelay_getTransactionByHash";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append(false);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "topRelay_getTransactionByHash";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append(true);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
}
TEST_F(test_xrpc_top_relay_query_manager, topRelay_getTransactionReceipt) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["id"] = "12345678";
    jr["jsonrpc"] = "2.0";
    jr["method"] = "topRelay_getTransactionReceipt";
    jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
    jr["params"].append(false);
    std::string request = jr.toStyledString();
    xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
    EXPECT_EQ(js_rsp.isNull(), false);
}
TEST_F(test_xrpc_top_relay_query_manager, set_relay_block_result) {
    base::xvaccount_t _table_addr("Tb0005@0");
    std::cout << "0" << std::endl;
    xobject_ptr_t<base::xvblock_t> block = m_block_store->load_block_object(_table_addr, 0, base::enum_xvblock_flag_authenticated, false);
    if (block == nullptr) {
        std::cout << "xrpc_eth_query_manager, load_block_object fail" << std::endl;
        return;
    }
    std::cout << "1" << std::endl;
    {
        Json::Value js_rsp;
        std::string blocklist_type;
        int ret = xrpc_eth_query_manager_ptr->set_relay_block_result(block, js_rsp, 0, blocklist_type);
        EXPECT_EQ(ret, 0);
    }
    std::cout << "2" << std::endl;
    {
        Json::Value js_rsp;
        std::string blocklist_type;
        int ret = xrpc_eth_query_manager_ptr->set_relay_block_result(block, js_rsp, 1, blocklist_type);
        EXPECT_EQ(ret, 0);
    }
    std::cout << "3" << std::endl;
    {
        Json::Value js_rsp;
        std::string blocklist_type;
        int ret = xrpc_eth_query_manager_ptr->set_relay_block_result(block, js_rsp, 2, blocklist_type);
        EXPECT_EQ(ret, 0);
    }
}
TEST_F(test_xrpc_top_relay_query_manager, query_relay_block_by_height) {
    xobject_ptr_t<base::xvblock_t> block_ptr = xrpc_eth_query_manager_ptr->query_relay_block_by_height("latest");
    EXPECT_NE(block_ptr.get(), nullptr);
    block_ptr = xrpc_eth_query_manager_ptr->query_relay_block_by_height("earliest");
    EXPECT_NE(block_ptr.get(), nullptr);
    block_ptr = xrpc_eth_query_manager_ptr->query_relay_block_by_height("pending");
    EXPECT_NE(block_ptr.get(), nullptr);
    block_ptr = xrpc_eth_query_manager_ptr->query_relay_block_by_height("0x1");
    EXPECT_EQ(block_ptr.get(), nullptr);
}