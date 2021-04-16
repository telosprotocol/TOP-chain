#include <vector>

#include "gtest/gtest.h"
#include "xstore/xstore.h"
#include "xstore/xstore_face.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/test/xstore_face_mock.h"

using namespace top;
using namespace top::store;
using namespace top::data;

class test_datamock : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_datamock, create_unit) {
    std::map<std::string, std::string> prop_list;
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    uint64_t count = 10;

    std::vector<xblock_ptr_t> units;
    for (uint64_t i = 0; i < count; i++) {
        xblock_ptr_t unit = datamock.create_unit(address, prop_list);
        ASSERT_EQ(unit->get_height(), i+1);
        units.push_back(unit);
    }

    {
        base::xauto_ptr<xblock_t> unit(store->get_block_by_height(address, 5));
        ASSERT_TRUE(unit.get());
        std::cout << "balance " << unit->get_balance_change() << std::endl;
    }


    for (uint64_t i = 0; i < count; i++) {
        base::xauto_ptr<xblock_t> unit(store->get_block_by_height(address, i+1));
        ASSERT_NE(unit, nullptr);
        ASSERT_EQ(unit->get_balance_change(), 100);
    }
}

TEST_F(test_datamock, create_unit_with_property) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string prop_name = "aaa";
    uint64_t count = 10;

    for (uint64_t i = 0; i < count; i++) {

        std::map<std::string, std::string> prop_list;
        prop_list[prop_name] = std::to_string(i);

        xblock_ptr_t unit = datamock.create_unit(address, prop_list);
        ASSERT_EQ(unit->get_height(), i+1);
        uint64_t chainheight = store->get_blockchain_height(address);
        ASSERT_EQ(chainheight, unit->get_height());
    }

    for (uint64_t i = 0; i < count; i++) {
        base::xauto_ptr<xblock_t> unit(store->get_block_by_height(address, i+1));
        ASSERT_NE(unit, nullptr);
        ASSERT_EQ(unit->get_balance_change(), 100);
    }
    std::vector<std::string> values;
    store->list_get_all(address, prop_name, values);
    ASSERT_EQ(values.size(), count-1);
    std::cout << "=====list all=====" << std::endl;
    for (auto & v : values) {
        std::cout << v << std::endl;
    }
}

