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
#include "xdata/xverifier/xtx_verifier.h"
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
        ASSERT_NE(block_event.get(), nullptr);
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
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 19;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
}

TEST_F(test_block_store_load, load_unexsit_block_1) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    std::string _test_addr = xdatamock_address::make_user_address_random(1);
    auto _block = blockstore->load_unit(base::xvaccount_t(_test_addr), 0);
    ASSERT_EQ(_block, nullptr);
}
TEST_F(test_block_store_load, load_unexsit_block_2) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(5, blockstore);
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
        // ASSERT_TRUE(_block->is_output_ready(true));
        // ASSERT_TRUE(_block->is_input_ready(true));
    }
    {
        auto _block = blockstore->load_block_object(mocktable, 1, 0, true);
        ASSERT_NE(_block, nullptr);
        ASSERT_EQ(_block->is_output_ready(true), true);
        ASSERT_EQ(_block->is_input_ready(true), true);
    }    
}
TEST_F(test_block_store_load, store_genesis_unit) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(20, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units(); 
    base::xvaccount_t _vaccount(mockunits[0].get_account());
    {
        for (auto & mockunit : mockunits) {
            base::xvaccount_t _vaccount(mockunit.get_account());
            auto _block = blockstore->load_unit(_vaccount, 0);
            ASSERT_NE(_block, nullptr);
        }
    }
    {
        const std::vector<xblock_ptr_t> & unitblocks = mockunits[0].get_history_units();
        bool ret = blockstore->store_unit(_vaccount, unitblocks[0].get());
        ASSERT_EQ(ret, true);        
    }
    {
        auto _block = blockstore->load_unit(_vaccount, 0);
        ASSERT_NE(_block, nullptr);
        xassert(_block->get_block_version() == base::xvblock_fork_t::get_block_init_version());
        xassert(_block->get_block_level() == base::enum_xvblock_level_unit);
        xassert(_block->get_block_class() != base::enum_xvblock_class_nil);
        ASSERT_EQ(_block->is_output_ready(true), true);
        ASSERT_EQ(_block->is_input_ready(true), true);
    }
}

TEST_F(test_block_store_load, store_batch_units) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(20, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units(); 
    {
        for (auto & mockunit : mockunits) {
            base::xvaccount_t _vaccount(mockunit.get_account());
            auto _block = blockstore->load_unit(_vaccount, 1);
            ASSERT_EQ(_block, nullptr);
        }
    }
    {
        const std::vector<xblock_ptr_t> & unitblocks = mockunits[0].get_history_units();
        tableblocks[1]->set_block_flag(base::enum_xvblock_flag_committed);
        bool ret = blockstore->store_units(tableblocks[1].get());
        ASSERT_EQ(ret, true);        
    }
    {
        for (auto & mockunit : mockunits) {
            base::xvaccount_t _vaccount(mockunit.get_account());
            auto _block = blockstore->load_unit(_vaccount, 1);
            ASSERT_NE(_block, nullptr);
            xassert(_block->get_block_version() == base::xvblock_fork_t::get_block_fork_new_version());
            xassert(_block->get_block_level() == base::enum_xvblock_level_unit);
            xassert(_block->get_block_class() == base::enum_xvblock_class_nil);
            ASSERT_EQ(_block->is_output_ready(true), true);
            ASSERT_EQ(_block->is_input_ready(true), true);
            ASSERT_TRUE(_block->check_block_flag(base::enum_xvblock_flag_committed));
        }
    }
}

