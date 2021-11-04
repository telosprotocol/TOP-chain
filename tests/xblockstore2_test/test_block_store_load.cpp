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
#include "xmbus/xevent_store.h"
#include "xmbus/xmessage_bus.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xmetrics/xmetrics.h"

// #include "test_blockmock.hpp"
#include "xstore/xstore.h"
#include "xverifier/xtx_verifier.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::xverifier;
using namespace top::mock;
using namespace top::metrics;

class test_event_dispatcher_t
  : public base::xiobject_t {
public:
    test_event_dispatcher_t(base::xcontext_t & _context, int32_t thread_id)
      : base::xiobject_t(_context, thread_id, base::enum_xobject_type_woker) {
    }

protected:
    ~test_event_dispatcher_t() override {
    }
};

class test_block_store_load : public testing::Test {
public:



    void test_on_db_event(mbus::xevent_ptr_t e) {
        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
        ASSERT_NE(block_event, nullptr);
        ASSERT_EQ(block_event->get_refcount(), 3);    
    }

    void test_on_db_event1(mbus::xevent_ptr_t e) {
        auto event_handler = [this, e](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
            mbus::xevent_object_t * event_obj = dynamic_cast<mbus::xevent_object_t *>(call.get_param1().get_object());
            return true;
        };

        {
            base::xauto_ptr<mbus::xevent_object_t> event_obj = new mbus::xevent_object_t(e, 0);
            base::xcall_t asyn_call(event_handler, event_obj.get());
        }
    }

    void test_on_db_event2(mbus::xevent_ptr_t e) {
        auto event_handler = [this, e](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
            xdbg("test_on_db_event2 e refcount %d", e->get_refcount());
            return true;
        };

        base::xcall_t asyn_call(event_handler);
        m_dispatcher->send_call(asyn_call);
    }

    void test_on_db_event3(mbus::xevent_ptr_t e) {
        auto event_handler = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
            mbus::xevent_object_t * event_obj = dynamic_cast<mbus::xevent_object_t *>(call.get_param1().get_object());
            xdbg("test_on_db_event3 e refcount %d", event_obj->event->get_refcount());
            return true;
        };

        base::xauto_ptr<mbus::xevent_object_t> event_obj = new mbus::xevent_object_t(e, 0);
        base::xcall_t asyn_call(event_handler, event_obj.get());
        m_dispatcher->send_call(asyn_call);
    }    
protected:
    void SetUp() override {
        m_thread = make_object_ptr<base::xiothread_t>();
        m_dispatcher = make_object_ptr<test_event_dispatcher_t>(base::xcontext_t::instance(), m_thread->get_thread_id());    
    }

    void TearDown() override {
    }  

    xobject_ptr_t<base::xiothread_t> m_thread;
    xobject_ptr_t<test_event_dispatcher_t> m_dispatcher;
};

TEST_F(test_block_store_load, store_batch_tables) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 19;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
}

