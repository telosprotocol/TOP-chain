// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>
#include <sstream>
#define private public
#define METRICS_UNIT_TEST
#ifndef ENABLE_METRICS
#define ENABLE_METRICS
#endif
#include "xmetrics/xmetrics.h"

#include <cinttypes>

#define TEST_CASE_SIZE 7
#define SLEEP_SECOND(t) std::this_thread::sleep_for(std::chrono::seconds(t))
#define SLEEP_MILLSECOND(t) std::this_thread::sleep_for(std::chrono::milliseconds(t))
#define SLEEP_NANOSECOND(t) std::this_thread::sleep_for(std::chrono::nanoseconds(t))

class metrics_test : public testing::Test {
public:
    void SetUp() {
        std::size_t test_dump_interval{2};
        XMETRICS_CONFIG_SET("dump_interval", test_dump_interval);
        top::metrics::e_metrics::get_instance().m_metrics_hub.clear();
        top::metrics::e_metrics::get_instance().start();
    }
    void TearDown() {
        SLEEP_SECOND(3);
        top::metrics::e_metrics::get_instance().stop();
    }
};
// #if 0
TEST(test_metrics, config) {
    auto & config_center = top::metrics::e_metrics_config::get_instance();
    // config_center.set("string",value);
    std::size_t get_dump_interval{};
    config_center.get("dump_interval", get_dump_interval);
    ASSERT_EQ(get_dump_interval, 300);
    get_dump_interval = 360;
    config_center.set("dump_interval", get_dump_interval);
    std::size_t new_dump_interval;
    XMETRICS_CONFIG_GET("dump_interval", new_dump_interval);
    ASSERT_EQ(new_dump_interval, 360);

    bool dump_full_unit{};
    XMETRICS_CONFIG_GET("dump_full_unit", dump_full_unit);
    ASSERT_EQ(dump_full_unit, true);
    dump_full_unit = false;
    XMETRICS_CONFIG_SET("dump_full_unit", dump_full_unit);
    bool new_dump_opt;
    XMETRICS_CONFIG_GET("dump_full_unit", new_dump_opt);
    ASSERT_EQ(new_dump_opt, false);
}

TEST_F(metrics_test, basic_function) {
    SLEEP_SECOND(1);
    std::size_t test_basic_size = 100;
    for (std::size_t index = 0; index < test_basic_size; ++index) {
        XMETRICS_ARRCNT_SET(top::metrics::xmetrics_array_tag_t::blockstore_sharding_table_block_commit, rand() % 64, 999);
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
        XMETRICS_COUNTER_INCREMENT("test_count_metrics2", static_cast<uint64_t>((100 + rand()) % 10000));  
        auto fork_tag = "test_error_code_num_" + std::to_string(enum_xerror_code_bad_channelid);
        XMETRICS_COUNTER_INCREMENT( fork_tag , 1);  
        SLEEP_NANOSECOND(rand() % 10);
        XMETRICS_ARRCNT_INCR(top::metrics::xmetrics_array_tag_t::blockstore_sharding_table_block_commit, rand() % 64, 100);
        XMETRICS_TIME_RECORD("test_time_metrics");

        SLEEP_NANOSECOND(rand() % 10);
        XMETRICS_ARRCNT_DECR(top::metrics::xmetrics_array_tag_t::blockstore_sharding_table_block_commit, rand() % 64, 100);
        XMETRICS_FLOW_COUNT("test_flow_metrics1", static_cast<uint64_t>(100));
        XMETRICS_FLOW_COUNT("test_flow_metrics2", static_cast<uint64_t>((100 + rand()) % 10000));
        SLEEP_NANOSECOND(rand() % 10);
    }
}

std::vector<std::pair<std::size_t, int64_t>> generate_random_data(std::size_t range, std::size_t num) {
    std::vector<std::pair<std::size_t, int64_t>> res;
    for (std::size_t i = 0; i < num; ++i) {
        res.push_back({rand() % range, rand() % 1000});
    }
    return res;
}

