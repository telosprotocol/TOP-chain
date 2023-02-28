#include "gtest/gtest.h"

#include <stdio.h>
#include <string>
#include <fstream>
#include "xconfig/xconfig_register.h"
#include "xdata/xverifier/xblacklist_verifier.h"
#include "xdata/xverifier/xverifier_utl.h"
#include "xconfig/xpredefined_configurations.h"

using namespace top;
using namespace top::config;

class test_blackaddr : public testing::Test {
public:
    void  clear_black_config() {
        std::string empty_config;
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist, empty_config);
        XSET_CONFIG(local_blacklist, empty_config);
    }
    void add_onchain_black_config(std::string const& addr) {
        std::string config = XGET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist);
        if (config.empty())
            config = addr;
        else
            config += "," + addr;
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist, config);
    }
    void add_offchain_black_config(std::string const& addr) {
        std::string config = XGET_CONFIG(local_blacklist);
        if (config.empty())
            config = addr;
        else
            config += "," + addr;
        XSET_CONFIG(local_blacklist, config);
    }

protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(test_blackaddr, config_set_get_empty_config) {
    clear_black_config();
    ASSERT_EQ("", XGET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist));
    ASSERT_EQ("", XGET_CONFIG(local_blacklist));
}
TEST_F(test_blackaddr, config_set_get_error_config) {
    clear_black_config();
    std::string err_config = "T8000098e61050e7fb920ab57a441722cbb5fb161b99a5:T80000a3f71fd99641294271047dd8753919781f0e6e7e";
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist, err_config);
    ASSERT_EQ(1, xverifier::xblacklist_utl_t::black_config().size());

    clear_black_config();
    XSET_CONFIG(local_blacklist, err_config);
    ASSERT_EQ(1, xverifier::xblacklist_utl_t::black_config().size());
}
TEST_F(test_blackaddr, config_set_get_one_addr) {
    clear_black_config();
    add_onchain_black_config("T8000098e61050e7fb920ab57a441722cbb5fb161b99a5");
    ASSERT_EQ(1, xverifier::xblacklist_utl_t::black_config().size());

    clear_black_config();
    add_offchain_black_config("T8000098e61050e7fb920ab57a441722cbb5fb161b99a5");
    ASSERT_EQ(1, xverifier::xblacklist_utl_t::black_config().size());
}
TEST_F(test_blackaddr, config_set_get_some_addrs) {
    clear_black_config();
    for (uint32_t i=0;i<100;i++) {
        std::string config = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_onchain_black_config(config);        
    }
    ASSERT_EQ(100, xverifier::xblacklist_utl_t::black_config().size());

    // add 100 repeat addrs
    for (uint32_t i=0;i<100;i++) {
        std::string config = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_offchain_black_config(config);        
    }
    ASSERT_EQ(100, xverifier::xblacklist_utl_t::black_config().size());

    // add 100 new addrs
    for (uint32_t i=0;i<100;i++) {
        std::string config = "T8000098e61050e7fb920ab57a441722cbb5fb161b88" + std::to_string(i);
        add_offchain_black_config(config);        
    }
    ASSERT_EQ(200, xverifier::xblacklist_utl_t::black_config().size());    
}

TEST_F(test_blackaddr, is_black_address) {
    clear_black_config();

    std::vector<std::string> addrs;
    for (uint32_t i=0;i<100;i++) {
        std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        ASSERT_FALSE(xverifier::xblacklist_utl_t::is_black_address(addr));
        add_onchain_black_config(addr);        
        addrs.push_back(addr);
        ASSERT_TRUE(xverifier::xblacklist_utl_t::is_black_address(addr));
    }
    for (auto & addr : addrs) {
        ASSERT_TRUE(xverifier::xblacklist_utl_t::is_black_address(addr));
    }

    for (uint32_t i=0;i<100;i++) {
        std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b88" + std::to_string(i);
        ASSERT_FALSE(xverifier::xblacklist_utl_t::is_black_address(addr));
        add_offchain_black_config(addr);        
        addrs.push_back(addr);
        ASSERT_TRUE(xverifier::xblacklist_utl_t::is_black_address(addr));
    }
    for (auto & addr : addrs) {
        ASSERT_TRUE(xverifier::xblacklist_utl_t::is_black_address(addr));
    }    
}


