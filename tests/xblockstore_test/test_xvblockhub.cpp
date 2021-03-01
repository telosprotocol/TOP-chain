#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbase/xvledger.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/tests/test_blockutl.hpp"

#include "test_blockmock.hpp"
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"
#include "xblockstore/src/xvblockhub.h"

using namespace top;
using namespace top::base;
using namespace top::store;
using namespace top::data;

class test_xvblockhub : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_xvblockhub, xblockacct_normal_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    xblockacct_t* blockacct = new xchainacct_t(address, 10000, "", *store, *blockstore);
    xblockacct_t::init_account(base::xcontext_t::instance());
    ASSERT_TRUE(blockacct->init());

    {
        auto block = blockacct->get_latest_committed_block();
        xassert(block->get_height() == 0);
    }

    xauto_ptr<base::xvblock_t> block0(test_blocktuil::create_genesis_empty_table(address));
    base::xvblock_t* prevblock = block0.get();
    uint64_t max_count = 10;
    uint64_t count = max_count;
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(true == blockacct->store_block(nextblock.get()));
        prevblock = nextblock.get();
        count--;
    } while (count > 0);

    {
        auto commitblock = blockacct->get_latest_committed_block();
        xassert(commitblock->get_height() == max_count - 2);
        auto lockblock = blockacct->get_latest_locked_block();
        xassert(lockblock->get_height() == max_count - 1);
        auto certblock = blockacct->get_latest_cert_block();
        xassert(certblock->get_height() == max_count - 0);
        auto executeblock = blockacct->get_latest_executed_block();
        xassert(executeblock->get_height() == max_count - 2);
        auto connectblock = blockacct->get_latest_connected_block();
        xassert(connectblock->get_height() == max_count - 2);
        auto current_block = blockacct->get_latest_current_block();
        xassert(current_block->get_height() == max_count);
    }
}

TEST_F(test_xvblockhub, xblockacct_normal_2) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    xblockacct_t* blockacct = new xchainacct_t(address, 10000, "", *store, *blockstore);
    xblockacct_t::init_account(base::xcontext_t::instance());
    ASSERT_TRUE(blockacct->init());

    {
        auto block = blockacct->get_latest_committed_block();
        xassert(block->get_height() == 0);
    }

    xauto_ptr<base::xvblock_t> block0(test_blocktuil::create_genesis_empty_table(address));
    base::xvblock_t* prevblock = block0.get();
    uint64_t max_count = 10;
    uint64_t count = max_count;
    std::vector<base::xvblock_t*> blocks;
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        nextblock->add_ref();
        blocks.push_back(nextblock.get());
        // xassert(true == blockacct->store_block(nextblock.get()));
        prevblock = nextblock.get();
        count--;
    } while (count > 0);

    for (uint64_t i = 0; i < max_count; i++) {
        xassert(true == blockacct->store_block(blocks[max_count-i-1]));
    }

    {
        std::cout << blockacct->dump() << std::endl;
        auto commitblock = blockacct->get_latest_committed_block();
        xassert(commitblock->get_height() == max_count - 2);
        auto lockblock = blockacct->get_latest_locked_block();
        xassert(lockblock->get_height() == max_count - 1);
        auto certblock = blockacct->get_latest_cert_block();
        xassert(certblock->get_height() == max_count - 0);
        auto executeblock = blockacct->get_latest_executed_block();
        xassert(executeblock->get_height() == max_count - 2);
        auto connectblock = blockacct->get_latest_connected_block();
        xassert(connectblock->get_height() == max_count - 2);
        auto current_block = blockacct->get_latest_current_block(false);
        xassert(current_block->get_height() == max_count);
    }
}

TEST_F(test_xvblockhub, xblockacct_not_save_metadata_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    xblockacct_t* blockacct = new xchainacct_t(address, 10000, "", *store, *blockstore);
    xblockacct_t::init_account(base::xcontext_t::instance());
    ASSERT_TRUE(blockacct->init());

    {
        auto block = blockacct->get_latest_committed_block();
        xassert(block->get_height() == 0);
    }

    xauto_ptr<base::xvblock_t> block0(test_blocktuil::create_genesis_empty_table(address));
    base::xvblock_t* prevblock = block0.get();
    uint64_t max_count = 10;
    uint64_t count = max_count;
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(true == blockacct->store_block(nextblock.get()));
        prevblock = nextblock.get();
        count--;
    } while (count > 0);

    {
        std::cout << blockacct->dump() << std::endl;
        auto current_block = blockacct->get_latest_current_block();
        xassert(current_block->get_height() == max_count);
    }
    // reload from db, the highqc height is missed
    xblockacct_t* blockacct2 = new xchainacct_t(address, 10000, "", *store, *blockstore);
    blockacct2->init();
    {
        std::cout << blockacct2->dump() << std::endl;
    }
}

