#include <gtest/gtest.h>

#include "ethash/keccak.hpp"
#include "trezor-crypto/sha3.h"
#include "xbase/xcontext.h"
#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbasic/xhex.h"
#include "xcommon/common.h"
#include "xcommon/rlp.h"
#include "xcommon/xaccount_address_fwd.h"
#include "xcommon/xbloom9.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xeth_address.h"
#include "xbasic/xfixed_hash.h"
#include "xcommon/xtop_event.h"
#include "xcrypto/xckey.h"
#include "xdata/xgenesis_data.h"
#include "xevm_common/address.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xevm_transaction_result.h"

#if defined(XCXX20)
#    include <secp256k1.h>
#    include <secp256k1_recovery.h>
#else
#    include "secp256k1/secp256k1.h"
#    include "secp256k1/secp256k1_recovery.h"
#endif

#include <json/value.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace top::evm_common;
using namespace top;
using namespace std;

TEST(test_top_log, event_transfer) {
    common::arguments_t inputs;
    auto address = common::xaccount_address_t::build_from("T20000ML7oBZbitBCcXhrJwqBhha2MUimd6SM9Z6");
    std::string func_sign = R"(Transfer(address,address,uint256))";
    std::string indexed_from = R"(T20000ML7oBZbitBCcXhrJwqBhha2MUimd6SM9Z6)";
    std::string indexed_to = R"(T80000b4e7a86997ce95ebaa1f9ccf5f9f50dbfb619b75)";
    uint64_t data = 10095634316;
    inputs.emplace_back(common::argument_t(data, common::enum_event_data_type::uint64, false));
    inputs.emplace_back(common::argument_t(indexed_from, common::enum_event_data_type::address, true));
    inputs.emplace_back(common::argument_t(indexed_to, common::enum_event_data_type::address, true));
    common::xtop_event_t event(func_sign, address, inputs);
    auto log = event.pack();

    ASSERT_EQ(top::to_hex_prefixed(log.data), "0x0000000000000000000000000000000000000000000000000000000259bf278c");
    ASSERT_EQ(top::to_hex_prefixed(log.topics[0].asBytes()), "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef");
    ASSERT_EQ(top::to_hex_prefixed(log.topics[1].asBytes()), "0x000000000000000000000000724a777142686861324d55696d6436534d395a36");
    ASSERT_EQ(top::to_hex_prefixed(log.topics[2].asBytes()), "0x000000000000000000000000b4e7a86997ce95ebaa1f9ccf5f9f50dbfb619b75");
    ASSERT_EQ(log.bloom().to_hex_string(),
              "0x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000080000000000"
              "00000000000002000000000000000000000000000000000000000000400000000000000000000000000010000000000000000000000000000000000008000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000000000000000000200000040000000000000000000001000100200000000000000000000000000000000000000000000000000000000040000"
              "0000000000000000000000");
}

TEST(test_top_log, json_logs) {
    auto removeSpacesAndNewlines = [](std::string & s) { s.erase(std::remove_if(s.begin(), s.end(), [](char c) { return std::isspace(c); }), s.end()); };
    std::string target = R"(
        {"logBloom" : "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
   "logs" : []
})";

    Json::Value js;
    js["logs"] = Json::arrayValue;
    js["logBloom"] = evm_common::xbloom9_t{}.to_hex_string();

    auto src = js.toStyledString();
    removeSpacesAndNewlines(src);
    removeSpacesAndNewlines(target);
    ASSERT_EQ(src, target);
    // cout << src << endl;
}