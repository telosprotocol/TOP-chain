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

class test_block_recovery : public testing::Test {
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(test_block_recovery, recover_block_continuous)
{
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = 200;
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

    std::vector<std::string>  unit_account_names = mocktable.get_unit_accounts();
    std::string unit_account_name = unit_account_names[0];
    base::xvaccount_t unit_account(unit_account_name);


    base::xvtable_t * target_table = base::xvchain_t::instance().get_table(unit_account.get_xvid()); 
   
    ASSERT_TRUE(target_table != nullptr);

    {
    base::xauto_ptr<base::xvaccountobj_t> account_obj = target_table->get_account(unit_account);
        
        ASSERT_TRUE(account_obj != nullptr);

        account_obj->set_idle_duration(10000);

        xauto_ptr<xvactplugin_t> block_plugin =  account_obj->get_plugin(base::enum_xvaccount_plugin_blockmgr );

        block_plugin->set_idle_duration(5000);


    }
    
       xinfo("start check address=%s "  ,unit_account_name.c_str());

    //mock case, plugin idle 5s,account 10s,wait 25s
    sleep(40);

    //change process id, so plugin will be recover meta 
    uint32_t old_process_id  =   base::xvchain_t::instance().get_current_process_id(); 
    std::cout << " test old_process_id " << old_process_id << std::endl;
    base::xvchain_t::instance().set_current_process_id(1);
   // sleep(1);
    xinfo("get_current_process_id=%ld "  , base::xvchain_t::instance().get_current_process_id()); 
  
    
    std::cout <<"start show address= " <<unit_account_name.c_str()<< std::endl;

    base::xauto_ptr<xvblock_t> latest_cert_block = blockstore->get_latest_cert_block(unit_account);
    std::cout << " test latest_cert_block " << latest_cert_block->get_height() << std::endl;
  //  EXPECT_EQ(latest_cert_block->get_height(), count);
    
    base::xauto_ptr<xvblock_t> latest_locked_block = blockstore->get_latest_locked_block(unit_account);
    std::cout << " test latest_locked_block " << latest_locked_block->get_height() << std::endl;
  //  EXPECT_EQ(latest_locked_block->get_height(), count - 1);

    base::xauto_ptr<xvblock_t> latest_commited_block = blockstore->get_latest_committed_block(unit_account);
    std::cout << " test latest_commited_block " << latest_commited_block->get_height() << std::endl;
  //  EXPECT_EQ(latest_commited_block->get_height(), count - 2);

    base::xauto_ptr<xvblock_t> latest_connected_block = blockstore->get_latest_connected_block(unit_account);
    std::cout << " test latest_connected_block " << latest_connected_block->get_height() << std::endl;
 //   std::cout << " test latest_connected_block " << latest_connected_block->get_height() << std::endl;
 //  EXPECT_EQ(latest_commited_block->get_height(), count - 2);

}