TEST_F(test_block_store_load, load_unexsit_block_1) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    std::string _test_addr = xdatamock_address::make_user_address_random(1);
    auto _block = blockstore->load_block_object(base::xvaccount_t(_test_addr), 0, 0, false);
    ASSERT_NE(_block, nullptr);  // TODO(jimmy) blockstore always will return genesis block
}
TEST_F(test_block_store_load, load_unexsit_block_2) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(5);
    {
        auto _block = blockstore->load_block_object(mocktable, 1, 0, false);
        ASSERT_EQ(_block, nullptr);
    }
    {
        const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
        bool ret = blockstore->store_block(mocktable, tableblocks[1].get());
        ASSERT_EQ(ret, true);        
    }
    {
        auto _block = blockstore->load_block_object(mocktable, 1, 0, false);
        ASSERT_NE(_block, nullptr);
        ASSERT_EQ(_block->is_output_ready(true), false);
        ASSERT_EQ(_block->is_input_ready(true), false);
    }
    {
        auto _block = blockstore->load_block_object(mocktable, 1, 0, true);
        ASSERT_NE(_block, nullptr);
        ASSERT_EQ(_block->is_output_ready(true), true);
        ASSERT_EQ(_block->is_input_ready(true), true);
    }    
}
TEST_F(test_block_store_load, load_unexsit_block_3) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(20);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();    
    {
        auto _block = blockstore->load_block_object(base::xvaccount_t(mockunits[0].get_account()), 1, 0, false);
        ASSERT_EQ(_block, nullptr);
    }
    {
        const std::vector<xblock_ptr_t> & unitblocks = mockunits[0].get_history_units();
        bool ret = blockstore->store_block(base::xvaccount_t(mockunits[0].get_account()), unitblocks[1].get());
        ASSERT_EQ(ret, true);        
    }
    {
        auto _block = blockstore->load_block_object(base::xvaccount_t(mockunits[0].get_account()), 1, 0, false);
        ASSERT_NE(_block, nullptr);
        ASSERT_EQ(_block->is_output_ready(true), false);
        ASSERT_EQ(_block->is_input_ready(true), true);
    }
    {
        auto _block = blockstore->load_block_object(base::xvaccount_t(mockunits[0].get_account()), 1, 0, true);
        ASSERT_NE(_block, nullptr);
        ASSERT_EQ(_block->is_output_ready(true), true);
        ASSERT_EQ(_block->is_input_ready(true), true);
    }    
}

TEST_F(test_block_store_load, load_units_BENCH) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 1000;
    uint32_t user_count = 20;
    uint16_t tableid = 1;
    mock::xdatamock_table mocktable(tableid, user_count);
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    {
        auto start_time = std::chrono::system_clock::now();
        for (auto & block : tableblocks) {
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
        }
        auto end_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << " store all blocks milliseconds " << duration.count() << std::endl;
    }

    {
        auto start_time = std::chrono::system_clock::now();
        for (auto & mockunit : mockunits) {
            uint64_t unit_height = mockunit.get_cert_block()->get_height();
            for (uint64_t height = 1; height <= unit_height; height++) {
                base::xvaccount_t _vaddr(mockunit.get_account());
                auto _block = blockstore->load_block_object(_vaddr, height, 0, false);
                blockstore->load_block_input(_vaddr, _block.get());
                blockstore->load_block_output(_vaddr, _block.get());
            }
        }
        auto end_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << " load all blocks milliseconds " << duration.count() << std::endl;
    }

}


TEST_F(test_block_store_load, mock_table_unit_1) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[3];

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();
    {
        xassert(_tableblock1->get_height() == 1);
        xassert(_tableblock1->get_block_class() == base::enum_xvblock_class_light);
        std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
        _tableblock1->extract_sub_blocks(sub_blocks);
        xassert(sub_blocks.size() == 1);
    }

    std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
    xassert(recv_txs.size() == send_txs.size());
    for (auto & tx : recv_txs) {
        xassert(tx->is_recv_tx());
    }
    mocktable.push_txs(recv_txs);    
    xblock_ptr_t _tableblock2 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();    
    {
        xassert(_tableblock2->get_height() == 4);
        xassert(_tableblock2->get_block_class() == base::enum_xvblock_class_light);
        std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
        _tableblock2->extract_sub_blocks(sub_blocks);
        xassert(sub_blocks.size() == 1);
    }

    std::vector<xcons_transaction_ptr_t> confirm_txs = mocktable.create_receipts(_tableblock2);
    xassert(confirm_txs.size() == send_txs.size());
    for (auto & tx : confirm_txs) {
        xassert(tx->is_confirm_tx());
    }
    mocktable.push_txs(confirm_txs); 
    xblock_ptr_t _tableblock3 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();
    {
        xassert(_tableblock3->get_height() == 7);
        xassert(_tableblock3->get_block_class() == base::enum_xvblock_class_light);
        std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
        _tableblock3->extract_sub_blocks(sub_blocks);
        xassert(sub_blocks.size() == 1);
    }    
}

