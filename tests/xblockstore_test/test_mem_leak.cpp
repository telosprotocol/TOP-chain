#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
// TODO(jimmy) #include "xbase/xvledger.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xdata/xtransaction_maker.hpp"

#include "test_blockmock.hpp"
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"

using namespace top;
using namespace top::base;
using namespace top::store;
using namespace top::data;

class test_mem_leak : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_mem_leak, block_mem_leak_1_BENCH) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock(genesis_block.get());
    ASSERT_TRUE(blockstore->store_block(block1.get()));
    ASSERT_EQ(block1->get_refcount(), 1 + 1 + 1);  // blockstore cache ref 1, prev pointer ref 1
    sleep(70);
    ASSERT_EQ(genesis_block->get_refcount(), 1);
    ASSERT_EQ(block1->get_refcount(), 1);
}

TEST_F(test_mem_leak, block_mem_leak_2_BENCH) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock(genesis_block.get());
    ASSERT_TRUE(blockstore->store_block(block1.get()));
    ASSERT_EQ(block1->get_refcount(), 1 + 1 + 1);  // blockstore cache ref 1, prev pointer ref 1

    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock(block1.get());
    ASSERT_TRUE(blockstore->store_block(block2.get()));
    ASSERT_EQ(block1->get_refcount(), 1 + 1 + 1 + 1);  // blockstore cache ref 1, genesis point to block1, block2 point to block1
    sleep(70);
    ASSERT_EQ(genesis_block->get_refcount(), 1);
    ASSERT_EQ(block1->get_refcount(), 1);
    ASSERT_EQ(block2->get_refcount(), 1);
}

TEST_F(test_mem_leak, block_mem_leak_3) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string to_addr = "T00000LaBamjr6xzvkJLooPNWwmwGxzVGcFg6otC";
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    xaccount_ptr_t account = make_object_ptr<xblockchain2_t>(address);
    xtransaction_ptr_t tx = xtransaction_maker::make_transfer_tx(account, to_addr, 1, 10000, 100, 1000);
    {
        xlightunit_block_para_t para;
        {
            xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
            ASSERT_TRUE(cons_tx->is_send_tx());
            para.set_one_input_tx(cons_tx);
            para.set_balance_change(-100);
        }
        ASSERT_EQ(tx->get_refcount(), 2);

        {
            base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
            base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_lightunit(para, genesis_block.get());
            ASSERT_EQ(tx->get_refcount(), 3);
            ASSERT_EQ(block1->get_refcount(), 1);
            sleep(1);
        }
        ASSERT_EQ(tx->get_refcount(), 2);
    }
    ASSERT_EQ(tx->get_refcount(), 1);
}

TEST_F(test_mem_leak, block_mem_leak_4) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string to_addr = "T00000LaBamjr6xzvkJLooPNWwmwGxzVGcFg6otC";
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    xaccount_ptr_t account = make_object_ptr<xblockchain2_t>(address);
    xtransaction_ptr_t tx = xtransaction_maker::make_transfer_tx(account, to_addr, 1, 10000, 100, 1000);
    {
        xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
        ASSERT_TRUE(cons_tx->is_send_tx());
        xlightunit_block_para_t para;
        para.set_one_input_tx(cons_tx);
        para.set_balance_change(-100);
        base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
        base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_lightunit_with_consensus(para, genesis_block.get());

        xlightunit_block_t* lightunit = dynamic_cast<xlightunit_block_t*>(block1.get());
        xcons_transaction_ptr_t recv_tx = lightunit->create_one_txreceipt(tx.get());
        ASSERT_TRUE(recv_tx->is_recv_tx());
        recv_tx->set_commit_prove_with_parent_cert(lightunit->get_cert());
        {
            xlightunit_block_para_t para2;
            para2.set_one_input_tx(recv_tx);
            para2.set_balance_change(-100);
            base::xauto_ptr<base::xvblock_t> to_genesis_block = blockstore->get_genesis_block(to_addr);
            base::xauto_ptr<base::xvblock_t> to_block1 = test_blocktuil::create_next_lightunit_with_consensus(para2, to_genesis_block.get());
        }
        ASSERT_EQ(recv_tx->get_refcount(), 1);
    }
    ASSERT_EQ(tx->get_refcount(), 1);
}

