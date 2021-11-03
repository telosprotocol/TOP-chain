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

class test_block_recover_meta : public testing::Test {
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(test_block_recover_meta, recover_block_continuous)
{
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = 20;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count);
    const std::vector<xblock_ptr_t>& tables = mocktable.get_history_tables();
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    base::xvaccount_t account(address);

    for (uint64_t i = 1; i <= count; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(blockstore->store_block(account, curr_block));
    }

    //mock case, plugin idle 5s,account 10s
    sleep(20);

    base::xauto_ptr<xvblock_t> latest_commited_block = blockstore->get_latest_committed_block(account);
    std::cout << " test latest_commited_block " << latest_commited_block->get_height() << std::endl;

    base::xauto_ptr<xvblock_t> latest_cert_block = blockstore->get_latest_cert_block(account);
    std::cout << " test latest_cert_block " << latest_cert_block->get_height() << std::endl;

    base::xauto_ptr<xvblock_t> latest_locked_block = blockstore->get_latest_locked_block(account);
    std::cout << " test latest_locked_block " << latest_locked_block->get_height() << std::endl;

    base::xauto_ptr<xvblock_t> latest_connected_block = blockstore->get_latest_connected_block(account);
    std::cout << " test latest_connected_block " << latest_connected_block->get_height() << std::endl;
}
