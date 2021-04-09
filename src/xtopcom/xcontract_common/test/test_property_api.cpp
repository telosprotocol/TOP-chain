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
        xproperty_access_control_data_t access_data;
        top::observer_ptr<xvbstate_t> vbstate{new xvbstate_t{address, 1,std::vector<top::base::xvproperty_t*>()}};
        api_ = new xproperty_api_t{vbstate, access_data};
    }
};

TEST_F(test_property_api, map_prop_api) {
    // map apis
    xaccount_address_t addr{address};
    xproperty_identifier_t  id{"test_create", xproperty_type_t::map, xproperty_category_t::sys_kernel};
    api_->map_prop_create<std::string, std::string>(addr, id);
    EXPECT_TRUE(api_->prop_exist(addr, id));

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


// TEST_F(test_property_api, queue_prop_test) {
//     // queue apis
//     std::string prop_name = "test_create";
//     api_->QUEUE_PROP_CREATE<std::string>(prop_name);
//     EXPECT_TRUE(api_->QUEUE_PROP_EXIST(prop_name));

//     api_->QUEUE_PROP_PUSHBACK<std::string>(prop_name, "first");
//     EXPECT_EQ(api_->QUEUE_PROP_QUERY<std::string>(prop_name, 0), "first");
//     api_->QUEUE_PROP_PUSHBACK<std::string>(prop_name, "second");
//     EXPECT_EQ(api_->QUEUE_PROP_QUERY<std::string>(prop_name, 0), "first");
//     EXPECT_EQ(api_->QUEUE_PROP_QUERY<std::string>(prop_name, 1), "second");

//     // api_->QUEUE_PROP_UPDATE(prop_name, 0, "0");
//     // EXPECT_EQ(api_->QUEUE_PROP_QUERY(prop_name, 0), "0");
//     // api_->QUEUE_PROP_UPDATE(prop_name, 1, "1");
//     // EXPECT_EQ(api_->QUEUE_PROP_QUERY(prop_name, 1), "1");
//     api_->QUEUE_PROP_CLEAR<std::string>(prop_name);
//     EXPECT_EQ(api_->QUEUE_PROP_QUERY<std::string>(prop_name).size(), 0);

//     std::deque<std::string> new_queue{"first", "second", "third"};
//     api_->QUEUE_PROP_UPDATE<std::string>(prop_name, new_queue);
//     EXPECT_EQ(api_->QUEUE_PROP_QUERY<std::string>(prop_name), new_queue);

//     // current not support queue erase
//     // api_->QUEUE_PROP_ERASE(prop_name, 0);
//     // std::map<std::string, std::string> target_map{"second", "third"};
//     // EXPECT_EQ(api_->QUEUE_PROP_QUERY(prop_name), target_map);
//     // api_->QUEUE_PROP_ERASE(prop_name, 1);
//     // target_map = {"second"};
//     // EXPECT_EQ(api_->QUEUE_PROP_QUERY(prop_name), target_map);
//     // api_->QUEUE_PROP_ERASE(prop_name, 0);
//     // target_map = {};
//     // EXPECT_EQ(api_->QUEUE_PROP_QUERY(prop_name), target_map);
// }

// TEST_F(test_property_api, str_prop_test) {
//     // str apis
//     std::string prop_name = "test_create";
//     api_->STR_PROP_CREATE(prop_name);
//     EXPECT_TRUE(api_->STR_PROP_EXIST(prop_name));
//     EXPECT_EQ(api_->STR_PROP_QUERY(prop_name), "");

//     api_->STR_PROP_UPDATE(prop_name, "0");
//     EXPECT_EQ(api_->STR_PROP_QUERY(prop_name), "0");
//     api_->STR_PROP_UPDATE(prop_name, "1");
//     EXPECT_EQ(api_->STR_PROP_QUERY(prop_name), "1");
//     api_->STR_PROP_CLEAR(prop_name);
//     EXPECT_EQ(api_->STR_PROP_QUERY(prop_name), "");

// }