TEST_F(test_block_store_load, mock_table_unit_2) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[3];

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
    xassert(recv_txs.size() == send_txs.size());
    for (auto & tx : recv_txs) {
        xassert(tx->is_recv_tx());
    }
    mocktable.push_txs(recv_txs);    
    xblock_ptr_t _tableblock2 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();    

    std::vector<xcons_transaction_ptr_t> confirm_txs = mocktable.create_receipts(_tableblock2);
    xassert(confirm_txs.size() == send_txs.size());
    for (auto & tx : confirm_txs) {
        xassert(tx->is_confirm_tx());
    }
    mocktable.push_txs(confirm_txs); 
    xblock_ptr_t _tableblock3 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();
}

void print_store_metrics(const db::xdb_meta_t & db_meta) {
    // db write count statistics
    std::cout << "=============db write count statistics=============" << std::endl;
    #ifdef ENABLE_METRICS
    std::cout << "db_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_write) << std::endl;
    std::cout << "store_block_index_table_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_index_table_write) << std::endl;
    std::cout << "store_block_index_unit_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_index_unit_write) << std::endl;
    std::cout << "store_block_index_other_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_index_other_write) << std::endl;
    std::cout << "store_block_table_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_table_write) << std::endl;
    std::cout << "store_block_unit_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_unit_write) << std::endl;
    std::cout << "store_block_other_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_other_write) << std::endl;
    std::cout << "store_block_input_table_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_input_table_write) << std::endl;
    std::cout << "store_block_input_unit_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_input_unit_write) << std::endl;
    std::cout << "store_block_output_table_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_output_table_write) << std::endl;
    std::cout << "store_block_output_unit_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_output_unit_write) << std::endl;
    std::cout << "store_state_table_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_state_table_write) << std::endl;
    std::cout << "store_state_unit_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_state_unit_write) << std::endl;
    std::cout << "store_tx_index_self=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_tx_index_self) << std::endl;
    std::cout << "store_tx_index_send=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_tx_index_send) << std::endl;
    std::cout << "store_tx_index_recv=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_tx_index_recv) << std::endl;
    std::cout << "store_tx_index_confirm=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_tx_index_confirm) << std::endl;
    std::cout << "store_block_meta_write=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_meta_write) << std::endl;        

    // db read count statistics
    std::cout << "=============db read count statistics=============" << std::endl;
    std::cout << "db_read=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_read) << std::endl;
    std::cout << "store_block_index_read=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_index_read) << std::endl;
    std::cout << "store_block_table_read=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_table_read) << std::endl;
    std::cout << "store_block_unit_read=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_unit_read) << std::endl;
    std::cout << "store_block_other_read=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_other_read) << std::endl;
    std::cout << "store_block_input_read=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_input_read) << std::endl;
    std::cout << "store_block_output_read=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_output_read) << std::endl;
    std::cout << "store_block_meta_read=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_block_meta_read) << std::endl;

    // db delete count statistics       
    std::cout << "=============db delete count statistics=============" << std::endl; 
    std::cout << "db_delete=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_delete) << std::endl;
    std::cout << "store_state_delete=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_state_delete) << std::endl;

    // db size statistics
    std::cout << "=============db size statistics=============" << std::endl; 
    std::cout << "db_key_size=" << db_meta.m_db_key_size << std::endl;
    std::cout << "db_value_size=" << db_meta.m_db_value_size << std::endl;
    std::cout << "key_count=" << db_meta.m_key_count << std::endl;

    std::cout << "store_dbsize_block_unit_empty=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_dbsize_block_unit_empty) << std::endl;
    std::cout << "store_dbsize_block_unit_light=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_dbsize_block_unit_light) << std::endl;
    std::cout << "store_dbsize_block_unit_full=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_dbsize_block_unit_full) << std::endl;
    std::cout << "store_dbsize_block_table_empty=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_dbsize_block_table_empty) << std::endl;
    std::cout << "store_dbsize_block_table_light=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_dbsize_block_table_light) << std::endl;
    std::cout << "store_dbsize_block_table_full=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_dbsize_block_table_full) << std::endl;
    std::cout << "store_dbsize_block_other=" << XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::store_dbsize_block_other) << std::endl;
    #endif
}