class test_xchainacct_t : public xchainacct_t
        {
        public:
            test_xchainacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,xstore_face_t & _persist_db,base::xvblockstore_t& _blockstore)
            : xchainacct_t(account_addr,timeout_ms,blockstore_path,_persist_db,_blockstore){}
        protected:
            virtual ~test_xchainacct_t() {}
        private:
            test_xchainacct_t();
            test_xchainacct_t(const test_xchainacct_t &);
            test_xchainacct_t & operator = (const test_xchainacct_t &);
        public:
            void set_meta(base::xvblock_t* highest_execute_block) {
                m_meta->_highest_execute_block_height = highest_execute_block->get_height();
                m_meta->_highest_execute_block_hash = highest_execute_block->get_block_hash();
            }
        };

TEST_F(test_xvblockhub, xblockacct_not_save_metadata_2) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    test_xchainacct_t* blockacct = new test_xchainacct_t(address, 10000, "", *store, *blockstore);
    xblockacct_t::init_account(base::xcontext_t::instance());
    ASSERT_TRUE(blockacct->init());

    {
        auto block = blockacct->get_latest_committed_block();
        xassert(block->get_height() == 0);
    }

    xauto_ptr<base::xvblock_t> block0(test_blocktuil::create_genesis_empty_table(address));
    base::xvblock_t* prevblock = block0.get();
    prevblock->add_ref();
    xassert(true == blockacct->execute_block(prevblock));
    uint64_t max_count = 10;
    uint64_t count = max_count;
    base::xvblock_t* old_execute_block;
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        nextblock->set_block_flag(base::enum_xvblock_flag_locked);
        nextblock->set_block_flag(base::enum_xvblock_flag_committed);
        nextblock->set_block_flag(base::enum_xvblock_flag_connected);
        xassert(true == blockacct->execute_block(nextblock.get()));
        prevblock->release_ref();
        prevblock = nextblock.get();
        prevblock->add_ref();
        count--;

        if (count == max_count/2) {
            old_execute_block = nextblock.get();
            old_execute_block->add_ref();
        }

    } while (count > 0);

    {
        std::cout << blockacct->dump() << std::endl;
        auto current_block = blockacct->get_latest_current_block();
        xassert(current_block->get_height() == max_count);
    }

    blockacct->set_meta(old_execute_block);
    {
        std::cout << blockacct->dump() << std::endl;
        auto current_block = blockacct->get_latest_current_block();
        xassert(current_block->get_height() == max_count);
    }

    // {
    //     xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
    //     nextblock->reset_block_flags();
    //     nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
    //     nextblock->set_block_flag(base::enum_xvblock_flag_locked);
    //     nextblock->set_block_flag(base::enum_xvblock_flag_committed);
    //     nextblock->set_block_flag(base::enum_xvblock_flag_connected);
    //     xassert(true == blockacct->execute_block(nextblock.get()));
    //     xassert(nextblock->check_block_flag(base::enum_xvblock_flag_executed));
    // }
    // {
    //     std::cout << blockacct->dump() << std::endl;
    // }
}

TEST_F(test_xvblockhub, xblockacct_not_save_metadata_3) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    test_xchainacct_t* blockacct = new test_xchainacct_t(address, 10000, "", *store, *blockstore);
    xblockacct_t::init_account(base::xcontext_t::instance());
    ASSERT_TRUE(blockacct->init());

    {
        auto block = blockacct->get_latest_committed_block();
        xassert(block->get_height() == 0);
    }

    xauto_ptr<base::xvblock_t> block0(test_blocktuil::create_genesis_empty_unit(address));
    base::xvblock_t* prevblock = block0.get();
    prevblock->add_ref();
    xassert(true == blockacct->store_block(prevblock));
    uint64_t max_count = 10;
    uint64_t count = max_count;
    base::xvblock_t* old_execute_block;
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        // nextblock->set_block_flag(base::enum_xvblock_flag_locked);
        // nextblock->set_block_flag(base::enum_xvblock_flag_committed);
        // nextblock->set_block_flag(base::enum_xvblock_flag_connected);
        xassert(true == blockacct->store_block(nextblock.get()));
        prevblock->release_ref();
        prevblock = nextblock.get();
        prevblock->add_ref();
        count--;

        if (count == max_count/2) {
            old_execute_block = nextblock.get();
            old_execute_block->add_ref();
        }

    } while (count > 0);

    {
        std::cout << blockacct->dump() << std::endl;
        auto current_block = blockacct->get_latest_current_block();
        xassert(current_block->get_height() == max_count);
    }

    blockacct->set_meta(old_execute_block);
    {
        std::cout << blockacct->dump() << std::endl;
        auto latest_executed_block = blockacct->get_latest_executed_block();
        xassert(latest_executed_block->get_height() == max_count-2);
        auto current_block = blockacct->get_latest_current_block();
        xassert(current_block->get_height() == max_count);
    }

    {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        // nextblock->set_block_flag(base::enum_xvblock_flag_locked);
        // nextblock->set_block_flag(base::enum_xvblock_flag_committed);
        // nextblock->set_block_flag(base::enum_xvblock_flag_connected);
        xassert(true == blockacct->store_block(nextblock.get()));
        // xassert(nextblock->check_block_flag(base::enum_xvblock_flag_executed));
    }
    {
        std::cout << blockacct->dump() << std::endl;
        auto latest_executed_block = blockacct->get_latest_executed_block();
        xassert(latest_executed_block->get_height() == max_count -1);
    }
}

