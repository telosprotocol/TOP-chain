#include "gtest/gtest.h"
#define private public
#define protected public
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

#include "xvledger/xvtxindex.h"
#include "xrpc/xrpc_eth_parser.h"
#include "xbasic/xhex.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xeth_address.h"
#include "xcommon/xaddress.h"
#include "xdata/xethheader.h"
#include "xdata/xblockextract.h"
#include "xdata/xethreceipt.h"
#include "xdata/xtx_factory.h"
#include "xdata/xerror/xerror.h"

using namespace top;

class test_xrpc_eth_parser : public testing::Test {
 protected:
    void SetUp() override {
        top::utl::xkeyaddress_t pubkey1(""); 
    }

    void TearDown() override {
        // delete block_handle_ptr;
    }
 public:
};

TEST_F(test_xrpc_eth_parser, uint64_to_hex_prefixed) {
    std::string ret = xrpc::xrpc_eth_parser_t::uint64_to_hex_prefixed(0);
    EXPECT_EQ(ret, "0x0");
    ret = xrpc::xrpc_eth_parser_t::uint64_to_hex_prefixed(256);
    EXPECT_EQ(ret, "0x100");
    ret = xrpc::xrpc_eth_parser_t::uint64_to_hex_prefixed(255);
    EXPECT_EQ(ret, "0xff");
}
TEST_F(test_xrpc_eth_parser, u256_to_hex_prefixed) {
    evm_common::u256 i = 1;
    std::string ret = xrpc::xrpc_eth_parser_t::u256_to_hex_prefixed(i);
    EXPECT_EQ(ret, "0x1");
    i = 255;
    ret = xrpc::xrpc_eth_parser_t::u256_to_hex_prefixed(i);
    EXPECT_EQ(ret, "0xff");
    i = 256;
    ret = xrpc::xrpc_eth_parser_t::u256_to_hex_prefixed(i);
    EXPECT_EQ(ret, "0x100");
}
TEST_F(test_xrpc_eth_parser, log_to_json) {
    xrpc::xlog_location_t loglocation("", "", "", "");
    xJson::Value js_log;
    evm_common::xevm_log_t log;
    xrpc::xrpc_eth_parser_t::log_to_json(loglocation, log, js_log);
    EXPECT_EQ(js_log.empty(), false);

    log.topics.emplace_back(evm_common::xh256_t{});
    xrpc::xrpc_eth_parser_t::log_to_json(loglocation, log, js_log);
    EXPECT_EQ(js_log.empty(), false);
}
TEST_F(test_xrpc_eth_parser, receipt_to_json) {
    std::error_code ec;
    xrpc::xtx_location_t txlocation("","","","");
    data::xeth_transaction_t ethtx;
    xJson::Value js_v;
    data::xeth_store_receipt_t evm_tx_receipt;
//    top::utl::xkeyaddress_t pubkey1("");
    xrpc::xrpc_eth_parser_t::receipt_to_json(txlocation, ethtx, evm_tx_receipt, js_v, ec);
    EXPECT_EQ(js_v.empty(), false);
    
}
TEST_F(test_xrpc_eth_parser, receipt_to_json2) {
    std::error_code ec;
    xrpc::xtx_location_t txlocation("","","","");
    data::xeth_transaction_t ethtx;
    xJson::Value js_v;

    std::string raw_tx_hash;
    xrpc::xtxindex_detail_ptr_t sendindex = xrpc::xrpc_loader_t::load_ethtx_indx_detail(raw_tx_hash);
    if (sendindex == nullptr)
        return;
    xrpc::xrpc_eth_parser_t::receipt_to_json("", sendindex, js_v, ec);    
    EXPECT_EQ(js_v.empty(), false);
}
TEST_F(test_xrpc_eth_parser, transaction_to_json) {
    std::error_code ec;
    xrpc::xtx_location_t txlocation("","","","");
    data::xeth_transaction_t ethtx;
    xJson::Value js_v;

    xrpc::xrpc_eth_parser_t::transaction_to_json(txlocation, ethtx, js_v, ec);    
    EXPECT_EQ(js_v.empty(), false);

}
TEST_F(test_xrpc_eth_parser, transaction_to_json2) {
    std::error_code ec;
    xrpc::xtx_location_t txlocation("","","","");
    data::xeth_transaction_t ethtx;
    xJson::Value js_v;
    std::string tx_hash_str;
    xrpc::xtxindex_detail_ptr_t sendindex = xrpc::xrpc_loader_t::load_ethtx_indx_detail(tx_hash_str);
    if (sendindex == nullptr)
        return;

    xrpc::xrpc_eth_parser_t::transaction_to_json(txlocation, sendindex->get_raw_tx(), js_v, ec);    
    EXPECT_EQ(js_v.empty(), false);

}