TEST_F(test_block_store_load, mock_key_value_size_analyze_1_BENCH) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint32_t addr_count = 2;
    mock::xdatamock_table mocktable(1, addr_count);
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    mocktable.disable_fulltable();

    uint64_t count = 200;
    uint32_t addr_index = 0;
    uint32_t tx_count = 1;
    for (uint64_t i = 0; i < count; i++)
    {
        std::string from_addr = unit_addrs[addr_index];
        std::string to_addr = unit_addrs[(addr_index+1) % addr_count];
        addr_index = (addr_index+1) % addr_count;

        std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, tx_count);
        mocktable.push_txs(send_txs);
        xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
        mocktable.generate_one_table();
        mocktable.generate_one_table();

        std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
        xassert(recv_txs.size() == send_txs.size());
        for (auto & tx : recv_txs) {
            xassert(tx->is_recv_tx());
        }
        mocktable.push_txs(recv_txs);    
        xblock_ptr_t _tableblock2 = mocktable.generate_one_table();
        mocktable.generate_one_table();
        mocktable.generate_one_table();    

        std::vector<xcons_transaction_ptr_t> confirm_txs = mocktable.create_receipts(_tableblock2);
        xassert(confirm_txs.size() == send_txs.size());
        for (auto & tx : confirm_txs) {
            xassert(tx->is_confirm_tx());
        }
        mocktable.push_txs(confirm_txs); 
        xblock_ptr_t _tableblock3 = mocktable.generate_one_table();
        mocktable.generate_one_table();
        mocktable.generate_one_table();
    }

    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }    

    {
        db::xdb_meta_t db_meta = creator.get_xdb()->get_meta();
        print_store_metrics(db_meta);
    }
}

TEST_F(test_block_store_load, mock_key_value_size_analyze_2_BENCH) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint32_t addr_count = 20;
    mock::xdatamock_table mocktable(1, addr_count);
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    mocktable.disable_fulltable();

    uint64_t count = 200;
    uint32_t addr_index = 0;
    uint32_t tx_count = 1;
    for (uint64_t i = 0; i < count; i++)
    {
        std::vector<xcons_transaction_ptr_t> send_txs;
        for (uint32_t j = 0 ; j < addr_count/2; j++) {
            std::string from_addr = unit_addrs[addr_index];
            std::string to_addr = unit_addrs[(addr_index+1) % addr_count];
            addr_index = (addr_index+1) % addr_count;

            std::vector<xcons_transaction_ptr_t> one_account_send_txs = mocktable.create_send_txs(from_addr, to_addr, tx_count);
            for (auto & v : one_account_send_txs) {
                send_txs.push_back(v);
            }            
        }

        mocktable.push_txs(send_txs);
        xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
        mocktable.generate_one_table();
        mocktable.generate_one_table();

        std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
        xassert(recv_txs.size() == send_txs.size());
        for (auto & tx : recv_txs) {
            xassert(tx->is_recv_tx());
        }
        mocktable.push_txs(recv_txs);    
        xblock_ptr_t _tableblock2 = mocktable.generate_one_table();
        mocktable.generate_one_table();
        mocktable.generate_one_table();    

        std::vector<xcons_transaction_ptr_t> confirm_txs = mocktable.create_receipts(_tableblock2);
        xassert(confirm_txs.size() == send_txs.size());
        for (auto & tx : confirm_txs) {
            xassert(tx->is_confirm_tx());
        }
        mocktable.push_txs(confirm_txs); 
        xblock_ptr_t _tableblock3 = mocktable.generate_one_table();
        mocktable.generate_one_table();
        mocktable.generate_one_table();
    }

    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }    

    {
        db::xdb_meta_t db_meta = creator.get_xdb()->get_meta();
        print_store_metrics(db_meta);
    }
}


