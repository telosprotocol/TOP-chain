#include "gtest/gtest.h"

#include "xbase/xobject_ptr.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xmbus/xmessage_bus.h"
#include "xmbus/xevent_store.h"

#define private public
#include "xloader/xconfig_onchain_loader.h"


using namespace top::loader;

class test_loader_api: public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};


class test_onchain_loader: public xconfig_onchain_loader_t {
public:
    test_onchain_loader(top::observer_ptr<top::mbus::xmessage_bus_face_t> const & bus,
                        top::observer_ptr<top::time::xchain_time_face_t> const & logic_timer): xconfig_onchain_loader_t(bus, logic_timer) {}
};


TEST_F(test_loader_api, onchain_loader_filter_changes) {
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = top::make_object_ptr<top::time::xchain_timer_t>(timer_driver);
    auto mbus = new top::mbus::xmessage_bus_t();

    test_onchain_loader onchain_loader(top::make_observer(mbus), top::make_observer(chain_timer.get()));
    onchain_loader.m_last_param_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}};

    // filter modification
    std::map<std::string, std::string> property_map = {{"id1", "modify"}, {"id2", "test2"}, {"id3", "test3"}};
    std::map<std::string, std::string> filter_map;
    onchain_loader.filter_changes(property_map, filter_map);
    EXPECT_TRUE(filter_map.size() == 1);
    EXPECT_EQ(filter_map["id1"], "modify");
    property_map = {{"id1", ""}, {"id2", "test2"}, {"id3", "test3"}};
    filter_map.clear();
    onchain_loader.filter_changes(property_map, filter_map);
    EXPECT_TRUE(filter_map.size() == 1);
    EXPECT_EQ(filter_map["id1"], "");

    // filter addition
    property_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}, {"id4", "test4"}};
    filter_map.clear();
    onchain_loader.filter_changes(property_map, filter_map);
    EXPECT_TRUE(filter_map.size() == 1);
    EXPECT_EQ(filter_map["id4"], "test4");

    // delete in delete case
}


TEST_F(test_loader_api, onchain_loader_delete_params) {
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = top::make_object_ptr<top::time::xchain_timer_t>(timer_driver);
    auto mbus = new top::mbus::xmessage_bus_t();

    test_onchain_loader onchain_loader(top::make_observer(mbus), top::make_observer(chain_timer.get()));
    onchain_loader.m_last_param_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}};

    // delete case
    std::map<std::string, std::string> property_map = {{"id1", "modify"}, {"id2", "test2"}};
    std::map<std::string, std::string> delete_map;
    onchain_loader.get_deleted_params(property_map, delete_map);
    EXPECT_TRUE(delete_map.size() == 1);
    EXPECT_EQ(delete_map["id3"], "test3");
}


TEST_F(test_loader_api, onchain_loader_param_changed) {
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = top::make_object_ptr<top::time::xchain_timer_t>(timer_driver);
    auto mbus = new top::mbus::xmessage_bus_t();

    test_onchain_loader onchain_loader(top::make_observer(mbus), top::make_observer(chain_timer.get()));
   onchain_loader.m_last_param_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}};

   // not change
   std::map<std::string, std::string> property_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}};
   EXPECT_FALSE(onchain_loader.onchain_param_changed(property_map));

   // changed
   //1. modify
   property_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "modify"}};
   EXPECT_TRUE(onchain_loader.onchain_param_changed(property_map));

   //2. add
   property_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}, {"id4", "test4"}};
   EXPECT_TRUE(onchain_loader.onchain_param_changed(property_map));

   //3. delete
   property_map = {{"id1", "test1"}, {"id2", "test2"}};
   EXPECT_TRUE(onchain_loader.onchain_param_changed(property_map));
}