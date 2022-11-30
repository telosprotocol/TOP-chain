#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xbase/xutl.h"
#include "xcommon/xaccount_address.h"
#include "xbase/xhash.h"

using namespace top;
using namespace top::base;

class test_performance : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};




TEST_F(test_performance, xvaccount_BENCH_hash64) {
//total time: 90 ms    
    uint32_t count = 10000000;

    auto t1 = base::xtime_utl::time_now_ms();

    std::string addr = "Ta0000@1";
    uint64_t result;
    for (uint32_t i =0;i<count;i++) {
        result = base::xhash64_t::digest(addr);
    }

    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time: " << t2 - t1 << " ms" << std::endl;
}

TEST_F(test_performance, xvaccount_BENCH_address_construct) {
//total time: 3728 ms    
    uint32_t count = 10000000;

    auto t1 = base::xtime_utl::time_now_ms();

    std::string addr = "Ta0000@1";
    for (uint32_t i =0;i<count;i++) {
        common::xaccount_address_t _address(addr);
    }

    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time: " << t2 - t1 << " ms" << std::endl;
}

TEST_F(test_performance, xvaccount_BENCH_address_to_string) {
//total time: 5229 ms
    uint32_t count = 10000000;

    auto t1 = base::xtime_utl::time_now_ms();

    std::string addr = "Ta0000@1";
    for (uint32_t i =0;i<count;i++) {
        common::xaccount_address_t _address(addr);
        std::string addr2 = _address.to_string();
    }

    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time: " << t2 - t1 << " ms" << std::endl;
}

TEST_F(test_performance, xvaccount_BENCH_address_to_vaccount) {
//total time: 10641 ms
    uint32_t count = 10000000;

    auto t1 = base::xtime_utl::time_now_ms();

    std::string addr = "Ta0000@1";
    for (uint32_t i =0;i<count;i++) {
        common::xaccount_address_t _address(addr);
        base::xvaccount_t addr2 = _address.vaccount();
    }

    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time: " << t2 - t1 << " ms" << std::endl;
}

TEST_F(test_performance, xvaccount_BENCH_string_malloc) {
//total time: 21 ms
    uint32_t count = 10000000;

    auto t1 = base::xtime_utl::time_now_ms();

    std::string addr = "Ta0000@1";
    std::string addr2;
    for (uint32_t i =0;i<count;i++) {
        addr2 = addr;
    }

    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time: " << t2 - t1 << " ms" << std::endl;
}