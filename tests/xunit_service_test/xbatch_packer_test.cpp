#include "gtest/gtest.h"
#include "xunit_service/xbatch_packer.h"
namespace top {
using namespace xunit_service;

class xbatch_packer_test : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

public:
};

TEST_F(xbatch_packer_test, pack_strategy) {
    uint32_t high_tps_num = 180;
    uint32_t middle_tps_num = 100;
    uint32_t low_tps_num = 50;
    xpack_strategy_t pack_strategy;

    ASSERT_EQ(pack_strategy.get_timer_interval(), 50);

    for (uint32_t i = 0; i < 5; i++) {
        pack_strategy.clear();
        uint32_t tx_num = pack_strategy.get_tx_num_threshold_first_time(1000);
        ASSERT_EQ(tx_num, high_tps_num);

        uint32_t r = (rand()%250) + 1;

        tx_num = pack_strategy.get_tx_num_threshold(1000 + 1);
        ASSERT_EQ(tx_num, high_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + r);
        ASSERT_EQ(tx_num, high_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + 250);
        ASSERT_EQ(tx_num, high_tps_num);

        tx_num = pack_strategy.get_tx_num_threshold(1000 + 251);
        ASSERT_EQ(tx_num, middle_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + 250 + r);
        ASSERT_EQ(tx_num, middle_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + 500);
        ASSERT_EQ(tx_num, middle_tps_num);

        tx_num = pack_strategy.get_tx_num_threshold(1000 + 501);
        ASSERT_EQ(tx_num, low_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + 500 + r);
        ASSERT_EQ(tx_num, low_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + 750);
        ASSERT_EQ(tx_num, low_tps_num);

        tx_num = pack_strategy.get_tx_num_threshold(1000 + 751);
        ASSERT_EQ(tx_num, 0);
        r = rand();
        tx_num = pack_strategy.get_tx_num_threshold(1000 + 750 + r);
        ASSERT_EQ(tx_num, 0);
    }
}
}  // namespace top
