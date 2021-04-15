#include "gtest/gtest.h"
#include "xBFT/src/xtimercertview.h"
#include "xvledger/xvblock.h"
#include "xBFT/xconsaccount.h"
#include "xbase/xthread.h"
#include "xdata/xgenesis_data.h"
#include "xcrypto/xckey.h"
#include "xcertauth/xcertauth_face.h"
#include "xBFT/test/common/xunitblock.hpp"

using namespace top;
using namespace top::data;
using namespace top::base;
using namespace top::xconsensus;

TEST(xconspacemaker_t, cache_one_node) {
    xvote_cache_t cache;

    uint64_t clock = 1;
    xvip2_t xip1;
    std::string qcert_bin1 = "123";

    ASSERT_TRUE(cache.add_qcert(xip1, clock, qcert_bin1));
    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock);
        ASSERT_EQ(validators.size(), 1);
    }
    ASSERT_FALSE(cache.add_qcert(xip1, clock, qcert_bin1));
    ASSERT_TRUE(cache.add_qcert(xip1, clock+1, qcert_bin1));
    ASSERT_TRUE(cache.add_qcert(xip1, clock+2, qcert_bin1));
    ASSERT_TRUE(cache.add_qcert(xip1, clock+3, qcert_bin1));

    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock);
        ASSERT_EQ(validators.size(), 1);
    }

    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock+1);
        ASSERT_EQ(validators.size(), 1);
    }

    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock+2);
        ASSERT_EQ(validators.size(), 1);
    }

    cache.clear();
    ASSERT_TRUE(cache.add_qcert(xip1, clock, qcert_bin1));
    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock);
        ASSERT_EQ(validators.size(), 1);
    }
}

TEST(xconspacemaker_t, cache_multi_node) {
    xvote_cache_t cache;

    uint64_t clock = 1;
    xvip2_t xip1;
    std::string qcert_bin1 = "123";

    xvip2_t xip2;
    xip2.high_addr = 2;
    std::string qcert_bin2 = "456";

    ASSERT_TRUE(cache.add_qcert(xip1, clock, qcert_bin1));
    ASSERT_TRUE(cache.add_qcert(xip2, clock, qcert_bin2));
    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock);
        ASSERT_EQ(validators.size(), 2);
        const std::string &str1 = validators.at(xip1);
        ASSERT_EQ(str1, qcert_bin1);

        const std::string &str2 = validators.at(xip2);
        ASSERT_EQ(str2, qcert_bin2);
    }

    ASSERT_TRUE(cache.add_qcert(xip1, clock+1, qcert_bin1));
    ASSERT_TRUE(cache.add_qcert(xip2, clock+1, qcert_bin2));
    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock+1);
        ASSERT_EQ(validators.size(), 2);

        const std::string &str1 = validators.at(xip1);
        ASSERT_EQ(str1, qcert_bin1);

        const std::string &str2 = validators.at(xip2);
        ASSERT_EQ(str2, qcert_bin2);
    }

    cache.clear();
    ASSERT_TRUE(cache.add_qcert(xip1, clock, qcert_bin1));
    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock);
        ASSERT_EQ(validators.size(), 1);
    }
}

#if 0
TEST(xconspacemaker_t, cache_clear_timeout) {
    xvote_cache_t cache;

    uint64_t clock = 1;
    xvip2_t xip1;
    std::string qcert_bin1 = "123";

    ASSERT_TRUE(cache.add_qcert(xip1, clock, qcert_bin1));

    cache.clear_timeout_clock();

    {
        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock);
        ASSERT_EQ(validators.size(), 1);
    }

    uint32_t i = 0;
    for (;;) {
        i++;
        sleep(1);

        cache.clear_timeout_clock();

        const std::map<xvip2_t,std::string,xvip2_compare>& validators = cache.get_clock_votes(clock);

        if (validators.size() == 0)
            break;
    }

    ASSERT_TRUE(i>20);
    ASSERT_TRUE(i<22);
}
#endif