TEST_F(test_mem_leak, block_mem_leak_5) {
    // create unit1
    std::string addr1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_addr = xblocktool_t::make_address_user_account("111111111111111111113");
    base::xauto_ptr<base::xvblock_t> account1_genesis_block = xblocktool_t::create_genesis_empty_unit(addr1);
    xaccount_ptr_t account_1 = make_object_ptr<xblockchain2_t>(addr1);
    xtransaction_ptr_t tx1 = xtransaction_maker::make_transfer_tx(account_1, to_addr, 1, 10000, 100, 1000);

    // create unit2
    std::string addr2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xauto_ptr<base::xvblock_t> account2_genesis_block = xblocktool_t::create_genesis_empty_unit(addr2);
    xaccount_ptr_t account_2 = make_object_ptr<xblockchain2_t>(addr2);
    xtransaction_ptr_t tx2 = xtransaction_maker::make_transfer_tx(account_2, to_addr, 1, 10000, 100, 1000);

    {
        xlightunit_block_para_t para1;
        para1.set_one_input_tx(tx1);
        base::xauto_ptr<base::xvblock_t> lightunit1 = test_blocktuil::create_next_lightunit(para1, account1_genesis_block.get());
        xlightunit_block_para_t para2;
        para2.set_one_input_tx(tx2);
        base::xauto_ptr<base::xvblock_t> lightunit2 = test_blocktuil::create_next_lightunit(para2, account2_genesis_block.get());
        {
            xtable_block_para_t table_para;
            table_para.add_unit(lightunit1.get());
            table_para.add_unit(lightunit2.get());

            std::string taccount1 = xblocktool_t::make_address_shard_table_account(1);
            base::xauto_ptr<base::xvblock_t> taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
            base::xauto_ptr<base::xvblock_t> taccount1_proposal_block = test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block.get());
            ASSERT_NE(taccount1_proposal_block, nullptr);
            ASSERT_EQ(dynamic_cast<xblock_t*>(taccount1_proposal_block.get())->get_txs_count(), 2);
            ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());

            xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block.get());
            auto & cache_units = tableblock->get_tableblock_units(false);
            for (auto & cache_unit : cache_units) {
                ASSERT_EQ(cache_unit->get_refcount(), 1);  // no unit cache
            }
        }
        ASSERT_EQ(lightunit1->get_refcount(), 1);
        ASSERT_EQ(lightunit2->get_refcount(), 1);
    }
    ASSERT_EQ(tx1->get_refcount(), 1);
    ASSERT_EQ(tx2->get_refcount(), 1);
}

#if 0
TEST_F(test_mem_leak, perf_leak_3000000_1_times) {
    {
        std::string addr1 = xblocktool_t::make_address_user_account("11111111111111111111");
        std::string to_addr = xblocktool_t::make_address_user_account("111111111111111111113");

        int64_t free_memory_size = 0;
        base::xsys_utl::get_memory_load(free_memory_size);
        std::cout << "begin to test. free_memory_size = " << free_memory_size << std::endl;

        uint32_t out_count = 0;
        uint32_t out_total_count = 1;
        while (out_count < out_total_count) {
            out_count++;
            std::vector<xtransaction_ptr_t> datas;
            uint32_t count = 0;
            uint32_t total_count = 3000000;
            while (count++ < total_count) {
                xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
                data::xproperty_asset asset(100);
                tx->make_tx_transfer(asset);
                tx->set_last_trans_hash_and_nonce({}, 0);
                tx->set_different_source_target_address(addr1, to_addr);
                tx->set_fire_and_expire_time(100);
                tx->set_deposit(100000);
                tx->set_digest();
                tx->set_len();
                datas.push_back(tx);
            }
            sleep(10);
            base::xsys_utl::get_memory_load(free_memory_size);
            std::cout << "created units count " << total_count << " free_memory_size = " << free_memory_size << std::endl;
        }
        sleep(2);
        int res = malloc_trim(0);
        if (1 == res) {
            std::cout << "memory cleaned__" << std::endl;
        } else {
            std::cout << "memory not cleaned__" << std::endl;
        }
        base::xsys_utl::get_memory_load(free_memory_size);
        std::cout << "finish test count free_memory_size = " << free_memory_size << std::endl;
        sleep(60);
    }
}

TEST_F(test_mem_leak, perf_leak_tx_object_ptr_2000000_5_times) {
    {
        std::string addr1 = xblocktool_t::make_address_user_account("11111111111111111111");
        std::string to_addr = xblocktool_t::make_address_user_account("111111111111111111113");

        int64_t free_memory_size = 0;
        base::xsys_utl::get_memory_load(free_memory_size);
        std::cout << "begin to test. free_memory_size = " << free_memory_size << std::endl;

        uint32_t out_count = 0;
        uint32_t out_total_count = 5;
        while (out_count < out_total_count) {
            std::cout << "count = " << out_count << std::endl;
            out_count++;
            std::vector<xtransaction_ptr_t> datas;
            uint32_t count = 0;
            uint32_t total_count = 2000000;
            while (count++ < total_count) {
                xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
                data::xproperty_asset asset(100);
                tx->make_tx_transfer(asset);
                tx->set_last_trans_hash_and_nonce({}, 0);
                tx->set_different_source_target_address(addr1, to_addr);
                tx->set_fire_and_expire_time(100);
                tx->set_deposit(100000);
                tx->set_digest();
                tx->set_len();
                datas.push_back(tx);
            }
            // sleep(10);
            // base::xsys_utl::get_memory_load(free_memory_size);
            // std::cout << "created units count " << total_count << " free_memory_size = " << free_memory_size << std::endl;
        }
        // sleep(2);
        // base::xsys_utl::get_memory_load(free_memory_size);
        // std::cout << "finish test count free_memory_size = " << free_memory_size << std::endl;
        // sleep(60);
    }
}
#endif

