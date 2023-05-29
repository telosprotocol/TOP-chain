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
    Json::Value js_log;
    evm_common::xevm_log_t log;
    xrpc::xrpc_eth_parser_t::log_to_json(loglocation, log, js_log);
    EXPECT_EQ(js_log.empty(), false);

    log.topics.emplace_back(xh256_t{});
    xrpc::xrpc_eth_parser_t::log_to_json(loglocation, log, js_log);
    EXPECT_EQ(js_log.empty(), false);
}

TEST_F(test_xrpc_eth_parser, receipt_to_json) {
    std::string rawtx_bin = "0x02f8708203ff80822710822710827b0c94b7762d8dbd7e5c023ff99402b78af7c13b01eec1881bc16d674ec8000080c001a0d336694faa98f9cd69792ee30f9979f906f997d7562a0cb86d109f3476643a65a0679a7f510b12d48f9d1637884245229b7037b4f4094d1fb30e0f4f5cc77388ec";
    data::eth_error ec;
    data::xeth_transaction_t ethtx = data::xeth_transaction_t::build_from(rawtx_bin,ec);
    if (ec.error_code) {xassert(false);}
    Json::Value js_v;
    data::xeth_store_receipt_t evm_tx_receipt;
    // xtx_location_t(std::string const& blockhash, std::string const& blocknumber, std::string const& txhash, std::string const& txindex)
    xrpc::xtx_location_t txlocation("bhash","height","thash","idx");
    std::error_code ec1;
    xrpc::xrpc_eth_parser_t::receipt_to_json(txlocation, ethtx, evm_tx_receipt, js_v, ec1);
    EXPECT_EQ(js_v.empty(), false);
}

TEST_F(test_xrpc_eth_parser, receipt_to_json_for_top_rpc) {
    std::string rawtx_bin = "0x02f8708203ff80822710822710827b0c94b7762d8dbd7e5c023ff99402b78af7c13b01eec1881bc16d674ec8000080c001a0d336694faa98f9cd69792ee30f9979f906f997d7562a0cb86d109f3476643a65a0679a7f510b12d48f9d1637884245229b7037b4f4094d1fb30e0f4f5cc77388ec";
    data::eth_error ec;
    data::xeth_transaction_t ethtx = data::xeth_transaction_t::build_from(rawtx_bin,ec);
    if (ec.error_code) {xassert(false);}
    Json::Value js_v;
    data::xeth_store_receipt_t evm_tx_receipt;
    // xtx_location_t(std::string const& blockhash, std::string const& blocknumber, std::string const& txhash, std::string const& txindex)
    xrpc::xtx_location_t txlocation("bhash","height","thash","idx");
    std::error_code ec1;
    xrpc::xrpc_eth_parser_t::receipt_to_json_for_top_rpc("bhash", 1, ethtx, evm_tx_receipt, js_v, ec1);
    EXPECT_EQ(js_v["blockHash"], "bhash");
    EXPECT_EQ(js_v["transactionIndex"].asInt(), 1);
    EXPECT_EQ(js_v["type"].asInt(), 2);
    EXPECT_EQ(js_v["status"].asInt(), 0);
}


