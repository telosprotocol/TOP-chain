#include <stdio.h>
#include <string>
#include "gtest/gtest.h"
#include "xbase/xutl.h"
#include "xmetrics/xmetrics.h"
#include "xstatistic/xstatistic.h"


using namespace top;
using namespace top::base;
using namespace std;
using namespace top::xstatistic;

class test_statistic : public testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }
};

#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
class test_class_t : public xstatistic_obj_face_t {
public:
    // test_class_t() : xstatistic_obj_face_t(enum_statistic_receipts) {}
    test_class_t(uint32_t x) : xstatistic_obj_face_t(enum_statistic_tx_v2), n(x) {
        // std::cout << "test_class_t this : " << this << std::endl;
    }
    ~test_class_t() {
        statistic_del();
    }
    virtual int32_t get_class_type() const override {return enum_statistic_tx_v2;}
    uint32_t n{0};
private:
    virtual size_t get_object_size_real() const override {
        return sizeof(*this);
    }
};

class test_class1_t : public xstatistic_obj_face_t {
public:
    test_class1_t(uint32_t x, uint32_t y) : xstatistic_obj_face_t(enum_statistic_tx_v3), n(x), m(y) {
        // std::cout << "test_class_t this : " << this << std::endl;
    }
    ~test_class1_t() {
        statistic_del();
    }
    uint32_t n{0};
    uint32_t m{0};
    virtual int32_t get_class_type() const override {return enum_statistic_tx_v3;}
private:
    virtual size_t get_object_size_real() const override {
        return sizeof(*this);
    }
};

TEST_F(test_statistic, basic) {
    std::vector<test_class_t> obj_vec;

    uint32_t num = 10000;
    for (uint32_t i = 0; i < num; i++){
        obj_vec.push_back(test_class_t(i));
    }

    usleep(100000);
    std::cout << "sleep 1 millisecond." << std::endl;
    xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
    ASSERT_EQ(obj_num, num);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
    ASSERT_EQ(obj_size, num*sizeof(test_class_t));
#endif
}

TEST_F(test_statistic, 2_obj_type) {
    std::vector<test_class_t> obj_vec;
    std::vector<test_class1_t> obj1_vec;

    uint32_t num = 10000;
    for (uint32_t i = 0; i < num; i++){
        obj_vec.push_back(test_class_t(i));
        obj1_vec.push_back(test_class1_t(i, i));
    }

    usleep(100000);
    std::cout << "sleep 1 millisecond." << std::endl;
    xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
    ASSERT_EQ(obj_num, num);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
    ASSERT_EQ(obj_size, num*sizeof(test_class_t));

    auto obj_num1 = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v3_num);
    ASSERT_EQ(obj_num1, num);
    auto obj_size1 = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v3_size);
    ASSERT_EQ(obj_size1, num*sizeof(test_class1_t));
#endif
}

void thread_push(std::vector<test_class_t> * obj_vec, std::mutex * mutex, bool * stop)
{
    uint32_t loop_num = 100000;
    for (uint32_t i = 0; i < loop_num; i++) {
        std::lock_guard<std::mutex> lck(*mutex);
        obj_vec->push_back(test_class_t(i));

        // std::cout << "thread_push i : " << i << " , size : " << obj_vec->size() << std::endl;
#ifdef ENABLE_METRICS
        if (i % 1000 == 0) {
            auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
            auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
            ASSERT_EQ(obj_size, obj_num*sizeof(test_class_t));
        }
#endif
    }
    *stop = true;
}

void thread_pop(std::vector<test_class_t> * obj_vec, std::mutex * mutex, bool * stop)
{
    while(!(*stop)) {
        std::lock_guard<std::mutex> lck(*mutex);
        auto size = obj_vec->size();
        // std::cout << "thread_pop size : " << size << std::endl;
        if (size > 10) {
            obj_vec->erase(obj_vec->begin(), obj_vec->begin() + (size - 10));
        }
    }

    obj_vec->clear();
}

TEST_F(test_statistic, multithread_1_boj_type_BENCH) {
    std::vector<test_class_t> obj_vec;
    std::mutex mutex;
    bool stop = false;

    std::thread t_push(thread_push, &obj_vec, &mutex, &stop);
    std::thread t_pop(thread_pop, &obj_vec, &mutex, &stop);

    t_push.join();
    t_pop.join();
#ifdef ENABLE_METRICS
    usleep(100000);
    std::cout << "sleep 1 millisecond." << std::endl;
    xstatistic_t::instance().refresh();
    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
    ASSERT_EQ(obj_num, 0);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
    ASSERT_EQ(obj_size, obj_num*sizeof(test_class_t));
#endif
}

void thread_push1(std::vector<test_class1_t> * obj_vec, std::mutex * mutex, bool * stop)
{
    uint32_t loop_num = 100000;
    for (uint32_t i = 0; i < loop_num; i++) {
        std::lock_guard<std::mutex> lck(*mutex);
        obj_vec->push_back(test_class1_t(i, i));

        // std::cout << "thread_push i : " << i << " , size : " << obj_vec->size() << std::endl;
#ifdef ENABLE_METRICS
        if (i % 1000 == 0) {
            auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v3_num);
            auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v3_size);
            ASSERT_EQ(obj_size, obj_num*sizeof(test_class1_t));
        }
#endif
    }
    *stop = true;
}

void thread_pop1(std::vector<test_class1_t> * obj_vec, std::mutex * mutex, bool * stop)
{
    while(!(*stop)) {
        std::lock_guard<std::mutex> lck(*mutex);
        auto size = obj_vec->size();
        // std::cout << "thread_pop size : " << size << std::endl;
        if (size > 10) {
            obj_vec->erase(obj_vec->begin(), obj_vec->begin() + (size - 10));
        }
    }

    obj_vec->clear();
}

TEST_F(test_statistic, multithread_2_boj_type_BENCH) {
    std::vector<test_class_t> obj_vec;
    std::vector<test_class1_t> obj_vec1;
    std::mutex mutex;
    std::mutex mutex1;
    bool stop = false;
    bool stop1 = false;

    std::thread t_push(thread_push, &obj_vec, &mutex, &stop);
    std::thread t_pop(thread_pop, &obj_vec, &mutex, &stop);
    std::thread t_push1(thread_push1, &obj_vec1, &mutex1, &stop1);
    std::thread t_pop1(thread_pop1, &obj_vec1, &mutex1, &stop1);

    t_push.join();
    t_pop.join();
    t_push1.join();
    t_pop1.join();
#ifdef ENABLE_METRICS
    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
    ASSERT_EQ(obj_num, 0);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
    ASSERT_EQ(obj_size, obj_num*sizeof(test_class_t));

    auto obj_num1 = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v3_num);
    ASSERT_EQ(obj_num1, 0);
    auto obj_size1 = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v3_size);
    ASSERT_EQ(obj_size1, obj_num1*sizeof(test_class1_t));
#endif
}

#endif
