#include "gtest/gtest.h"

#include <stdio.h>
#include <string>
#include <fstream>
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xverifier/xwhitelist_verifier.h"
#include "xdata/xverifier/xverifier_utl.h"
#include <json/json.h>

using namespace top;
using namespace top::config;

class test_whitelist : public testing::Test {
public:
    void  clear_white_config() {
        std::string empty_config;
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist, empty_config);
        XSET_CONFIG(local_whitelist, empty_config);
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist, false);
        XSET_CONFIG(local_toggle_whitelist, false);
    }
    void enable_toggle_whitelist(bool enable, bool is_local) {
        if (is_local) {
            XSET_CONFIG(local_toggle_whitelist, enable);
        } else {
            XSET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist, enable);
        }
    }
    void add_onchain_white_config(std::string const& addr) {
        std::string config = XGET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);
        if (config.empty())
            config = addr;
        else
            config += "," + addr;
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist, config);
    }
    void add_offchain_white_config(std::string const& addr) {
        std::string config = XGET_CONFIG(local_whitelist);
        if (config.empty())
            config = addr;
        else
            config += "," + addr;
        XSET_CONFIG(local_whitelist, config);
    }    
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(test_whitelist, config_set_get_error_config) {
    clear_white_config();
    std::string err_config = "T8000098e61050e7fb920ab57a441722cbb5fb161b99a5:T80000a3f71fd99641294271047dd8753919781f0e6e7e";
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist, err_config);
    ASSERT_EQ(1, xverifier::xwhitelist_utl::whitelist_config().size());

    clear_white_config();
    XSET_CONFIG(local_whitelist, err_config);
    ASSERT_EQ(1, xverifier::xwhitelist_utl::whitelist_config().size());
}

TEST_F(test_whitelist, config_set_get_some_addrs) {
    clear_white_config();
    for (uint32_t i=0;i<100;i++) {
        std::string config = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_onchain_white_config(config);        
    }
    ASSERT_EQ(100, xverifier::xwhitelist_utl::whitelist_config().size());

    // add 100 repeat addrs
    for (uint32_t i=0;i<100;i++) {
        std::string config = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_offchain_white_config(config);        
    }
    ASSERT_EQ(100, xverifier::xwhitelist_utl::whitelist_config().size());

    // add 100 new addrs
    for (uint32_t i=0;i<100;i++) {
        std::string config = "T8000098e61050e7fb920ab57a441722cbb5fb161b88" + std::to_string(i);
        add_offchain_white_config(config);        
    }
    ASSERT_EQ(200, xverifier::xwhitelist_utl::whitelist_config().size());    
}

TEST_F(test_whitelist, is_white_address_limit_toggle) {
    clear_white_config();
    enable_toggle_whitelist(true, true);

    std::vector<std::string> addrs;
    for (uint32_t i=0;i<100;i++) {
        std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_onchain_white_config(addr);        
        addrs.push_back(addr);
    }
    for (auto & addr : addrs) {
        ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    }

    for (uint32_t i=0;i<100;i++) {
        std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b88" + std::to_string(i);
        add_offchain_white_config(addr);        
        addrs.push_back(addr);
    }
    for (auto & addr : addrs) {
        ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    }    

    std::vector<std::string> addrs2;
    for (uint32_t i=0;i<100;i++) {
        std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b77" + std::to_string(i);
        addrs2.push_back(addr);
    }
    for (auto & addr : addrs2) {
        ASSERT_TRUE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    }    

    enable_toggle_whitelist(false, true);
    for (auto & addr : addrs) {
        ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    }    
    for (auto & addr : addrs2) {
        ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    }        
}

TEST_F(test_whitelist, is_white_address_limit_basic) {
    clear_white_config();
{
    std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b990";
    std::string addr2 = "T8000098e61050e7fb920ab57a441722cbb5fb161b991";
    add_onchain_white_config(addr);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));
    enable_toggle_whitelist(true, false);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_TRUE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));
    enable_toggle_whitelist(false, false);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));    
    enable_toggle_whitelist(true, true);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_TRUE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));    
    enable_toggle_whitelist(false, true);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));    
}

    clear_white_config();
{
    std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b990";
    std::string addr2 = "T8000098e61050e7fb920ab57a441722cbb5fb161b991";
    std::string addr1 = "T8000098e61050e7fb920ab57a441722cbb5fb161b992";
    add_onchain_white_config(addr);
    add_onchain_white_config(addr1);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));
    enable_toggle_whitelist(true, false);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_TRUE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));
    enable_toggle_whitelist(false, false);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));    
    enable_toggle_whitelist(true, true);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_TRUE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));    
    enable_toggle_whitelist(false, true);
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr));
    ASSERT_FALSE(xverifier::xwhitelist_utl::is_white_address_limit(addr2));    
}
}
