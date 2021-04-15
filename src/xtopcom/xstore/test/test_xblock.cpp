#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xstore/xstore.h"
#include "xstore/xstore_face.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xstore/test/test_datamock.hpp"
#include "xmbus/xevent_store.h"

using namespace top;
using namespace top::base;
using namespace top::store;
using namespace top::data;

class test_xblock : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_xblock, cache_time_1) {
    std::map<std::string, std::string> prop_list;
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    uint64_t count = 1;
    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, prop_list);
    }

    {
        ASSERT_EQ(count, store->get_blockchain_height(address));
        for (uint64_t i = 0; i < count; i++) {
            uint64_t block_height = i + 1;
            base::xauto_ptr<xblock_t> block(store->get_block_by_height(address, block_height));
            ASSERT_EQ(block->get_height(), block_height);
        }
    }

    // sleep(4);

    {
        ASSERT_EQ(count, store->get_blockchain_height(address));
        for (uint64_t i = 0; i < count; i++) {
            uint64_t block_height = i + 1;
            base::xauto_ptr<xblock_t> block(store->get_block_by_height(address, block_height));
            ASSERT_EQ(block->get_height(), block_height);
        }
    }
}