TEST_F(test_block_store_load, mock_table_tx_check) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[3];

    for (uint32_t i = 0; i < 10; i++) {
        std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
        for (auto & tx : send_txs) {
            ASSERT_EQ(0, xtx_verifier::verify_tx_signature(tx->get_transaction(), nullptr));
            ASSERT_EQ(0, xtx_verifier::verify_send_tx_validation(tx->get_transaction()));
            // std::cout << "tx = " << tx->dump() << std::endl;
        }
        mocktable.push_txs(send_txs);
        xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    }
}


TEST_F(test_block_store_load, unit_unpack_repeat_check_BENCH) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10);
    auto table_blocks = mocktable.get_history_tables();
    auto test_block = table_blocks[3];

    // store first tableblock
    test_block->reset_block_flags();
    test_block->set_block_flag(base::enum_xvblock_flag_authenticated);
    ASSERT_TRUE(blockstore->store_block(mocktable, test_block.get()));

    db::xdb_meta_t db_meta = creator.get_xdb()->get_meta();
    #ifdef ENABLE_METRICS
    auto store_call_1 = XMETRICS_GAUGE_GET_VALUE(metrics::store_block_call);
    #endif

    // store the same tableblock with 1000 times repeatly
    for (int i = 0; i < 1000; i++) {
        test_block->reset_block_flags();
        test_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        ASSERT_TRUE(blockstore->store_block(mocktable, test_block.get()));
    }
    db::xdb_meta_t db_meta2 = creator.get_xdb()->get_meta();
    ASSERT_EQ(db_meta.m_write_count, db_meta2.m_write_count);
    std::cout << "db write count = " << db_meta.m_write_count << std::endl;

    #ifdef ENABLE_METRICS
    auto store_call_2 = XMETRICS_GAUGE_GET_VALUE(metrics::store_block_call);
    auto call_sub = store_call_2 - store_call_1;
    ASSERT_EQ(call_sub, 1000);
    std::cout << "store_call_2 = " << store_call_2 << std::endl;
    #endif
}

TEST_F(test_block_store_load, unit_unpack_repeat_check_2_BENCH) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(200);
    auto table_blocks = mocktable.get_history_tables();

    // blockstore->reset_cache_timeout(mocktable, 1000); // idle time change to 1s

    // store first tableblock
    for (int i = 0; i < 200; i++) {
        auto test_block = table_blocks[i];
        if (test_block->get_height() > 0) {
            test_block->reset_block_flags();
            test_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        }
        ASSERT_TRUE(blockstore->store_block(mocktable, test_block.get()));
    }

    //sleep(1*16+5); // wait for meta save to db. table has 16 times than unit
    sleep((enum_plugin_idle_timeout_ms + enum_plugin_idle_check_interval)/1000+1);
    db::xdb_meta_t db_meta = creator.get_xdb()->get_meta();
    #ifdef ENABLE_METRICS
    auto store_call_1 = XMETRICS_GAUGE_GET_VALUE(metrics::store_block_call);
    #endif

    xdbg("==========unit_unpack_repeat_check_2_BENCH 1===========");
    // store the tableblocks with 200 times repeatly
    for (int i = 1; i < 200; i++) {  // TODO(jimmy) height=0 will invoke write to db
        auto test_block = table_blocks[i];
        test_block->reset_block_flags();
        test_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        ASSERT_TRUE(blockstore->store_block(mocktable, test_block.get()));
    }
    db::xdb_meta_t db_meta2 = creator.get_xdb()->get_meta();
    ASSERT_TRUE(db_meta.m_write_count < db_meta2.m_write_count + 2);
    std::cout << "db write count = " << db_meta.m_write_count << std::endl;

    #ifdef ENABLE_METRICS
    auto store_call_2 = XMETRICS_GAUGE_GET_VALUE(metrics::store_block_call);
    auto call_sub = store_call_2 - store_call_1;
    ASSERT_EQ(call_sub, 200-1);
    std::cout << "store_call_2 = " << store_call_2 << std::endl;
    #endif
}

