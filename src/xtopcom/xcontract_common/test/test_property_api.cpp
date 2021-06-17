#include "gtest/gtest.h"

#include "xcontract_common/test/xproperty_api.h"
#include "xcontract_common/xproperties/xproperty_identifier.h"

#include "xbasic/xmemory.hpp"
#include "xdata/xblocktool.h"
#include "xstore/xstore.h"
#include "xstore/xstore_face.h"

using namespace top::common;
using namespace top::base;

class test_property_api : public testing::Test {
public:
    xproperty_api_t* api_;
    std::string address;

protected:
    void SetUp() override {
        address = xblocktool_t::make_address_user_account("T00000LPggAizKRsMxzS3xwKBRk3Q8qu5xGbz2Q3");
        top::base::xvblock_t::register_object(top::base::xcontext_t::instance());
        xproperty_access_control_data_t access_data;
        top::observer_ptr<xvbstate_t> vbstate{new xvbstate_t{address, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0}};
        api_ = new xproperty_api_t{vbstate, access_data};
    }
};

TEST_F(test_property_api, map_prop_api) {
    // map apis
    xaccount_address_t addr{address};
    xproperty_identifier_t  id{"test_create", xproperty_type_t::map, xproperty_category_t::system};
    api_->map_prop_create<std::string, std::string>(addr, id);
    EXPECT_TRUE(api_->property_exist(addr, id));

    api_->map_prop_add<std::string, std::string>(addr, id, "first", "first");
    auto res_value = api_->map_prop_query<std::string, std::string>(addr, id, "first");
    EXPECT_EQ(res_value, "first");

    api_->map_prop_update<std::string, std::string>(addr, id, "first", "another");
    res_value = api_->map_prop_query<std::string, std::string>(addr, id, "first");
    EXPECT_EQ(res_value, "another");
    api_->map_prop_clear<std::string, std::string>(addr, id);
    auto res_map = api_->map_prop_query<std::string, std::string>(addr, id);
    EXPECT_EQ(res_map.size(), 0);

    std::map<std::string, std::string> new_map{{"first", "first"}, {"second", "second"}, {"third", "third"}};
    api_->map_prop_update<std::string, std::string>(addr, id, new_map);
    res_map = api_->map_prop_query<std::string, std::string>(addr, id);
    EXPECT_EQ(res_map, new_map);

    api_->map_prop_erase<std::string, std::string>(addr, id, "first");
    std::map<std::string, std::string> target_map{{"second", "second"}, {"third", "third"}};
    res_map = api_->map_prop_query<std::string, std::string>(addr, id);
    EXPECT_EQ(res_map, target_map);
}
