#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xblockextract.h"
#include "xevm_common/common.h"
#include "xevm_common/common_data.h"
#include "xevm_common/fixed_hash.h"
#include "tests/xdata_test/test_eth.hpp"

using namespace top;
using namespace top::base;
using namespace top::data;

class test_block_extract : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_block_extract, cross_chain_config) {
    std::error_code ec;
    std::string cross_addr = "0xbc9b5f068bc20a5b12030fcb72975d8bddc4e84c";
    std::string cross_topic_str = "0x342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735";
    evm_common::xh256_t cross_topic = evm_common::xh256_t(top::from_hex(cross_topic_str, ec));

    std::string cross_addr2 = "0xbc9b5f068bc20a5b12030fcb72975d8bddc4e841";
    std::string cross_topic_str2 = "0x342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a732";
    evm_common::xh256_t cross_topic2 = evm_common::xh256_t(top::from_hex(cross_topic_str2, ec));

    std::string cross_config_addr = cross_addr + ":" + cross_topic_str + ":0," + cross_addr2 + ":" + cross_topic_str2 + ":1";
    top::config::config_register.get_instance().set(config::xcross_chain_contract_list_onchain_goverance_parameter_t::name, cross_config_addr);

    auto cross_chain_config = data::xblockextract_t::get_cross_chain_config();
    EXPECT_EQ(cross_chain_config.size(), 2);

    evm_common::xevm_logs_t logs;

    bool ret = data::xblockextract_t::is_cross_tx(logs, cross_chain_config);
    EXPECT_EQ(ret, false);

    evm_common::xevm_log_t log;
    log.address = common::xtop_eth_address::build_from(cross_addr);
    log.topics.push_back(cross_topic);
    logs.push_back(log);

    ret = data::xblockextract_t::is_cross_tx(logs, cross_chain_config);
    EXPECT_EQ(ret, true);

    evm_common::u256 chain_bits;
    ret = data::xblockextract_t::get_chain_bits(logs, cross_chain_config, chain_bits);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(chain_bits, 1);

    evm_common::xevm_log_t log2;
    log2.address = common::xtop_eth_address::build_from(cross_addr2);
    log2.topics.push_back(cross_topic2);
    evm_common::xevm_logs_t logs2;
    logs2.push_back(log2);

    ret = data::xblockextract_t::is_cross_tx(logs2, cross_chain_config);
    xassert(ret == true);
    EXPECT_EQ(ret, true);

    evm_common::u256 chain_bits2;
    ret = data::xblockextract_t::get_chain_bits(logs2, cross_chain_config, chain_bits2);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(chain_bits2, 2);

    evm_common::xevm_log_t log3;
    evm_common::xevm_logs_t logs3;
    logs3.push_back(log3);
    logs3.push_back(log);

    ret = data::xblockextract_t::is_cross_tx(logs3, cross_chain_config);
    EXPECT_EQ(ret, true);

    evm_common::u256 chain_bits3;
    ret = data::xblockextract_t::get_chain_bits(logs3, cross_chain_config, chain_bits3);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(chain_bits3, 1);
}

