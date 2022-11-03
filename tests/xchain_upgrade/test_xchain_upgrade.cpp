#include "gtest/gtest.h"
#include "xconfig/xpredefined_configurations.h"
#include "xchain_fork/xutility.h"
#include "xconfig/xconfig_register.h"



using namespace top::chain_upgrade;

class test_chain_fork_center: public xtop_chain_fork_config_center {
public:
    void  set_chain_fork_config(xchain_fork_config_t const& config);
    void  set_local_chain_fork_config(xchain_fork_config_t const& config);
    xchain_fork_config_t  get_local_chain_fork_config() const;
    bool  is_fork_point_equal(xfork_point_t const& origin_fork_point, xfork_point_t const& fork_point);
    xchain_fork_config_t  local_chain_config;
};

void test_chain_fork_center::set_chain_fork_config(xchain_fork_config_t const& config) {
    mainnet_chain_config = config;
}

void test_chain_fork_center::set_local_chain_fork_config(xchain_fork_config_t const& config) {
    local_chain_config = config;
}

xchain_fork_config_t test_chain_fork_center::get_local_chain_fork_config() const {
    return local_chain_config;
}

class test_xchain_upgrade : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

public:

    test_chain_fork_center center;
};


bool test_chain_fork_center::is_fork_point_equal(xfork_point_t const& origin_fork_point, xfork_point_t const& fork_point) {
    return origin_fork_point.description == fork_point.description && origin_fork_point.fork_type == fork_point.fork_type &&
            origin_fork_point.point == fork_point.point;
}

#if 0
TEST_F(test_xchain_upgrade, test_config_select) {
    using namespace top::config;
    config_register.set(std::string{xchain_name_configuration_t::name}, std::string{chain_name_mainnet});
    auto mainnet_config = center.chain_fork_config();
    // ASSERT_TRUE(center.is_fork_point_equal(mainnet_config.point.value(), xfork_point_t{xtop_fork_point_type_t::logic_time, 0, "original fork point"}));

    config_register.set(std::string{xchain_name_configuration_t::name}, std::string{chain_name_testnet});
    auto testnet_config = center.chain_fork_config();
    ASSERT_FALSE(testnet_config.reward_fork_point.has_value());


}
#endif

//TEST_F(test_xchain_upgrade, test_config_center) {
//    using namespace top::config;
//    config_register.set(std::string{xchain_name_configuration_t::name}, std::string{chain_name_mainnet});
//    auto initial_config = center.chain_fork_config();
//    ASSERT_TRUE(initial_config.reward_fork_point.has_value());
//    // ASSERT_TRUE(center.is_fork_point_equal(initial_config.point.value(), xfork_point_t{xtop_fork_point_type_t::logic_time, 0, "original fork point"}));
//
//
//    xchain_fork_config_t another_chain_config{ xfork_point_t(xfork_point_type_t::logic_time, 100, "original fork point")};
//    center.set_chain_fork_config(another_chain_config);
//    auto set_config =  center.chain_fork_config();
//    ASSERT_TRUE(set_config.reward_fork_point.has_value());
//    // ASSERT_TRUE(center.is_fork_point_equal(set_config.point.value(), xfork_point_t{xtop_fork_point_type_t::logic_time, 100, "original fork point"}));
//    ASSERT_TRUE(center.is_forked(set_config.reward_fork_point, 100));
//    ASSERT_TRUE(center.is_forked(set_config.reward_fork_point, 110));
//    ASSERT_FALSE(center.is_forked(set_config.reward_fork_point, 99));
//}




