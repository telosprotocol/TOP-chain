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
#include "test_common.hpp"
#include "xvledger/xvblock_offdata.h"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::xverifier;
using namespace top::mock;
using namespace top::metrics;

class test_block_db_size : public testing::Test {
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};
TEST_F(test_block_db_size, table_unit_size) {
// after unit optimize
// table = Ta0000@1 height=0 size = 213 object:213 input:0 output:0 offdata: 0
// table = Ta0000@1 height=1 size = 3439 object:553 input:1423 output:656 offdata: 807
// table = Ta0000@1 height=2 size = 3459 object:588 input:1423 output:641 offdata: 807
// table = Ta0000@1 height=3 size = 3498 object:621 input:1429 output:641 offdata: 807
// table = Ta0000@1 height=4 size = 3500 object:621 input:1431 output:641 offdata: 807
// table = Ta0000@1 height=5 size = 3500 object:621 input:1431 output:641 offdata: 807
// unit = T00000LWPwYdDaQDnCAKY3hBJV3Ztx2zYmAekxaB height=0 size = 1049 header:90 qcert:145 input:358 output:180 input_res:358 output_res:180 binlog:102
// unit = T00000LWPwYdDaQDnCAKY3hBJV3Ztx2zYmAekxaB height=1 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LWPwYdDaQDnCAKY3hBJV3Ztx2zYmAekxaB height=2 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LWPwYdDaQDnCAKY3hBJV3Ztx2zYmAekxaB height=3 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LWPwYdDaQDnCAKY3hBJV3Ztx2zYmAekxaB height=4 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LWPwYdDaQDnCAKY3hBJV3Ztx2zYmAekxaB height=5 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNAmUR4HcaSChs3PdY1SCkqJrtvMtKCBVX height=0 size = 1049 header:90 qcert:145 input:358 output:180 input_res:358 output_res:180 binlog:102
// unit = T00000LNAmUR4HcaSChs3PdY1SCkqJrtvMtKCBVX height=1 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNAmUR4HcaSChs3PdY1SCkqJrtvMtKCBVX height=2 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNAmUR4HcaSChs3PdY1SCkqJrtvMtKCBVX height=3 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNAmUR4HcaSChs3PdY1SCkqJrtvMtKCBVX height=4 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNAmUR4HcaSChs3PdY1SCkqJrtvMtKCBVX height=5 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LRyHY2akBduz4r8GMrr4jrNeSthBtmbCP9 height=0 size = 1049 header:90 qcert:145 input:358 output:180 input_res:358 output_res:180 binlog:102
// unit = T00000LRyHY2akBduz4r8GMrr4jrNeSthBtmbCP9 height=1 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LRyHY2akBduz4r8GMrr4jrNeSthBtmbCP9 height=2 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LRyHY2akBduz4r8GMrr4jrNeSthBtmbCP9 height=3 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LRyHY2akBduz4r8GMrr4jrNeSthBtmbCP9 height=4 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LRyHY2akBduz4r8GMrr4jrNeSthBtmbCP9 height=5 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNpUvtRERaAX4PJ57oiy87AocC2yvNgMwA height=0 size = 1049 header:90 qcert:145 input:358 output:180 input_res:358 output_res:180 binlog:102
// unit = T00000LNpUvtRERaAX4PJ57oiy87AocC2yvNgMwA height=1 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNpUvtRERaAX4PJ57oiy87AocC2yvNgMwA height=2 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNpUvtRERaAX4PJ57oiy87AocC2yvNgMwA height=3 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNpUvtRERaAX4PJ57oiy87AocC2yvNgMwA height=4 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// unit = T00000LNpUvtRERaAX4PJ57oiy87AocC2yvNgMwA height=5 size = 282 header:196 qcert:37 input:0 output:0 input_res:0 output_res:0 binlog:67
// dbmeta key_size:102042 value_size = 406348 key_count = 1506 write_count = 1550 read_count = 836
//  hash_calc_count = 224

    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    xhashtest_t::hash_calc_count = 0;
    xhashtest_t::print_hash_calc = false;
    uint64_t max_block_height = 5;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);


    xhashtest_t::print_hash_calc = false;
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));

        std::string block_object_bin;
        block->serialize_to_string(block_object_bin);
        size_t t_size = block_object_bin.size() + block->get_input_data().size() + block->get_output_data().size() + block->get_output_offdata().size();
        std::cout << "table = " <<  block->get_account() << " height=" << block->get_height() << " size = " << t_size
        << " object:" << block_object_bin.size() 
        << " input:" << block->get_input_data().size() 
        << " output:" << block->get_output_data().size() 
        << " offdata: " << block->get_output_offdata().size() << std::endl;
    }

    for (auto & datamock_unit : mocktable.get_mock_units()) {
        for (auto & unit : datamock_unit.get_history_units()) {
            std::string header_bin;
            unit->get_header()->serialize_to_string(header_bin);
            std::string cert_bin;
            unit->get_cert()->serialize_to_string(cert_bin);
            std::string input_bin = unit->get_input_data();  
            std::string output_bin = unit->get_output_data();

            base::xstream_t stream(base::xcontext_t::instance());
            unit->full_block_serialize_to(stream);
            std::cout << "unit = " <<  unit->get_account() << " height=" << unit->get_height() << " size = " << stream.size()  
            << " header:" << header_bin.size()
            << " qcert:" << cert_bin.size()
            << " input:" << input_bin.size()
            << " output:" << output_bin.size()          
            << " input_res:" << unit->get_input_data().size() 
            << " output_res:" << unit->get_output_data().size()                        
            << " binlog:" << unit->get_binlog().size() << std::endl;// TODO(jimmy) size 590-660
        }
    }

    db::xdb_meta_t dbmeta = creator.get_xdb()->get_meta();
    std::cout << "dbmeta key_size:" << dbmeta.m_db_key_size
    << " value_size = " << dbmeta.m_db_value_size
    << " key_count = " << dbmeta.m_key_count  
    << " write_count = " << dbmeta.m_write_count
    << " read_count = " << dbmeta.m_read_count
    << std::endl;
    std::cout << " hash_calc_count = " << xhashtest_t::hash_calc_count << std::endl;
    xhashtest_t::print_hash_calc = false;
}

