#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xbase/xcontext.h"
#include "xbasic/xobject_ptr.h"
#include "xdata/xblocktool.h"
#include "xdb/xdb_mem.h"
#include "xstore/xstore.h"

using namespace std;
using namespace top;
using namespace top::data;
using namespace top::base;
using namespace top::store;

class test_xstore_chaincore : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// TEST_F(test_xstore_chaincore, store_unit)
// {
//     std::shared_ptr<xdb_face_t> db = std::make_shared<xdb_mem_t>();
//     std::shared_ptr<xledger_t> ledger = std::make_shared<xledger_t>(db);
//     std::shared_ptr<xstore> store = std::make_shared<xstore>(ledger);
//     std::string address = "aaa";
//     bool ret;

//     //add fullunit to account
//     xobject_ptr_t<xfullunit_t> fullunit = make_object_ptr<xfullunit_t>();
//     {
//         fullunit->create_genesis_unit(address, 0);
//         fullunit->set_digest();
//         ASSERT_EQ(fullunit->height(), 1);
//         ret = store->update(address, fullunit);
//         ASSERT_TRUE(ret);
//         xaccount_ptr_t account = store->query_account(address);
//         ASSERT_TRUE(account != nullptr);
//         ASSERT_EQ(account->balance(), 0);
//         ASSERT_EQ(account->unit_height(), 1);
//     }

//     xobject_ptr_t<xlightunit_t> lightunit1 = make_object_ptr<xlightunit_t>();
//     {
//         int64_t balance_change = 10;
//         lightunit1->create_light_unit(fullunit->get_hash(), fullunit->height(), balance_change);
//         lightunit1->set_digest();
//         ASSERT_EQ(lightunit1->height(), 2);
//         ret = store->update(address, lightunit1);
//         ASSERT_TRUE(ret);
//         xaccount_ptr_t account = store->query_account(address);
//         ASSERT_TRUE(account != nullptr);
//         ASSERT_EQ(account->balance(), 10);
//         ASSERT_EQ(account->unit_height(), 2);
//     }

//     xobject_ptr_t<xlightunit_t> lightunit2 = make_object_ptr<xlightunit_t>();
//     {
//         int64_t balance_change = -5;
//         lightunit2->create_light_unit(lightunit1->get_hash(), lightunit1->height(), balance_change);
//         lightunit2->set_digest();
//         ASSERT_EQ(lightunit2->height(), 3);
//         ret = store->update(address, lightunit2);
//         ASSERT_TRUE(ret);
//         xaccount_ptr_t account = store->query_account(address);
//         ASSERT_TRUE(account != nullptr);
//         ASSERT_EQ(account->balance(), 5);
//         ASSERT_EQ(account->unit_height(), 3);
//     }
// }

TEST_F(test_xstore_chaincore, object_ptr_test_1)
{
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    xobject_ptr_t<xaccount_t> account = make_object_ptr<xaccount_t>(address);
    ASSERT_EQ(account->get_refcount(), 1);
}
TEST_F(test_xstore_chaincore, object_ptr_test_2)
{
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    xobject_ptr_t<xaccount_t> account = make_object_ptr<xaccount_t>(address);
    ASSERT_EQ(account->get_refcount(), 1);
    xobject_ptr_t<xdataobj_t> object = static_cast<xobject_ptr_t<xdataobj_t>>(account);
    ASSERT_EQ(account->get_refcount(), 2);
    xobject_ptr_t<xaccount_t> account2 = dynamic_xobject_ptr_cast<xaccount_t>(object);
    ASSERT_EQ(account->get_refcount(), 3);
    ASSERT_EQ(account2->address(), account->address());
}
TEST_F(test_xstore_chaincore, object_ptr_test_3)
{
    // std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    // xobject_ptr_t<xaccount_t> account = make_object_ptr<xaccount_t>(address);
    // ASSERT_EQ(account->get_refcount(), 1);
    // xobject_ptr_t<xdataobj_t> object = static_cast<xobject_ptr_t<xdataobj_t>>(account);
    // ASSERT_EQ(account->get_refcount(), 2);
    // xobject_ptr_t<const xaccount_t> account2 = static_cast<xobject_ptr_t<xaccount_t>> (object);
    // ASSERT_EQ(account->get_refcount(), 3);
    // ASSERT_EQ(account2->address(), account->address());
}

