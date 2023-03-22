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

class xvnode_house_mock : public base::xvnodesrv_t {
public:
    virtual bool                       add_group(const base::xvnodegroup_t* group_ptr)   override {return false;}
    virtual bool                       remove_group(const xvip2_t & target_group)  override {return false;}
};

class test_xrpc_eth_query_manager : public testing::Test {
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
Json::Value parse_res(const std::string& res);

TEST_F(test_xrpc_eth_query_manager, illegal_request) {
    Json::Value js_req;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    std::string strRequet = "illegal";
    auto ret = xrpc_eth_query_manager_ptr->handle(strRequet, js_req, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(false, ret);
    js_rsp["result"] = strResult;
    auto jres = parse_res(js_rsp.toStyledString());
    EXPECT_EQ("Method not Found!", jres["result"].asString());
}

TEST_F(test_xrpc_eth_query_manager, query_account_by_number) {
    data::xunitstate_ptr_t account_ptr;
    auto ret = xrpc_eth_query_manager_ptr->query_account_by_number("T60004b7762d8dbd7e5c023ff99402b78af7c13b01eec1", "latest", account_ptr);
    EXPECT_EQ(ret, xrpc::enum_success);
    ret = xrpc_eth_query_manager_ptr->query_account_by_number("T60004b7762d8dbd7e5c023ff99402b78af7c13b01eec1", "earliest", account_ptr);
    EXPECT_EQ(ret, xrpc::enum_success);
    ret = xrpc_eth_query_manager_ptr->query_account_by_number("T60004b7762d8dbd7e5c023ff99402b78af7c13b01eec1", "pending", account_ptr);
    EXPECT_EQ(ret, xrpc::enum_success);
    ret = xrpc_eth_query_manager_ptr->query_account_by_number("T60004b7762d8dbd7e5c023ff99402b78af7c13b01eec1", "0x1", account_ptr);
    EXPECT_EQ(ret, xrpc::enum_block_not_found);
}
TEST_F(test_xrpc_eth_query_manager, query_block_by_height) {
    xobject_ptr_t<base::xvblock_t> block_ptr = xrpc_eth_query_manager_ptr->query_block_by_height("latest");
    EXPECT_NE(block_ptr.get(), nullptr);
    block_ptr = xrpc_eth_query_manager_ptr->query_block_by_height("earliest");
    EXPECT_NE(block_ptr.get(), nullptr);
    block_ptr = xrpc_eth_query_manager_ptr->query_block_by_height("pending");
    EXPECT_NE(block_ptr.get(), nullptr);
    block_ptr = xrpc_eth_query_manager_ptr->query_block_by_height("0x1");
    EXPECT_EQ(block_ptr.get(), nullptr);
}
TEST_F(test_xrpc_eth_query_manager, get_block_height) {
    uint64_t height = xrpc_eth_query_manager_ptr->get_block_height("latest");
    EXPECT_EQ(height, 0);
    height = xrpc_eth_query_manager_ptr->get_block_height("earliest");
    EXPECT_EQ(height, 0);
    height = xrpc_eth_query_manager_ptr->get_block_height("pending");
    EXPECT_EQ(height, 0);
    height = xrpc_eth_query_manager_ptr->get_block_height("0x100");
    EXPECT_EQ(height, 0);
}
TEST_F(test_xrpc_eth_query_manager, eth_getBalance) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["id"] = "12345678";
    jr["jsonrpc"] = "2.0";
    jr["action"] = "eth_getBalance";
    jr["params"].append("0x83d85d169f750ad626dc10565043a802b5499a3f");
    jr["params"].append("0x1");
    std::string request = jr.toStyledString();

    auto ret = xrpc_eth_query_manager_ptr->handle(request, jr, js_rsp, strResult, nErrorCode);
    EXPECT_EQ(true, ret);
    //std::cout << js_rsp.toStyledString() <<std::endl;
}
TEST_F(test_xrpc_eth_query_manager, eth_getBalance2) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_getBalance";
        jr["params"].append("0x83d85d169f750ad626dc10565043a802b5499a3f");
        jr["params"].append("0x1");
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
        jr["method"] = "eth_getBalance";
        jr["params"].append("0x83d85d169f750ad626dc10565043a802b5499a3f");
        jr["params"].append("latest");
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
        jr["method"] = "eth_getBalance";
        jr["params"].append("0x83d85d169f750ad626dc10565043a802b5499a3f");
        jr["params"].append("earliest");
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }        
}
TEST_F(test_xrpc_eth_query_manager, eth_getTransactionCount) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_getTransactionCount";
        jr["params"].append("0x83d85d169f750ad626dc10565043a802b5499a3f");
        jr["params"].append("0x1");
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
        jr["method"] = "eth_getTransactionCount";
        jr["params"].append("0x83d85d169f750ad626dc10565043a802b5499a3f");
        jr["params"].append("earliest");
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
        jr["method"] = "eth_getTransactionCount";
        jr["params"].append("0x83d85d169f750ad626dc10565043a802b5499a3f");
        jr["params"].append("latest");
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }    
}
TEST_F(test_xrpc_eth_query_manager, eth_getTransactionByHash) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_getTransactionByHash";
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
        jr["method"] = "eth_getTransactionByHash";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append(true);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
}
TEST_F(test_xrpc_eth_query_manager, eth_getTransactionReceipt) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["id"] = "12345678";
    jr["jsonrpc"] = "2.0";
    jr["method"] = "eth_getTransactionReceipt";
    jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
    jr["params"].append(false);
    std::string request = jr.toStyledString();
    xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
    EXPECT_EQ(js_rsp.isNull(), false);
}
TEST_F(test_xrpc_eth_query_manager, eth_blockNumber) {
    Json::Value jr;
    Json::Value js_rsp;
    std::string strResult = "ok";
    uint32_t nErrorCode = 0;
    jr["id"] = "12345678";
    jr["jsonrpc"] = "2.0";
    jr["method"] = "eth_blockNumber";
    jr["params"].resize(0);
    std::string request = jr.toStyledString();
    xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
    EXPECT_EQ(js_rsp.isNull(), false);
}
TEST_F(test_xrpc_eth_query_manager, eth_getBlockByHash) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_getBlockByHash";
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
        jr["method"] = "eth_getBlockByHash";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append(true);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
}
TEST_F(test_xrpc_eth_query_manager, eth_getBlockByNumber) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_getBlockByNumber";
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
        jr["method"] = "eth_getBlockByNumber";
        jr["params"].append("0x1");
        jr["params"].append(true);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }
}
TEST_F(test_xrpc_eth_query_manager, eth_getCode) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_getCode";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append("latest");
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
        jr["method"] = "eth_getCode";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append("earliest");
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
        jr["method"] = "eth_getCode";
        jr["params"].append("0x6e8fb2788c51763d62ccf40728e4f8ce4ff2b899e6b60395d3c564fc6e50c27e");
        jr["params"].append("0x1");
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }    
}
TEST_F(test_xrpc_eth_query_manager, eth_call) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_call";
        Json::Value param;
        param["from"] = "0x83d85d169f750ad626dc10565043a802b5499a3f";
        param["to"] = "0x1C46bA26351ADC1ADB9165Fc78e240EDc02ee96A";
        param["data"] = "0x70a08231000000000000000000000000de0e9abfafffe4618e5a8fb7dd97b19434987533";
        param["gas"] = "0x53b9";
        jr["params"].append(param);
        jr["params"].append("latest");
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
        jr["method"] = "eth_call";
        Json::Value param;
        param["from"] = "0x83d85d169f750ad626dc10565043a802b5499a3f";
        param["to"] = "0x1C46bA26351ADC1ADB9165Fc78e240EDc02ee96A";
        param["data"] = "0x70a08231000000000000000000000000de0e9abfafffe4618e5a8fb7dd97b19434987533";
        param["gas"] = "0x53b9";
        jr["params"].append(param);
        jr["params"].append("earliest");
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
        jr["method"] = "eth_call";
        Json::Value param;
        param["from"] = "0x83d85d169f750ad626dc10565043a802b5499a3f";
        param["to"] = "0x1C46bA26351ADC1ADB9165Fc78e240EDc02ee96A";
        param["data"] = "0x70a08231000000000000000000000000de0e9abfafffe4618e5a8fb7dd97b19434987533";
        param["gas"] = "0x53b9";
        jr["params"].append(param);
        jr["params"].append("0x1");
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }    
}
TEST_F(test_xrpc_eth_query_manager, eth_estimateGas) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_estimateGas";
        Json::Value param;
        param["from"] = "0x83d85d169f750ad626dc10565043a802b5499a3f";
        param["to"] = "0x1C46bA26351ADC1ADB9165Fc78e240EDc02ee96A";
        param["data"] = "0x70a08231000000000000000000000000de0e9abfafffe4618e5a8fb7dd97b19434987533";
        param["gas"] = "0x53b9";
        jr["params"].append(param);
        jr["params"].append("latest");
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
        jr["method"] = "eth_estimateGas";
        Json::Value param;
        param["from"] = "0x83d85d169f750ad626dc10565043a802b5499a3f";
        param["to"] = "0x1C46bA26351ADC1ADB9165Fc78e240EDc02ee96A";
        param["data"] = "0x70a08231000000000000000000000000de0e9abfafffe4618e5a8fb7dd97b19434987533";
        param["gas"] = "0x53b9";
        jr["params"].append(param);
        jr["params"].append("earliest");
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
        jr["method"] = "eth_estimateGas";
        Json::Value param;
        param["from"] = "0x83d85d169f750ad626dc10565043a802b5499a3f";
        param["to"] = "0x1C46bA26351ADC1ADB9165Fc78e240EDc02ee96A";
        param["data"] = "0x70a08231000000000000000000000000de0e9abfafffe4618e5a8fb7dd97b19434987533";
        param["gas"] = "0x53b9";
        jr["params"].append(param);
        jr["params"].append("0x1");
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }    
}
TEST_F(test_xrpc_eth_query_manager, eth_getStorageAt) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_getStorageAt";
        jr["params"].append("0x2F6d20e1344EC580d1Af7be80351BCF1065455F4");
        jr["params"].append("0x1");
        jr["params"].append("latest");
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
        jr["method"] = "eth_getStorageAt";
        jr["params"].append("0x2F6d20e1344EC580d1Af7be80351BCF1065455F4");
        jr["params"].append("0x1");
        jr["params"].append("earliest");
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
        jr["method"] = "eth_getStorageAt";
        jr["params"].append("0x2F6d20e1344EC580d1Af7be80351BCF1065455F4");
        jr["params"].append("0x1");
        jr["params"].append("0x1");
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }    
}
TEST_F(test_xrpc_eth_query_manager, eth_getLogs) {
    {
        Json::Value jr;
        Json::Value js_rsp;
        std::string strResult = "ok";
        uint32_t nErrorCode = 0;
        jr["id"] = "12345678";
        jr["jsonrpc"] = "2.0";
        jr["method"] = "eth_getLogs";
        Json::Value param;
        param["fromBlock"] = "0x0";
        jr["params"].append(param);
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
        jr["method"] = "eth_getLogs";
        Json::Value param;
        param["fromBlock"] = "0x0";
        param["toBlock"] = "latest";
        param["topics"].append("0x342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735");
        jr["params"].append(param);
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
        jr["method"] = "eth_getLogs";
        Json::Value param;
        param["fromBlock"] = "0x0";
        param["address"].append("0x2f6d20e1344ec580d1af7be80351bcf1065455f4");
        param["topics"].append("0x342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735");
        jr["params"].append(param);
        std::string request = jr.toStyledString();
        xrpc_eth_query_manager_ptr->call_method(jr["method"].asString(), jr["params"], js_rsp, strResult, nErrorCode);
        EXPECT_EQ(js_rsp.isNull(), false);
    }    
}
TEST_F(test_xrpc_eth_query_manager, check_block_log_bloom) {
    base::xvaccount_t _table_addr("Ta0004@0");
    xobject_ptr_t<base::xvblock_t> block = m_block_store->load_block_object(_table_addr, 0, base::enum_xvblock_flag_authenticated, false);
    if (block == nullptr) {
        std::cout << "xrpc_eth_query_manager::get_log, load_block_object fail" << std::endl;
        return;
    }
    std::vector<std::set<std::string>> vTopics;
    std::set<std::string> sAddress;
    bool ret = xrpc_eth_query_manager_ptr->check_block_log_bloom(block, vTopics, sAddress);
    EXPECT_EQ(ret, true);
}
TEST_F(test_xrpc_eth_query_manager, check_log_is_match) {
    evm_common::xevm_log_t log;
    std::vector<std::set<std::string>> vTopics;
    std::set<std::string> sAddress;
    bool ret = xrpc_eth_query_manager_ptr->check_log_is_match(log, vTopics, sAddress);
    EXPECT_EQ(ret, true);
}