TEST_F(test_blackaddr, is_black_address_tx) {
    clear_black_config();
    add_offchain_black_config("T8000098e61050e7fb920ab57a441722cbb5fb161b99a1");
    add_offchain_black_config("T8000098e61050e7fb920ab57a441722cbb5fb161b99a2");
    add_offchain_black_config("T8000098e61050e7fb920ab57a441722cbb5fb161b99a3");
    add_offchain_black_config("T8000098e61050e7fb920ab57a441722cbb5fb161b99a4");

    ASSERT_FALSE(xverifier::xblacklist_utl_t::is_black_address("", ""));
    ASSERT_FALSE(xverifier::xblacklist_utl_t::is_black_address("T8000098e61050e7fb920ab57a441722cbb5fb161b99a8", ""));
    ASSERT_FALSE(xverifier::xblacklist_utl_t::is_black_address("","T8000098e61050e7fb920ab57a441722cbb5fb161b99a8"));
    ASSERT_TRUE(xverifier::xblacklist_utl_t::is_black_address("T8000098e61050e7fb920ab57a441722cbb5fb161b99a1",""));
    ASSERT_TRUE(xverifier::xblacklist_utl_t::is_black_address("","T8000098e61050e7fb920ab57a441722cbb5fb161b99a2"));
    ASSERT_TRUE(xverifier::xblacklist_utl_t::is_black_address("T8000098e61050e7fb920ab57a441722cbb5fb161b99a3","T8000098e61050e7fb920ab57a441722cbb5fb161b99a2"));
}

TEST_F(test_blackaddr, blackaddr_refresh) {
    clear_black_config(); 
    ASSERT_TRUE(xverifier::xtop_blacklist_utl::instance().refresh_and_get_new_addrs().empty());

    for (uint32_t i=0;i<100;i++) {
        std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_onchain_black_config(addr);        
        auto addrs = xverifier::xtop_blacklist_utl::instance().refresh_and_get_new_addrs();
        ASSERT_EQ(addrs.size(), 1);
        ASSERT_EQ(addrs[0], addr);
    }
    clear_black_config(); 
    ASSERT_TRUE(xverifier::xtop_blacklist_utl::instance().refresh_and_get_new_addrs().empty());

    for (uint32_t i=0;i<100;i++) {
        std::string addr1 = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_onchain_black_config(addr1);        
        i++;
        std::string addr2 = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_onchain_black_config(addr2);     

        auto addrs = xverifier::xtop_blacklist_utl::instance().refresh_and_get_new_addrs();
        ASSERT_EQ(addrs.size(), 2);
        ASSERT_EQ(addrs[0], addr1);
        ASSERT_EQ(addrs[1], addr2);
    }    

    //add repeat addrs
    for (uint32_t i=0;i<100;i++) {
        std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_onchain_black_config(addr);        
        ASSERT_TRUE(xverifier::xtop_blacklist_utl::instance().refresh_and_get_new_addrs().empty());
    }    
}


TEST_F(test_blackaddr, is_black_address_onchain_BENCH) {
    clear_black_config();

    std::vector<std::string> addrs;
    for (uint32_t i=0;i<100;i++) {
        std::string addr = "T8000098e61050e7fb920ab57a441722cbb5fb161b99" + std::to_string(i);
        add_onchain_black_config(addr);        
        addrs.push_back(addr);
    }

    std::string test_addr = addrs[50];
    uint64_t time_begin = base::xtime_utl::time_now_ms();

    uint32_t total_count = 100000;
    for (uint32_t i = 0; i < total_count; i++) {
        xverifier::xblacklist_utl_t::is_black_address(test_addr, "");
    }

    uint64_t time_finish = base::xtime_utl::time_now_ms();

    // before optimize total_count=100000 time_ms=5138
    std::cout << "total_count=" << total_count << " time_ms=" << time_finish - time_begin << std::endl;
}

TEST_F(test_blackaddr, bwlist_load_BENCH) {
    clear_black_config();

    // TODO should create bwlist.json in current directory
    std::string config_file = "./bwlist.json";
    std::map<std::string, std::string> result;

    ASSERT_TRUE(xverifier::xtx_utl::load_bwlist_content(config_file, result));
    ASSERT_EQ(result.size(), 3);
    {
        std::string local_toggle_whitelist = result["local_toggle_whitelist"];
        std::cout << "local_toggle_whitelist:" << local_toggle_whitelist << std::endl;       
    }

    {
        std::string local_blacklist = result["local_blacklist"];
        std::cout << "local_blacklist:" << local_blacklist << std::endl;        
        std::set<std::string> local_wl;
        xverifier::xtx_utl::parse_bwlist_config_data(local_blacklist, local_wl);
        ASSERT_EQ(local_wl.size(), 3);
    }

    {
        std::string local_whitelist = result["local_whitelist"];
        std::cout << "local_whitelist:" << local_whitelist << std::endl;
        std::set<std::string> local_wl;
        xverifier::xtx_utl::parse_bwlist_config_data(local_whitelist, local_wl);
        ASSERT_EQ(local_wl.size(), 4);    
    }
}