TEST_F(test_block_store_load, simple_unit_check) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(20, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();    

    const std::vector<xblock_ptr_t> & unitblocks = mockunits[0].get_history_units();
    xassert(unitblocks.size() > 2);
    // genesis unit check
    {
        auto & genesis_unit = unitblocks[0];
        ASSERT_EQ(genesis_unit->get_block_class(), base::enum_xvblock_class_light);
        ASSERT_EQ(genesis_unit->get_block_version(), enum_xvblock_fork_version_init);
        std::error_code ec;
        ASSERT_NE(genesis_unit->load_input(ec), nullptr);
        ASSERT_NE(genesis_unit->load_output(ec), nullptr);
        ASSERT_EQ(genesis_unit->is_output_ready(true), true);
        ASSERT_EQ(genesis_unit->is_input_ready(true), true);      
        ASSERT_NE(genesis_unit->get_input_data().size(), 0);
        ASSERT_NE(genesis_unit->get_output_data().size(), 0);          

        std::string block_bin;
        genesis_unit->serialize_to_string(block_bin);
        xassert(block_bin.size() < 600);  //genesis block bin 499
    }
    {
        for (uint32_t i=1;i<unitblocks.size();i++) {
            auto & _unit = unitblocks[i];
            ASSERT_EQ(_unit->get_block_class(), base::enum_xvblock_class_nil);
            ASSERT_EQ(_unit->get_block_version(), base::xvblock_fork_t::get_block_fork_new_version());
            std::error_code ec;
            ASSERT_EQ(_unit->load_input(ec), nullptr);
            ASSERT_EQ(_unit->load_output(ec), nullptr);
            ASSERT_EQ(_unit->is_output_ready(true), true);
            ASSERT_EQ(_unit->is_input_ready(true), true);    
            ASSERT_EQ(_unit->get_input_data().size(), 0);
            ASSERT_EQ(_unit->get_output_data().size(), 0);   
            ASSERT_NE(_unit->get_cert()->get_parent_block_height(), 0);            
            ASSERT_NE(_unit->get_cert()->get_viewid(), 0);
            ASSERT_NE(_unit->get_cert()->get_clock(), 0);
            ASSERT_EQ(_unit->get_cert()->get_parent_block_viewid(), 0);
            ASSERT_EQ(_unit->get_cert()->get_viewtoken(), 0);
            ASSERT_EQ(_unit->get_cert()->get_drand_height(), 0);
            ASSERT_TRUE(is_xip2_empty(_unit->get_cert()->get_auditor()));
            ASSERT_TRUE(is_xip2_empty(_unit->get_cert()->get_validator()));
            ASSERT_EQ(_unit->get_cert()->get_auditor_threshold(), 0);
            ASSERT_EQ(_unit->get_cert()->get_validator_threshold(), 0);
            ASSERT_EQ(_unit->get_cert()->get_audit_signature(), std::string());
            ASSERT_EQ(_unit->get_cert()->get_verify_signature(), std::string());
            ASSERT_EQ(_unit->get_cert()->get_extend_cert(), std::string());
            ASSERT_EQ(_unit->get_cert()->get_extend_data(), std::string());
            ASSERT_EQ(_unit->get_cert()->get_expired(), _unit->get_cert()->get_clock());
            ASSERT_NE(_unit->get_cert()->get_gmtime(), 0);
            ASSERT_EQ(_unit->get_cert()->get_nonce(), 0);
            ASSERT_EQ(_unit->get_cert()->get_header_hash(), std::string());
            ASSERT_EQ(_unit->get_cert()->get_input_root_hash(), std::string());
            ASSERT_EQ(_unit->get_cert()->get_output_root_hash(), std::string());
            ASSERT_EQ(_unit->get_cert()->get_justify_cert_hash(), std::string());
            ASSERT_NE(_unit->build_block_hash(), std::string());
            ASSERT_EQ(_unit->get_cert()->get_consensus_type(), base::enum_xconsensus_type_none);
            ASSERT_EQ(_unit->get_cert()->get_consensus_threshold(), base::enum_xconsensus_threshold_anyone);
            ASSERT_EQ(_unit->get_cert()->get_consensus_flags(), base::enum_xconsensus_flag_simple_cert);
            ASSERT_FALSE(_unit->get_cert()->is_consensus_flag_has_extend_cert());
            ASSERT_EQ(_unit->get_cert()->get_crypto_key_type(), base::enum_xvchain_key_curve_secp256k1);
            ASSERT_EQ(_unit->get_cert()->get_crypto_sign_type(), base::enum_xvchain_threshold_sign_scheme_none);
            ASSERT_EQ(_unit->get_cert()->get_crypto_hash_type(), enum_xhash_type_sha2_256);

            ASSERT_NE(_unit->get_header()->get_account(), std::string());
            ASSERT_EQ(_unit->get_header()->get_height(), i);
            ASSERT_EQ(_unit->get_header()->get_block_characters(), base::enum_xvblock_character_simple_unit | base::enum_xvblock_character_certify_header_only);
            ASSERT_EQ(_unit->get_header()->get_block_class(), base::enum_xvblock_class_nil);
            ASSERT_EQ(_unit->get_header()->get_block_level(), base::enum_xvblock_level_unit);
            xassert(_unit->get_header()->get_block_type() == base::enum_xvblock_type_lightunit || _unit->get_header()->get_block_type() == base::enum_xvblock_type_fullunit);
            ASSERT_NE(_unit->get_header()->get_chainid(), 0);
            ASSERT_TRUE(_unit->get_header()->get_input_hash().empty());
            ASSERT_TRUE(_unit->get_header()->get_output_hash().empty());
            ASSERT_TRUE(_unit->get_header()->get_comments().empty());
            ASSERT_FALSE(_unit->get_header()->get_extra_data().empty());
            ASSERT_TRUE(_unit->get_header()->get_last_full_block_hash().empty());
            ASSERT_EQ(_unit->get_header()->get_weight(), 1);
            ASSERT_EQ(_unit->get_header()->get_last_full_block_height(), 0);

            std::string block_bin;
            _unit->serialize_to_string(block_bin); 
            xassert(block_bin.size() < 300); // block bin 279
        }
    }
}

