#include <stdio.h>
#include <string>
#include "gtest/gtest.h"
#include "xbase/xutl.h"
#include "xmetrics/xmetrics.h"
#include "xstatistic/xstatistic.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"


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

#ifdef CACHE_SIZE_STATISTIC
class test_class_t : public xstatistic_obj_face_t {
public:
    // test_class_t() : xstatistic_obj_face_t(enum_statistic_receipts) {}
    test_class_t(uint32_t x) : xstatistic_obj_face_t(enum_statistic_send_tx), n(x) {
        // std::cout << "test_class_t this : " << this << std::endl;
    }
    ~test_class_t() {
    }
    uint32_t n{0};
private:
    virtual const int32_t get_object_size_real() const override {
        return sizeof(*this);
    }
};

class test_class1_t : public xstatistic_obj_face_t {
public:
    test_class1_t(uint32_t x, uint32_t y) : xstatistic_obj_face_t(enum_statistic_receipts), n(x), m(y) {
        // std::cout << "test_class_t this : " << this << std::endl;
    }
    ~test_class1_t() {
    }
    uint32_t n{0};
    uint32_t m{0};
private:
    virtual const int32_t get_object_size_real() const override {
        return sizeof(*this);
    }
};

TEST_F(test_statistic, basic) {
    XSET_CONFIG(calculate_size_delay_time, 1);
    std::vector<test_class_t> obj_vec;
    // std::vector<test_class1_t> obj1_vec;

    uint32_t num = 100000;
    // obj_vec.resize(num);
    for (uint32_t i = 0; i < num; i++){
        obj_vec.push_back(test_class_t(i));
        // obj1_vec.push_back(test_class1_t(i, i));
    }

    // for (auto & obj : obj_vec) {
    //     std::cout << "obj.n : " << obj.n << std::endl;
    // }

    usleep(1000);
    std::cout << "sleep 1 millisecond." << std::endl;
    xstatistic_hub_t::instance()->refresh();

    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_send_tx_num);
    ASSERT_EQ(obj_num, num);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_send_tx_size);
    ASSERT_EQ(obj_size, num*sizeof(test_class_t));

    // auto obj_num1 = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_receipt_num);
    // ASSERT_EQ(obj_num1, num);
    // auto obj_size1 = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_receipt_size);
    // ASSERT_EQ(obj_size1, num*sizeof(test_class1_t));
}


void thread_push(std::vector<test_class_t> * obj_vec, std::mutex * mutex, bool * stop)
{
    uint32_t loop_num = 100000;
    for (uint32_t i = 0; i < loop_num; i++) {
        std::lock_guard<std::mutex> lck(*mutex);
        obj_vec->push_back(test_class_t(i));

        // std::cout << "thread_push i : " << i << " , size : " << obj_vec->size() << std::endl;

        if (i % 1000 == 0) {
            auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_send_tx_num);
            auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_send_tx_size);
            ASSERT_EQ(obj_size, obj_num*sizeof(test_class_t));
        }
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

TEST_F(test_statistic, multithread_BENCH) {
    std::vector<test_class_t> obj_vec;
    std::mutex mutex;
    bool stop = false;

    std::thread t_push(thread_push, &obj_vec, &mutex, &stop);
    std::thread t_pop(thread_pop, &obj_vec, &mutex, &stop);

    t_push.join();
    t_pop.join();

    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_send_tx_num);
    ASSERT_EQ(obj_num, 0);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_send_tx_size);
    ASSERT_EQ(obj_size, obj_num*sizeof(test_class_t));
}


#endif
