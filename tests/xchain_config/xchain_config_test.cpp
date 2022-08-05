#include "gtest/gtest.h"
#include "xchain_config/xconfig_center.h"
#include "xbase/xutl.h"
#include "xdata/xnative_contract_address.h"

namespace top {
class test_xchain_config : public testing::Test {
 protected:
    void SetUp() override { }

    void TearDown() override {
    }
 public:
};

TEST_F(test_xchain_config, test_get_table_accounts) {
    std::set<std::string> tables = config::xconfig_center::instance().get_table_accounts();
    EXPECT_NE(tables.find("Ta0000@0"), tables.end());
    EXPECT_NE(tables.find("Ta0004@0"), tables.end());
    EXPECT_NE(tables.find("Ta0005@0"), tables.end());
    EXPECT_NE(tables.find("Ta0001@0"), tables.end());
    EXPECT_EQ(tables.find("Ta0000@64"), tables.end());
    EXPECT_EQ(tables.find("Ta0001@1"), tables.end());
    EXPECT_EQ(tables.find("Ta0002@3"), tables.end());
    EXPECT_EQ(tables.find("Ta0004@1"), tables.end());
    EXPECT_EQ(tables.find("Ta0005@1"), tables.end());
}
TEST_F(test_xchain_config, test_get_table_config) {
    std::map<std::string, config::xtable_config> config = config::xconfig_center::instance().get_table_config();
    EXPECT_NE(config.find(sys_contract_beacon_table_block_addr), config.end());
    EXPECT_NE(config.find(sys_contract_zec_table_block_addr), config.end());
    EXPECT_NE(config.find(sys_contract_sharding_table_block_addr), config.end());
    EXPECT_NE(config.find(sys_contract_eth_table_block_addr), config.end());
    EXPECT_NE(config.find(sys_contract_relay_table_block_base_addr), config.end());
    EXPECT_EQ(config.find("TTTTTTTT"), config.end());

}
}