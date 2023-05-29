#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xfixed_hash.h"
#include "xbasic/xhex.h"
#include "xblockmaker/xtable_cross_plugin.h"
#include "xcommon/common.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xblockextract.h"

using namespace top;
using namespace top::base;
using namespace top::data;


class test_xtable_cross_plugin : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


// string key is u256.str()
// map<chainid_bits, gas_price)
using cross_chain_contract_gasprice_info = std::map<std::string, evm_common::u256>;
//<contract_address, map<topics, pair<speed_type, chainid_bits>>>
using cross_chain_contract_info = std::map<std::string, std::map<std::string, std::pair<uint32_t, evm_common::u256>>>;


static cross_chain_contract_info get_cross_chain_config() {
    auto cross_chain_config_str = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_chain_contract_tx_list);
    std::vector<std::string> str_vec;
    base::xstring_utl::split_string(cross_chain_config_str, ',', str_vec);
    if (str_vec.size() <= 0) {
        return {};
    }

    cross_chain_contract_info cross_chain_config;
    for (auto & str : str_vec) {
        std::vector<std::string> config_str_vec;
        base::xstring_utl::split_string(str, ':', config_str_vec);
        if (config_str_vec.size() != 4) {
            xerror("get_cross_chain_config cross_chain_contract_tx_list invalid:%s", cross_chain_config_str.c_str());
            return {};
        }

        std::string & addr = config_str_vec[0];
        std::string & topic = config_str_vec[1];
        uint32_t tx_seppd_type = static_cast<std::uint32_t>(std::stoi(config_str_vec[2]));
        evm_common::u256 chain_bit = evm_common::u256(config_str_vec[3]);
        cross_chain_config[addr][topic] =  std::make_pair(tx_seppd_type,chain_bit);
    }

    if (str_vec.size() != cross_chain_config.size()) {
        xerror("get_cross_chain_config repeat addresses in cross_chain_contract_tx_list:%s", cross_chain_config_str.c_str());
    }
    return cross_chain_config;
}

static cross_chain_contract_gasprice_info get_cross_chain_gasprice_config() {
    auto cross_chain_gasprice_config_str = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_chain_gasprice_list);
    std::vector<std::string> str_vec;
    base::xstring_utl::split_string(cross_chain_gasprice_config_str, ',', str_vec);
    if (str_vec.size() <= 0) {
        return {};
    }
    cross_chain_contract_gasprice_info cross_chain_gasprices_config;
    for (auto& str : str_vec) {
        std::vector<std::string> config_str_vec;
        base::xstring_utl::split_string(str, ':', config_str_vec);
        if (config_str_vec.size() != 2) {
            xerror("get_cross_chain_gasprice_config cross_chain_contract_tx_list invalid:%s", cross_chain_gasprice_config_str.c_str());
            return {};
        }
        evm_common::u256 chain_bit = evm_common::u256(config_str_vec[0]);
        evm_common::u256 gasprice =  evm_common::u256(config_str_vec[1]);
        cross_chain_gasprices_config[chain_bit.str()] = gasprice;
    }

    if (str_vec.size() != cross_chain_gasprices_config.size()) {
        xerror("get_cross_chain_gasprice_config repeat addresses in cross_chain_contract_tx_list:%s", cross_chain_gasprice_config_str.c_str());
    }
    return cross_chain_gasprices_config;
}


static bool cross_tx_info_check_and_get(const evm_common::xevm_logs_t& logs, const cross_chain_contract_info& cross_chain_config,
                                        uint32_t& speed_type, evm_common::u256& chain_bit) {
    for (auto& log : logs) {
        auto it = cross_chain_config.find(log.address.to_hex_string());
        if (it == cross_chain_config.end()) {
            continue;
        }

        std::string topic_hex = top::to_hex_prefixed(log.topics[0].asBytes());
        auto it2 = it->second.find(topic_hex);
        if (it2 != it->second.end()) {
            speed_type = it2->second.first;
            chain_bit = it2->second.second;
            return true;
        }
    }
    return false;
}


