#include "gtest/gtest.h"
#include "xconfig/xconfig_register.h"
#include "xbase/xutl.h"

#define private public
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"



class test_registration_contract_api: public testing::Test {
protected:
    void SetUp() override{}
    void TearDown() override{}
};


TEST_F(test_registration_contract_api, initial_creditscore) {
    using namespace top::xstake;
    const std::string INITIAL_CREDITSCORE = "initial_creditscore";
    xrec_registration_contract rec_contract(top::common::xnetwork_id_t{0});
    auto& config_center = top::config::config_register.get_instance();

    // if config item not exist
#if !defined(XCHAIN_FORKED_BY_DEFAULT)
    auto const& not_exist_initial_credit = top::config::config_register.value_or<std::string>(std::string{""}, INITIAL_CREDITSCORE);
    EXPECT_TRUE(not_exist_initial_credit.empty());
    EXPECT_EQ(top::base::xstring_utl::touint64(not_exist_initial_credit), 0);
#endif

    // config item exist
    std::string origin_initial_credit = "330000";
    config_center.set<std::string>(INITIAL_CREDITSCORE, "330000");
    auto const& exist_initial_credit = top::config::config_register.value_or<std::string>(std::string{""}, INITIAL_CREDITSCORE);
    EXPECT_EQ(exist_initial_credit, origin_initial_credit);
    EXPECT_EQ(top::base::xstring_utl::touint64(exist_initial_credit), 330000);
}