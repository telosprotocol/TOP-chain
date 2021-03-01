
#include "gtest/gtest.h"
#include "xdata/xchain_param.h"
#include "xconsensus/xconsensus_mgr.h"
// #include "tests/xconsensus/test_common.h"

using namespace top::data;

// using top::consensus::test::Mockxtop_dummy_vhost;

class test_xelect_consensus : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
 protected:
};

// TEST_F(test_xelect_consensus, init) {
//     Mockxtop_dummy_vhost host;
//     xelect_consensus_imp cons_imp(nullptr, consensus_adapter::get_instance());
//     EXPECT_TRUE(cons_imp.init());
// }