void test_metrics_array_counter_thread(top::metrics::xmetrics_array_tag_t tag,std::vector<std::pair<std::size_t,int64_t>> const & data){
    for(auto const & _p : data){
        XMETRICS_ARRCNT_INCR(tag,_p.first,_p.second);
        SLEEP_MILLSECOND(rand() % 10);
    }
}

void calc_expect_res(std::vector<std::pair<uint64_t,int64_t>> & res,std::vector<std::pair<std::size_t,int64_t>> const & data){
    for (auto const & _p : data) {
        res[_p.first].first += 1;
        res[_p.first].second += _p.second;
    }
}

TEST_F(metrics_test,array_multi_thread){
    auto data1 = generate_random_data(64,1000);
    auto data2 = generate_random_data(64,1000);
    auto data3 = generate_random_data(64,1000);
    std::thread t1 = std::thread(&test_metrics_array_counter_thread,top::metrics::xmetrics_array_tag_t::blockstore_sharding_table_block_genesis_connect,data1);
    t1.detach();
    std::thread t2 = std::thread(&test_metrics_array_counter_thread,top::metrics::xmetrics_array_tag_t::blockstore_sharding_table_block_genesis_connect,data2);
    t2.detach();
    std::thread t3 = std::thread(&test_metrics_array_counter_thread,top::metrics::xmetrics_array_tag_t::blockstore_sharding_table_block_genesis_connect,data3);
    t3.detach();

    std::vector<std::pair<uint64_t,int64_t>> exp_res(64,{0,0});
    calc_expect_res(exp_res,data1);
    calc_expect_res(exp_res,data2);
    calc_expect_res(exp_res,data3);
    printf("each_value:[");
    for (auto const & _p : exp_res) {
        printf("%" PRIu64 ",",_p.second);
    }
    printf("]\n");
    printf("each_count:[");
    for (auto const & _p : exp_res) {
        printf("%" PRIu64 ",",_p.first);
    }
    printf("]\n");

    SLEEP_SECOND(2);
    XMETRICS_COUNTER_INCREMENT("drives", 1);
    SLEEP_SECOND(3);
}

TEST_F(metrics_test, test_timed_out) {
    std::string test_name{"TEST_TIME_OUT_DEFAULT"};
    for (auto index = 0; index < TEST_CASE_SIZE; ++index) {
        XMETRICS_TIME_RECORD(test_name);
        SLEEP_SECOND(index);
    }
    for (auto index = 0; index < TEST_CASE_SIZE; ++index) {
        XMETRICS_TIMER_START(test_name);
        SLEEP_SECOND(index);
        XMETRICS_TIMER_STOP(test_name);
    }

    test_name = "TEST_TIME_OUT_KEY";
    for (auto index = 0; index < TEST_CASE_SIZE; ++index) {
        XMETRICS_TIME_RECORD_KEY(test_name, "string:time_out_info:xxx");
        SLEEP_SECOND(index);
    }
    for (auto index = 0; index < TEST_CASE_SIZE; ++index) {
        XMETRICS_TIMER_START(test_name);
        SLEEP_SECOND(index);
        XMETRICS_TIMER_STOP_KEY(test_name, 1234567);
    }

    test_name = "TEST_TIME_OUT_KEY_TIME";
    for (auto index = 0; index < TEST_CASE_SIZE; ++index) {
        XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(test_name, "string:time_out_info:xxx", 2000);
        SLEEP_MILLSECOND(index);
    }
    for (auto index = 0; index < TEST_CASE_SIZE; ++index) {
        XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(test_name, 1234567, std::chrono::seconds(2));
        SLEEP_SECOND(index);
    }

    SLEEP_SECOND(2);
}

void test_metrics_time_thread_start_end(std::string metrics_name, int sleep_time_millsecond) {
    XMETRICS_TIMER_START(metrics_name);
    SLEEP_MILLSECOND(sleep_time_millsecond);
    XMETRICS_TIMER_STOP(metrics_name);
}

void test_metrics_time_thread_record(std::string metrics_name, int sleep_time_millsecond) {
    XMETRICS_TIME_RECORD(metrics_name);
    SLEEP_MILLSECOND(sleep_time_millsecond);
}