TEST_F(test_block_store_load, load_units_BENCH) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 1000;
    uint32_t user_count = 20;
    uint16_t tableid = 1;
    mock::xdatamock_table mocktable(tableid, user_count);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    {
        auto start_time = std::chrono::system_clock::now();
        for (auto & block : tableblocks) {
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
        }

        for (uint64_t i = 1; i < max_block_height-2; i++) {
            blockstore->store_units(tableblocks[i].get());
        }
        auto end_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << " store all blocks milliseconds " << duration.count() << std::endl;
    }

    {
        auto start_time = std::chrono::system_clock::now();
        for (auto & mockunit : mockunits) {
            uint64_t unit_height = mockunit.get_cert_block()->get_height();
            for (uint64_t height = 1; height <= unit_height-2; height++) {
                base::xvaccount_t _vaddr(mockunit.get_account());
                auto _block = blockstore->load_unit(_vaddr, height);
                ASSERT_TRUE(_block != nullptr);
            }
        }
        auto end_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << " load all blocks milliseconds " << duration.count() << std::endl;
    }

}


TEST_F(test_block_store_load, mock_table_unit_1) {
    mock::xvchain_creator creator(true);
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
    for (uint32_t i = 0; i < confirm_txs.size(); i++) {
        xassert(confirm_txs[i]->is_confirm_tx());
        confirm_txs[i]->set_raw_tx(send_txs[i]->get_transaction());
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
    mock::xvchain_creator creator(true);
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
    mock::xvchain_creator creator(true);
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
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[3];

    for (uint32_t i = 0; i < 10; i++) {
        std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
        for (auto & tx : send_txs) {
            // ASSERT_EQ(0, xtx_verifier::verify_tx_signature(tx->get_transaction()));
            ASSERT_EQ(0, xtx_verifier::verify_send_tx_validation(tx->get_transaction()));
            // std::cout << "tx = " << tx->dump() << std::endl;
        }
        mocktable.push_txs(send_txs);
        xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    }
}


TEST_F(test_block_store_load, unit_unpack_repeat_check_BENCH) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10, blockstore);
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
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(200, blockstore);
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
    sleep((enum_block_plugin_idle_timeout_ms + enum_timer_check_interval)/1000+1);
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
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10, blockstore);
    auto table_blocks = mocktable.get_history_tables();

    for (auto & block : table_blocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    auto bindex = blockstore->load_block_index(mocktable, 3, base::enum_xvblock_flag_committed);
    ASSERT_NE(bindex, nullptr);

    mbus::xevent_ptr_t _event = creator.get_mbus()->create_event_for_store_committed_block(bindex.get());
    ASSERT_TRUE(_event != nullptr);
    ASSERT_EQ(_event->get_refcount(), 1);


    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&test_block_store_load::test_on_db_event, this, std::placeholders::_1));

    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store,
            [&](const top::mbus::xevent_ptr_t& e) {

        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
        ASSERT_TRUE(block_event != nullptr);
        ASSERT_EQ(block_event->get_refcount(), 2);

        auto block = mbus::extract_block_from(block_event, 0);
        ASSERT_NE(block.get(), nullptr);
    });

    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store,
            [&](const top::mbus::xevent_ptr_t& e) {

        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
        ASSERT_NE(block_event.get(), nullptr);
        ASSERT_EQ(block_event->get_refcount(), 2);

        auto block = mbus::extract_block_from(block_event, 0);
        ASSERT_NE(block.get(), nullptr);
    });

    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store,
            [&](const top::mbus::xevent_ptr_t& e) {

        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
        ASSERT_NE(block_event.get(), nullptr);
        ASSERT_EQ(block_event->get_refcount(), 2);

        auto block = mbus::extract_block_from(block_event, 0);
        ASSERT_NE(block.get(), nullptr);
    });

    creator.get_mbus()->push_event(_event);

    ASSERT_EQ(_event->get_refcount(), 1);
}