TEST_F(test_xtable_cross_plugin, cross_chain_config) {
    std::error_code ec;
    std::string cross_addr = "0xbc9b5f068bc20a5b12030fcb72975d8bddc4e84c";
    std::string cross_topic_str = "0x342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735";
    xh256_t cross_topic = xh256_t(top::from_hex(cross_topic_str, ec));

    std::string cross_addr2 = "0xbc9b5f068bc20a5b12030fcb72975d8bddc4e841";
    std::string cross_topic_str2 = "0x342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a732";
    xh256_t cross_topic2 = xh256_t(top::from_hex(cross_topic_str2, ec));

    //170 is 0xAA
    std::string cross_config_addr = cross_addr + ":" + cross_topic_str + ":0:1," + cross_addr2 + ":" + cross_topic_str2 + ":1:170";
    top::config::config_register.get_instance().set(config::xcross_chain_contract_tx_list_onchain_goverance_parameter_t::name, cross_config_addr);

    auto cross_chain_config = get_cross_chain_config();
    EXPECT_EQ(cross_chain_config.size(), 2);

    evm_common::xevm_logs_t logs;
    uint32_t speed_type = 0;
    evm_common::u256 chain_bits{0};
    bool ret = cross_tx_info_check_and_get(logs, cross_chain_config, speed_type, chain_bits);
    EXPECT_EQ(ret, false);
 
    evm_common::xevm_log_t log;
    log.address = common::xtop_eth_address::build_from(cross_addr);
    log.topics.push_back(cross_topic);
    logs.push_back(log);

    ret =cross_tx_info_check_and_get(logs, cross_chain_config, speed_type, chain_bits);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(speed_type, 0);
    EXPECT_EQ(chain_bits.str(), "1");
    EXPECT_EQ(chain_bits, 1);

    evm_common::xevm_log_t log2;
    log2.address = common::xtop_eth_address::build_from(cross_addr2);
    log2.topics.push_back(cross_topic2);
    evm_common::xevm_logs_t logs2;
    logs2.push_back(log2);

    ret = cross_tx_info_check_and_get(logs2, cross_chain_config, speed_type, chain_bits);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(speed_type, 1);
    EXPECT_EQ(chain_bits.str(), "170");
    EXPECT_EQ(chain_bits, 0xAA);

    evm_common::xevm_log_t log3;
    evm_common::xevm_logs_t logs3;
    logs3.push_back(log3);
    logs3.push_back(log);

    evm_common::u256 chain_bits3;
    ret = cross_tx_info_check_and_get(logs3, cross_chain_config, speed_type, chain_bits);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(speed_type, 0);
    EXPECT_EQ(chain_bits.str(), "1");
    EXPECT_EQ(chain_bits, 1);
}


TEST_F(test_xtable_cross_plugin, cross_chain_gasprice_config) {

    std::string cross_config_gasprice_addr = "1:100,43690:1000";  //0xAAAA:1000
    top::config::config_register.get_instance().set(config::xcross_chain_gasprice_list_onchain_goverance_parameter_t::name, cross_config_gasprice_addr);

    auto cross_chain_config = get_cross_chain_gasprice_config();
    EXPECT_EQ(cross_chain_config.size(), 2);

    evm_common::u256 gasprice = 0;
    evm_common::u256 chain_bit_1 = 100;
    auto iter_1 = cross_chain_config.find(chain_bit_1.str());
    if (iter_1 != cross_chain_config.end()) {
        gasprice = iter_1->second;
    }

    EXPECT_EQ(gasprice, 0);

    evm_common::u256 chain_bit_2 = 1;
    auto iter_2 = cross_chain_config.find(chain_bit_2.str());
    if (iter_2 != cross_chain_config.end()) {
        gasprice = iter_2->second;
    }

    EXPECT_EQ(gasprice, 100);

    evm_common::u256 chain_bit_3 = 0xAAAA;
    auto iter_3 = cross_chain_config.find(chain_bit_3.str());
    if (iter_3 != cross_chain_config.end()) {
        gasprice = iter_3->second;
    }

    EXPECT_EQ(gasprice, 1000);
}