TEST_F(test_xvblockhub, execute_height_behind_2) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    // xblockacct_t* blockacct = new xblockacct_t(address, 10000, "", *store, *blockstore);
    // xblockacct_t::init_account(base::xcontext_t::instance());

    {
        auto block = blockstore->get_latest_committed_block(address);
        xassert(block->get_height() == 0);
        auto current_block = blockstore->get_latest_current_block(address);
        xassert(current_block->get_height() == 0);
    }

    xauto_ptr<base::xvblock_t> block0(test_blocktuil::create_genesis_empty_table(address));
    base::xvblock_t* prevblock = block0.get();
    uint64_t count = 4;
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(true == blockstore->store_block(nextblock.get()));
        prevblock = nextblock.get();
        count--;
    } while (count > 0);

    // {
    //     auto commitblock = blockacct->get_latest_committed_block();
    //     xassert(commitblock->get_height() == count - 2);
    //     auto lockblock = blockacct->get_latest_locked_block();
    //     xassert(lockblock->get_height() == count - 1);
    //     auto certblock = blockacct->get_latest_cert_block();
    //     xassert(certblock->get_height() == count - 0);
    // }
}

TEST_F(test_xvblockhub, xblockacct_locked_block_fork) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    xblockacct_t* blockacct = new xchainacct_t(address, 10000, "", *store, *blockstore);
    xblockacct_t::init_account(base::xcontext_t::instance());
    ASSERT_TRUE(blockacct->init());

    {
        auto block = blockacct->get_latest_committed_block();
        xassert(block->get_height() == 0);
    }

    xauto_ptr<base::xvblock_t> block0(test_blocktuil::create_genesis_empty_table(address));
    base::xvblock_t* prevblock = block0.get();
    uint64_t max_count = 10;
    uint64_t count = max_count;
    // create 10 empty block
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(true == blockacct->store_block(nextblock.get()));
        prevblock = nextblock.get();
        count--;
    } while (count > 0);

    base::xvblock_t* latest_connected_block = prevblock;

    // create a block that will fork with another one and save to blockstore
    xauto_ptr<base::xvblock_t> fork_block1(test_blocktuil::create_next_emptyblock(prevblock));
    fork_block1->reset_block_flags();
    fork_block1->set_block_flag(base::enum_xvblock_flag_authenticated);
    xassert(true == blockacct->store_block(fork_block1.get()));

    // create a block grow from fork_block1 which will push fork_block1 to locked state
    xauto_ptr<base::xvblock_t> fork_block1_next(test_blocktuil::create_next_emptyblock(fork_block1.get()));
    fork_block1_next->reset_block_flags();
    fork_block1_next->set_block_flag(base::enum_xvblock_flag_authenticated);
    xassert(true == blockacct->store_block(fork_block1_next.get()));

    // create another locked block that forks with fork_block1, not save to blockstore for the moment
    xauto_ptr<base::xvblock_t> fork_block2(test_blocktuil::create_next_emptyblock(prevblock, fork_block1->get_clock() + 1));
    fork_block2->reset_block_flags();
    fork_block2->set_block_flag(base::enum_xvblock_flag_authenticated);
    prevblock = fork_block2.get();

    // create a block grow from fork_block2, not save to blockstore for the moment
    xauto_ptr<base::xvblock_t> fork_block2_next(test_blocktuil::create_next_emptyblock(prevblock));
    fork_block2_next->reset_block_flags();
    fork_block2_next->set_block_flag(base::enum_xvblock_flag_authenticated);
    prevblock = fork_block2_next.get();

    // create 10 block grow from fork_block2 and store
    count = max_count;
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(true == blockacct->store_block(nextblock.get()));
        prevblock = nextblock.get();
        count--;
    } while (count > 0);

    base::xvblock_t* lastest_cur_block = blockacct->get_latest_current_block();
    xassert(lastest_cur_block == fork_block1.get());

    // save fork_block2 to blockstore, it will replace fork_block1.get()
    fork_block2_next->set_block_flag(base::enum_xvblock_flag_locked);
    fork_block2_next->set_block_flag(base::enum_xvblock_flag_committed);
    xassert(true == blockacct->store_block(fork_block2_next.get()));

    base::xvblock_t* block_tmp = blockacct->get_block(fork_block1_next->get_height(), fork_block1_next->get_block_hash());
    xassert(block_tmp == nullptr);
    block_tmp = blockacct->get_block(fork_block2_next->get_height(), fork_block2_next->get_block_hash());
    xassert(block_tmp != nullptr);

    lastest_cur_block = blockacct->get_latest_current_block();
    xassert(lastest_cur_block == latest_connected_block);

    // save fork_block2 to blockstore, it will replace fork_block1.get()
    fork_block2->set_block_flag(base::enum_xvblock_flag_locked);
    fork_block2->set_block_flag(base::enum_xvblock_flag_committed);
    xassert(true == blockacct->store_block(fork_block2.get()));

    block_tmp = blockacct->get_block(fork_block1->get_height(), fork_block1->get_block_hash());
    xassert(block_tmp == nullptr);
    block_tmp = blockacct->get_block(fork_block2->get_height(), fork_block2->get_block_hash());
    xassert(block_tmp != nullptr);

    lastest_cur_block = blockacct->get_latest_current_block();
    xassert(lastest_cur_block->get_height() == prevblock->get_height());
}


