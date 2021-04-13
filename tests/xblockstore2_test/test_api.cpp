#include "gtest/gtest.h"
#include "xdata/xblocktool.h"
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xdatamock_address.hpp"
#include "test_blockstore_util.hpp"

using namespace top;

class test_api : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_api, store_genesis_block_1) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t init_balance = 100;
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
    xassert(genesis_block != nullptr);
    base::xvaccount_t _vaddr(address);

    auto ret = blockstore->store_block(_vaddr, genesis_block.get());
    xassert(ret);
}

TEST_F(test_api, store_genesis_block_2) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t init_balance = 100;
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
    xassert(genesis_block != nullptr);
    base::xvaccount_t _vaddr(address);
    auto ret = blockstore->store_block(_vaddr, genesis_block.get());
    xassert(ret);
    sleep(15);
    xdbg("jimmy========acct_t begin to get");
    base::xauto_ptr<base::xvblock_t> query_block = blockstore->get_latest_committed_block(_vaddr);
    xdbg("jimmy========acct_t end to get");
    xassert(query_block->get_block_class() == base::enum_xvblock_class_light);
    data::xlightunit_block_t* lightunit = dynamic_cast<data::xlightunit_block_t*>(query_block.get());
    xassert(lightunit != nullptr);
    xassert((int64_t)init_balance == lightunit->get_balance_change());
}


TEST_F(test_api, store_genesis_block_3) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t init_balance = 100;
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
    xassert(genesis_block != nullptr);
    base::xvaccount_t _vaddr(address);

    auto ret = blockstore->store_block(_vaddr, genesis_block.get());
    xassert(ret);
    ret = blockstore->execute_block(_vaddr, genesis_block.get());
    xassert(ret);
    base::xauto_ptr<base::xvblock_t> query_block = blockstore->get_latest_committed_block(_vaddr);
    xassert(query_block->get_block_class() == base::enum_xvblock_class_light);
    data::xlightunit_block_t* lightunit = dynamic_cast<data::xlightunit_block_t*>(query_block.get());
    xassert(lightunit != nullptr);
    xassert((int64_t)init_balance == lightunit->get_balance_change());
}

