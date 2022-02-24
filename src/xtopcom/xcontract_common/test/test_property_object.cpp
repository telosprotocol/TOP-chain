#include "gtest/gtest.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xdata/xblocktool.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xbasic_contract.h"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xproperties/xproperty_map.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include <string>
#include <memory>

using namespace top::base;
using namespace top::data;
using namespace top::contract_common::properties;

struct test_data {

    operator std::string() const {
        return std::to_string(block_count_) + std::to_string(subset_count_);
    }

    uint32_t block_count_;
    uint32_t subset_count_;

};


class test_property_object: public testing::Test {
public:
    top::contract_common::xcontract_state_t* context_;

public:
    test_data make_test_data(uint32_t block_count, uint32_t subset_count) {
        test_data test;
        test.block_count_ = block_count;
        test.subset_count_ = subset_count;
        return test;
    }

protected:
    void SetUp() override {
        std::string address = xblocktool_t::make_address_user_account("T00000LPggAizKRsMxzS3xwKBRk3Q8qu5xGbz2Q3");
        top::xobject_ptr_t<xvbstate_t> vbstate;
        vbstate.attach(new xvbstate_t{address, (uint64_t)1, (uint64_t)0, std::string{""}, std::string{""}, (uint64_t)0, (uint32_t)0, (uint16_t)0});
        xproperty_access_control_data_t ac_data;
        std::shared_ptr<xproperty_access_control_t> api_ = std::make_shared<xproperty_access_control_t>(top::make_observer(vbstate.get()), ac_data);

        top::contract_common::xcontract_metadata_t meta;
        context_ =  new top::contract_common::xcontract_state_t{top::common::xaccount_address_t{address}, top::make_observer(api_.get())};
    }

};


TEST_F(test_property_object, simple_map_property) {

    // simple map<string, string>
    xtop_map_property<std::string, std::string> map_prop{"test_map1", nullptr};
    EXPECT_EQ(map_prop.clone().size(), 0);

    map_prop.add("first", "first");
    EXPECT_EQ(map_prop.query("first"), "first");

    map_prop.update("first", "another");
    EXPECT_EQ(map_prop.query("first"), "another");
    map_prop.clear();
    EXPECT_EQ(map_prop.clone().size(), 0);

    std::map<std::string, std::string> new_map{{"first", "first"}, {"second", "second"}, {"third", "third"}};
    map_prop.update(new_map);
    EXPECT_EQ(map_prop.clone(), new_map);


    map_prop.erase("first");
    std::map<std::string, std::string> target_map{{"second", "second"}, {"third", "third"}};
    EXPECT_EQ(map_prop.clone(), target_map);

}

TEST_F(test_property_object, obj_map_property) {
    // map<string, object>
    xtop_map_property<std::string, test_data> obj_prop{"test_map2", nullptr};
    EXPECT_EQ(obj_prop.clone().size(), 0);

    test_data test = make_test_data(1, 1);
    obj_prop.add("first", test);
    auto res = obj_prop.query("first");
    EXPECT_EQ(res, (std::string)test);

}