TEST_F(metrics_test, test_multithread) {
    std::thread t1 = std::thread(&test_metrics_time_thread_start_end, "test_start_end", 1000);
    t1.detach();
    SLEEP_MILLSECOND(300);
    std::thread t2 = std::thread(&test_metrics_time_thread_start_end, "test_start_end", 100);
    t2.detach();
    SLEEP_MILLSECOND(600);
    std::thread t3 = std::thread(&test_metrics_time_thread_record, "test_record", 1000);
    t3.detach();
    SLEEP_MILLSECOND(300);
    std::thread t4 = std::thread(&test_metrics_time_thread_record, "test_record", 100);
    t4.detach();
    SLEEP_MILLSECOND(600);
    SLEEP_SECOND(2);
    XMETRICS_COUNTER_INCREMENT("drives", 1);
    SLEEP_SECOND(3);
}
// #endif
// #if 0
// to test metrics performance
// v0.3 result : tps over six billion message_event per second
// so the message_process can theoretically handle over three billion metrics per second.
TEST_F(metrics_test, performance) {
    std::size_t test_performance = 40000;
    std::size_t count = 20;
    for (std::size_t i = 0; i < count; ++i) {
        top::metrics::e_metrics::get_instance().stop();
        for (std::size_t index = 0; index < test_performance; ++index) {
            XMETRICS_FLOW_COUNT("test_benchmark", static_cast<uint64_t>(rand() % 10000));
        }
        XMETRICS_TIMER_START("TIME_benchmark_time");
        top::metrics::e_metrics::get_instance().start();
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        XMETRICS_TIMER_STOP("TIME_benchmark_time");
        SLEEP_SECOND(3);
    }
}

enum xevent_major_type_t {
    xevent_major_type_none,
    xevent_major_type_timer,
    xevent_major_type_chain_timer,
    xevent_major_type_store,
    xevent_major_type_sync_executor,
    xevent_major_type_network,
    // for dispatch message, due to multi-vnode
    // there is a dispatcher to send all message
    xevent_major_type_dispatch,
    // major type for all deceit types
    xevent_major_type_deceit,
    xevent_major_type_consensus,
    xevent_major_type_transaction,
    xevent_major_type_behind,
    xevent_major_type_vnode,
    xevent_major_type_account,
    xevent_major_type_role,
    xevent_major_type_blockfetcher,
    xevent_major_type_sync,
    xevent_major_type_state_sync,
    xevent_major_type_max
};

void test_metrics_minus() {
    for (auto _major = (int)xevent_major_type_none; _major < (int)xevent_major_type_behind; _major++) {
        XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)(top::metrics::xevent_begin + _major), -1);
    }
}

TEST_F(metrics_test, gauge) {
    int gauge_start = (int)top::metrics::e_simple_begin;
    int end = (int)top::metrics::e_simple_total;
    for(int round = 1; round <= 10; round++) {
        for(int gauge = gauge_start; gauge < end; gauge++) {
            // todo remove later
            if (gauge < (int)top::metrics::message_category_send || gauge > (int)top::metrics::message_broad_category_end) {
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)gauge, 1);
            }
        }
        if ((round % 2) == 0) {
            for(int gauge = gauge_start; gauge < end; gauge++) {
                if (gauge < (int)top::metrics::message_category_send || gauge > (int)top::metrics::message_broad_category_end) {
                    XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)gauge, -1);
                }
            }
        }

        top::metrics::e_metrics::get_instance().gauge_dump();
    }
    
    for (auto _major = (int)xevent_major_type_none; _major < (int)xevent_major_type_behind; _major++) {
        XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)(top::metrics::xevent_begin + _major), 1);
    }
    
    std::thread t1 = std::thread(&test_metrics_minus);
    t1.detach();
    
    for(auto gauge = gauge_start+1; gauge < end; gauge++) {
        if (gauge < (int)top::metrics::message_category_send || gauge > (int)top::metrics::message_broad_category_end) {
            auto value = XMETRICS_GAUGE_GET_VALUE((top::metrics::E_SIMPLE_METRICS_TAG)gauge);
            EXPECT_EQ(value, 5);
        }
    }
}

// #endif