TEST_F(test_xvblockhub, xblockacct_high_qc_block_fork) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    xblockacct_t* blockacct = new xchainacct_t(address, 10000, "", *store, *blockstore);
    xblockacct_t::init_account(base::xcontext_t::instance());
    ASSERT_TRUE(blockacct->init());

    {
        auto block = blockacct->get_latest_committed_block();
        xassert(block->get_height() == 0);
    }

    xauto_ptr<base::xvblock_t> block0(test_blocktuil::create_genesis_empty_table(address));
    base::xvblock_t* prevblock = block0.get();
    uint64_t max_count = 10;
    uint64_t count = max_count;
    // create 10 empty block
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(true == blockacct->store_block(nextblock.get()));
        prevblock = nextblock.get();
        count--;
    } while (count > 0);

    base::xvblock_t* latest_connected_block = prevblock;

    // create a block that will fork with another one and save to blockstore
    xauto_ptr<base::xvblock_t> fork_block1(test_blocktuil::create_next_emptyblock(prevblock));
    fork_block1->reset_block_flags();
    fork_block1->set_block_flag(base::enum_xvblock_flag_authenticated);
    xassert(true == blockacct->store_block(fork_block1.get()));

    // create another block that forks with fork_block1, not save to blockstore for the moment
    xauto_ptr<base::xvblock_t> fork_block2(test_blocktuil::create_next_emptyblock(prevblock, fork_block1->get_clock() + 1));
    fork_block2->reset_block_flags();
    fork_block2->set_block_flag(base::enum_xvblock_flag_authenticated);
    prevblock = fork_block2.get();

    // create 10 block grow from fork_block2 and store
    count = max_count;
    do {
        xauto_ptr<base::xvblock_t> nextblock(test_blocktuil::create_next_emptyblock(prevblock));
        nextblock->reset_block_flags();
        nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(true == blockacct->store_block(nextblock.get()));
        prevblock = nextblock.get();
        count--;
    } while (count > 0);

    base::xvblock_t* lastest_cur_block = blockacct->get_latest_current_block();
    xassert(lastest_cur_block == latest_connected_block);

    // save fork_block2 to blockstore, it will replace fork_block1.get()
    fork_block2->set_block_flag(base::enum_xvblock_flag_locked);
    fork_block2->set_block_flag(base::enum_xvblock_flag_committed);
    xassert(true == blockacct->store_block(fork_block2.get()));

    base::xvblock_t* block_tmp = blockacct->get_block(fork_block1->get_height(), fork_block1->get_block_hash());
    xassert(block_tmp == nullptr);
    block_tmp = blockacct->get_block(fork_block2->get_height(), fork_block2->get_block_hash());
    xassert(block_tmp != nullptr);

    lastest_cur_block = blockacct->get_latest_current_block();
    xassert(lastest_cur_block->get_height() == prevblock->get_height());
}