TEST_F(test_block_store_load, commit_block_event_2) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10, blockstore);
    auto table_blocks = mocktable.get_history_tables();

    for (auto & block : table_blocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    auto bindex = blockstore->load_block_index(mocktable, 3, base::enum_xvblock_flag_committed);
    ASSERT_NE(bindex, nullptr);

    mbus::xevent_ptr_t _event = creator.get_mbus()->create_event_for_store_committed_block(bindex.get());
    ASSERT_NE(_event.get(), nullptr);
    ASSERT_EQ(_event->get_refcount(), 1);


    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&test_block_store_load::test_on_db_event2, this, std::placeholders::_1));

    creator.get_mbus()->push_event(_event);

    sleep(2);

    ASSERT_EQ(_event->get_refcount(), 2);  // TODO(jimmy) fail
}

TEST_F(test_block_store_load, commit_block_event_3) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10, blockstore);
    auto table_blocks = mocktable.get_history_tables();

    for (auto & block : table_blocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    auto bindex = blockstore->load_block_index(mocktable, 3, base::enum_xvblock_flag_committed);
    ASSERT_NE(bindex, nullptr);

    mbus::xevent_ptr_t _event = creator.get_mbus()->create_event_for_store_committed_block(bindex.get());
    ASSERT_NE(_event.get(), nullptr);
    ASSERT_EQ(_event->get_refcount(), 1);


    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&test_block_store_load::test_on_db_event3, this, std::placeholders::_1));

    creator.get_mbus()->push_event(_event);

    sleep(2);

    ASSERT_EQ(_event->get_refcount(), 1);
}

TEST_F(test_block_store_load, commit_block_event_4) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(10, blockstore);
    auto table_blocks = mocktable.get_history_tables();

    for (auto & block : table_blocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    auto bindex = blockstore->load_block_index(mocktable, 3, base::enum_xvblock_flag_committed);
    ASSERT_NE(bindex, nullptr);

    mbus::xevent_ptr_t _event = creator.get_mbus()->create_event_for_store_committed_block(bindex.get());
    ASSERT_NE(_event.get(), nullptr);
    ASSERT_EQ(_event->get_refcount(), 1);


    creator.get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&test_block_store_load::test_on_db_event1, this, std::placeholders::_1));

    creator.get_mbus()->push_event(_event);

    ASSERT_EQ(_event->get_refcount(), 1);
}
