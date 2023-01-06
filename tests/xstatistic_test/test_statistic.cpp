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

#ifdef CACHE_SIZE_STATISTIC
class test_class_t : public xstatistic_obj_face_t {
public:
    test_class_t(uint32_t n){}
    virtual enum_statistic_class_type get_class_type() const override {
        return enum_statistic_send_tx;
    }
private:
    virtual const int32_t get_object_size_real() const override {
        return sizeof(*this);
    }
    uint32_t n{0};
};

TEST_F(test_statistic, basic) {
    std::vector<test_class_t> obj_vec;

    uint32_t num = 100;
    for (uint32_t i = 0; i < num; i++){
        obj_vec.push_back(test_class_t(i));
    }

    xstatistic_hub_t::instance()->refresh();

    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_send_tx_num);
    ASSERT_EQ(obj_num, num);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_send_tx_size);
    ASSERT_EQ(obj_size, num*sizeof(test_class_t));
}
#endif
