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

class test_block_performance : public testing::Test {
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};
TEST_F(test_block_performance, table_store_load_BENCH_1) {
// store_blocks 1000 time_ms=4784  ->  3658
// load_blocks 1000 time_ms=326
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 1000;
    mock::xdatamock_table mocktable(1, 60);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();

    base::xvchain_t::instance().set_node_type(true, true);

    auto t1 = base::xtime_utl::time_now_ms();
    for (auto & block : tableblocks) {
        blockstore->store_block(mocktable, block.get());
    }
    auto t2 = base::xtime_utl::time_now_ms();

    for (uint64_t h = 1; h <= max_block_height; h++ ) {
        xobject_ptr_t<base::xvblock_t> block = blockstore->load_block_object(mocktable, h, base::enum_xvblock_flag_authenticated, true);
    }
    auto t3 = base::xtime_utl::time_now_ms();

    std::cout << "store_blocks " << max_block_height << " time_ms=" << (t2-t1) << std::endl;
    std::cout << "load_blocks " << max_block_height << " time_ms=" << (t3-t2) << std::endl;
}
#if 0
TEST_F(test_block_performance, table_block_sync_BENCH_1) {
// full_block_serialize_and_read_form 10000 time_ms=4572
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 2;
    mock::xdatamock_table mocktable(1, 60);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();

    base::xstream_t _stream(base::xcontext_t::instance());
    tableblocks[1]->full_block_serialize_to(_stream);
    size_t block_size = _stream.size();
    base::xauto_ptr<base::xvblock_t> _block = base::xvblock_t::full_block_read_from(_stream);

    auto t1 = base::xtime_utl::time_now_ms();
    uint32_t count = 10000;
    for (uint32_t i = 0; i < count; i++) {
        base::xautostream_t<1024> _stream2(base::xcontext_t::instance());
        _block->full_block_serialize_to(_stream2);
        base::xauto_ptr<base::xvblock_t> _block2 = base::xvblock_t::full_block_read_from(_stream2);
    }

    auto t2 = base::xtime_utl::time_now_ms();

    std::cout << "full_block_serialize_and_read_form count=" << count << " block_size=" << block_size << " time_ms=" << (t2-t1) << std::endl;
}
#endif