TEST_F(test_block_store_load, commit_block_event_1) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10);
    auto table_blocks = mocktable.get_history_tables();

    for (auto & block : table_blocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    auto bindex = blockstore->load_block_index(mocktable, 3, base::enum_xvblock_flag_committed);
    ASSERT_NE(bindex, nullptr);

    mbus::xevent_ptr_t _event = creator.get_mbus()->create_event_for_store_committed_block(bindex.get());
    ASSERT_NE(_event, nullptr);
    ASSERT_EQ(_event->get_refcount(), 1);


    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&test_block_store_load::test_on_db_event, this, std::placeholders::_1));

    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store,
            [&](const top::mbus::xevent_ptr_t& e) {

        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
        ASSERT_NE(block_event, nullptr);
        ASSERT_EQ(block_event->get_refcount(), 2);

        auto block = mbus::extract_block_from(block_event, 0);
        ASSERT_NE(block, nullptr);
    });

    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store,
            [&](const top::mbus::xevent_ptr_t& e) {

        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
        ASSERT_NE(block_event, nullptr);
        ASSERT_EQ(block_event->get_refcount(), 2);

        auto block = mbus::extract_block_from(block_event, 0);
        ASSERT_NE(block, nullptr);
    });

    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store,
            [&](const top::mbus::xevent_ptr_t& e) {

        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
        ASSERT_NE(block_event, nullptr);
        ASSERT_EQ(block_event->get_refcount(), 2);

        auto block = mbus::extract_block_from(block_event, 0);
        ASSERT_NE(block, nullptr);
    });

    creator.get_mbus()->push_event(_event);

    ASSERT_EQ(_event->get_refcount(), 1);
}

TEST_F(test_block_store_load, commit_block_event_2) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10);
    auto table_blocks = mocktable.get_history_tables();

    for (auto & block : table_blocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    auto bindex = blockstore->load_block_index(mocktable, 3, base::enum_xvblock_flag_committed);
    ASSERT_NE(bindex, nullptr);

    mbus::xevent_ptr_t _event = creator.get_mbus()->create_event_for_store_committed_block(bindex.get());
    ASSERT_NE(_event, nullptr);
    ASSERT_EQ(_event->get_refcount(), 1);


    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&test_block_store_load::test_on_db_event2, this, std::placeholders::_1));

    creator.get_mbus()->push_event(_event);

    sleep(2);

    ASSERT_EQ(_event->get_refcount(), 2);  // TODO(jimmy) fail
}

TEST_F(test_block_store_load, commit_block_event_3) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10);
    auto table_blocks = mocktable.get_history_tables();

    for (auto & block : table_blocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    auto bindex = blockstore->load_block_index(mocktable, 3, base::enum_xvblock_flag_committed);
    ASSERT_NE(bindex, nullptr);

    mbus::xevent_ptr_t _event = creator.get_mbus()->create_event_for_store_committed_block(bindex.get());
    ASSERT_NE(_event, nullptr);
    ASSERT_EQ(_event->get_refcount(), 1);


    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&test_block_store_load::test_on_db_event3, this, std::placeholders::_1));

    creator.get_mbus()->push_event(_event);

    sleep(2);

    ASSERT_EQ(_event->get_refcount(), 1);
}

TEST_F(test_block_store_load, commit_block_event_4) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10);
    auto table_blocks = mocktable.get_history_tables();

    for (auto & block : table_blocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    auto bindex = blockstore->load_block_index(mocktable, 3, base::enum_xvblock_flag_committed);
    ASSERT_NE(bindex, nullptr);

    mbus::xevent_ptr_t _event = creator.get_mbus()->create_event_for_store_committed_block(bindex.get());
    ASSERT_NE(_event, nullptr);
    ASSERT_EQ(_event->get_refcount(), 1);


    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&test_block_store_load::test_on_db_event1, this, std::placeholders::_1));

    creator.get_mbus()->push_event(_event);

    ASSERT_EQ(_event->get_refcount(), 1);
}
