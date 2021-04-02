// #include "gtest/gtest.h"
// #include "xdata/xslash.h"
// #include "xdata/xblocktool.h"

// #include <string>
// #include <memory>

// using namespace top::base;

// struct test_data {

//     operator std::string() const {
//         return std::to_string(block_count_) + std::to_string(subset_count_);
//     }

//     uint32_t block_count_;
//     uint32_t subset_count_;

// };


// class test_property_object: public testing::Test {
// public:
//     std::shared_ptr<top::contract::xaccount_state_t> context_;

// public:
//     test_data make_test_data(uint32_t block_count, uint32_t subset_count) {
//         test_data test;
//         test.block_count_ = block_count;
//         test.subset_count_ = subset_count;
//         return test;
//     }

// protected:
//     void SetUp() override {
//         std::string address = xblocktool_t::make_address_user_account("T00000LPggAizKRsMxzS3xwKBRk3Q8qu5xGbz2Q3");
//         top::xobject_ptr_t<xvbstate_t> vbstate;
//         vbstate.attach(new xvbstate_t{address, 1,std::vector<top::base::xvproperty_t*>()});
//         std::shared_ptr<xproperty_access_control_t> api_ = std::make_shared<xproperty_access_control_t>(vbstate);

//         top::contract::xcontract_metadata_t meta;
//         context_ = std::make_shared<xproperty_context_t>(api_, rule, meta);
//     }

// };


// TEST_F(test_property_object, simple_map_property) {

//     // simple map<string, string>
//     xtop_map_property<std::string, std::string> map_prop{"test_map1", context_};
//     EXPECT_EQ(map_prop.clone().size(), 0);

//     map_prop.add("first", "first");
//     EXPECT_EQ(map_prop.query("first"), "first");

//     map_prop.update("first", "another");
//     EXPECT_EQ(map_prop.query("first"), "another");
//     map_prop.clear();
//     EXPECT_EQ(map_prop.clone().size(), 0);

//     std::map<std::string, std::string> new_map{{"first", "first"}, {"second", "second"}, {"third", "third"}};
//     map_prop.update(new_map);
//     EXPECT_EQ(map_prop.clone(), new_map);


//     map_prop.erase("first");
//     std::map<std::string, std::string> target_map{{"second", "second"}, {"third", "third"}};
//     EXPECT_EQ(map_prop.clone(), target_map);

// }

// TEST_F(test_property_object, obj_map_property) {
//     // map<string, object>
//     xtop_map_property<std::string, test_data> obj_prop{"test_map2", context_};
//     EXPECT_EQ(obj_prop.clone().size(), 0);

//     test_data test = make_test_data(1, 1);
//     obj_prop.add("first", test);
//     auto res = obj_prop.query("first");
//     EXPECT_EQ(res, (std::string)test);

// }
