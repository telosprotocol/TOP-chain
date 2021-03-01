#include "gtest/gtest.h"
#include "xchain_timer/xchain_timer.h"
#include "xbase/xutl.h"
// #include <functional>

namespace top {
class test_chain_timer : public testing::Test {
 protected:
    void SetUp() override { }

    void TearDown() override {
    }
 public:
};

TEST_F(test_chain_timer, update) {
    // time::xchain_timer_t timer;
    // timer.init();
    // time::xchain_time_st st;
    // // st.beacon_height = 1;
    // // st.beacon_round = 1;
    // st.xtime_round = 1;
    // // st.xtimestamp = base::xtime_utl::gmttime_ms();
    // auto ret = timer.update_time(st.xtime_round);
    // EXPECT_TRUE(ret) << "update time succ";

    // auto t = timer.get_local_time();
    // EXPECT_EQ(t.xtime_round, st.xtime_round) << "time round equals";
    // timer.close();
    // sleep(2);
}

// time::xchain_time_st g_timer;
// time::xchain_time_st g_timer2;

// void on_1_timer(const time::xchain_time_st & st) {
//     // g_timer.beacon_round = st.beacon_round;
//     // g_timer.beacon_height = st.beacon_height;
//     // g_timer.xtimestamp = st.xtimestamp;
//     g_timer.xtime_round = st.xtime_round;
//     g_timer.local_update_time = base::xtime_utl::gmttime_ms();
// }

// void on_2_timer(const time::xchain_time_st & st) {
//     // g_timer2.beacon_round = st.beacon_round;
//     // g_timer2.beacon_height = st.beacon_height;
//     // g_timer2.xtimestamp = st.xtimestamp;
//     g_timer2.xtime_round = st.xtime_round;
//     g_timer2.local_update_time = base::xtime_utl::gmttime_ms();
// }

TEST_F(test_chain_timer, notify) {
    // time::xchain_timer_t timer;
    // timer.init();
    // time::xchain_time_st st;
    // timer.watch("test", 1, std::bind(&on_1_timer, std::placeholders::_1));
    // timer.watch("test_2", 2, std::bind(&on_2_timer, std::placeholders::_1));
    // // st.beacon_height = 1;
    // // st.beacon_round = 1;
    // st.xtime_round = 1;
    // // st.xtimestamp = base::xtime_utl::gmttime_ms();
    // auto ret = timer.update_time(st.xtime_round);
    // EXPECT_TRUE(ret) << "update time succ";
    // sleep(2);
    // EXPECT_EQ(g_timer.xtime_round, st.xtime_round) << "time notify equals";
    // EXPECT_TRUE(g_timer2.xtime_round != st.xtime_round) << "time notify not equals";
    // ret = timer.unwatch("test");
    // EXPECT_TRUE(ret) << "update uwatch succ";
    // // st.beacon_height = 2;
    // // st.beacon_round = 2;
    // st.xtime_round = 2;
    // // st.xtimestamp = base::xtime_utl::gmttime_ms();
    // ret = timer.update_time(st.xtime_round);
    // EXPECT_TRUE(ret) << "update time succ";
    // sleep(1);
    // EXPECT_EQ(g_timer2.xtime_round, st.xtime_round) << "time notify equals";
    // EXPECT_TRUE(g_timer.xtime_round != st.xtime_round) << "time notify not equals";
}

}