TEST_F(test_block_db_size, table_unit_IO_opt_BENCH) {
// dbmeta key_size:849067 value_size = 5462528 key_count = 14408 write_count = 25782 read_count = 4788 erase_count = 0
// hash_calc_count = 46780    
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    xhashtest_t::hash_calc_count = 0;
    xhashtest_t::print_hash_calc = false;
    uint64_t max_block_height = 1000;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    xhashtest_t::print_hash_calc = false;
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    db::xdb_meta_t dbmeta = creator.get_xdb()->get_meta();
    std::cout << "dbmeta key_size:" << dbmeta.m_db_key_size
    << " value_size = " << dbmeta.m_db_value_size
    << " key_count = " << dbmeta.m_key_count  
    << " write_count = " << dbmeta.m_write_count
    << " read_count = " << dbmeta.m_read_count
    << " erase_count = " << dbmeta.m_erase_count
    << std::endl;
    std::cout << "hash_calc_count = " << xhashtest_t::hash_calc_count << std::endl;
    xhashtest_t::print_hash_calc = false;
}


TEST_F(test_block_db_size, table_unit_offdata_1) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    xhashtest_t::hash_calc_count = 0;
    xhashtest_t::print_hash_calc = false;
    uint64_t max_block_height = 5;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    auto tableblock = tableblocks[1];
    
    std::vector<xobject_ptr_t<xvblock_t>> sub_blocks;
    tableblock->extract_sub_blocks(sub_blocks);
    ASSERT_EQ(sub_blocks.size(), 4);

    base::xvblock_out_offdata_t offdata(sub_blocks);
    std::string offdata_bin;
    offdata.serialize_to_string(offdata_bin);
    if (false == tableblock->set_output_offdata(offdata_bin)) {        
        printf("xvblockstore_impl::load_block_output_offdata,fail-unmatch offdata. block:%s,offdata_size=%zu,offdata_hash=%s:%s",tableblock->dump().c_str(), offdata_bin.size(),
            base::xstring_utl::to_hex(tableblock->get_cert()->hash(offdata_bin)).c_str(), base::xstring_utl::to_hex(tableblock->get_output_offdata_hash()).c_str());
        xassert(false);
    }
}

TEST_F(test_block_db_size, table_unit_offdata_2) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    xhashtest_t::hash_calc_count = 0;
    xhashtest_t::print_hash_calc = false;
    uint64_t max_block_height = 1;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    auto tableblock = tableblocks[1];

{
    std::vector<xobject_ptr_t<xvblock_t>> sub_blocks;
    tableblock->extract_sub_blocks(sub_blocks);
    ASSERT_EQ(sub_blocks.size(), 4);

    sub_blocks[0]->set_block_flag(base::enum_xvblock_flag_committed);
    sub_blocks[1]->set_block_flag(base::enum_xvblock_flag_committed);

    base::xvblock_out_offdata_t offdata(sub_blocks);
    std::string offdata_bin;
    offdata.serialize_to_string(offdata_bin);
    if (false == tableblock->set_output_offdata(offdata_bin)) {        
        printf("xvblockstore_impl::load_block_output_offdata,fail-unmatch offdata. block:%s,offdata_size=%zu,offdata_hash=%s:%s",tableblock->dump().c_str(), offdata_bin.size(),
            base::xstring_utl::to_hex(tableblock->get_cert()->hash(offdata_bin)).c_str(), base::xstring_utl::to_hex(tableblock->get_output_offdata_hash()).c_str());
        xassert(false);
    }    
}

{
    ASSERT_TRUE(blockstore->store_block(mocktable, tableblock.get()));

    auto db_block = blockstore->load_block_object(mocktable, tableblock->get_height(), tableblock->get_block_hash(), true);
    ASSERT_EQ(db_block->get_block_hash(), tableblock->get_block_hash());
}

}

TEST_F(test_block_db_size, table_unit_offdata) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    xhashtest_t::hash_calc_count = 0;
    xhashtest_t::print_hash_calc = false;
    uint64_t max_block_height = 5;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    xhashtest_t::print_hash_calc = false;
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    for (uint64_t height = 1; height <= max_block_height; height++) {
        auto db_block = blockstore->load_block_object(mocktable, tableblocks[height]->get_height(), tableblocks[height]->get_block_hash(), true);
        ASSERT_EQ(db_block->get_block_hash(), tableblocks[height]->get_block_hash());
    }

}


TEST_F(test_block_db_size, unit_optimize) {
    {
        base::xstream_t stream(base::xcontext_t::instance());
        std::string emtpy_str;
        stream.write_compact_var(emtpy_str);
        ASSERT_EQ(stream.size(), 1);
    }
    {
        base::xstream_t stream(base::xcontext_t::instance());
        std::string emtpy_str;
        stream << emtpy_str;
        ASSERT_EQ(stream.size(), 4);
    }
}