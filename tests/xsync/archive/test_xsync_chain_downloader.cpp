#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xmbus/xevent_executor.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xmock_auth.hpp"
#define private public
#define protected public
#include "xsync/xchain_downloader.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_store_shadow.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_message.h"
#include "xsync/xsync_ratelimit.h"
#undef private
#undef protected


using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;
using namespace top::mock;



#define TEST_CHAIN_HEIGHT (18)


TEST(test_xsync_chain_downloader, test_chain_downloader_on_behind)
{
   mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    mock::xdatamock_table m_mocktable;
    base::xvblockstore_t* m_blockstore { nullptr };
    m_blockstore = creator.get_blockstore();
    m_mocktable.genrate_table_chain(TEST_CHAIN_HEIGHT, m_blockstore);
    
    const std::vector<xblock_ptr_t>& tables = m_mocktable.get_history_tables();
    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);
    for (uint64_t i = 0; i < tables.size(); i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(m_blockstore->store_block(account, curr_block));
    }
    
    std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
    xsync_store_t sync_store("", make_observer(m_blockstore), store_shadow.get());
    for (uint64_t i = 0; i < tables.size(); i++) {
        auto curr_block = tables[i].get();
        store_shadow->on_chain_event(address, curr_block->get_height());
    }

    xrole_xips_manager_t role_xips_mgr("");
    xrole_chains_mgr_t role_chain_mgr("");

    xsync_sender_t *sync_sender{};
    xsync_ratelimit_t *ratelimit{};

    xmock_auth_t auth {1};

    auto chain_downloader = std::make_shared<xchain_downloader_t>("", &sync_store, &role_xips_mgr, &role_chain_mgr,
        make_observer(&auth), sync_sender, ratelimit, address);

    top::common::xnode_address_t network_self;
    top::common::xnode_address_t target_address;

    std::string reason = "on_timer";
    uint64_t fast_end_height =0; 
    uint64_t cp_end_height =0; 
    uint64_t full_end_height =0;

    uint64_t set_start_height = 0;
    uint64_t set_end_height = 110;
    //mock behind event
    for (int sync_type = (int)enum_chain_sync_policy_fast; sync_type < (int)enum_chain_sync_policy_max; sync_type++) {
        chain_downloader->on_behind(set_start_height, set_end_height, (enum_chain_sync_policy)sync_type, network_self, target_address, reason);

        for (int get_sync_type = (int)enum_chain_sync_policy_fast; get_sync_type < (int)enum_chain_sync_policy_max; get_sync_type++) {
            uint64_t start_height = chain_downloader->m_chain_objects[(enum_chain_sync_policy)get_sync_type].m_start_height;
            uint64_t end_height = chain_downloader->get_chain_last_end_height((enum_chain_sync_policy)get_sync_type);
            if (get_sync_type == sync_type) {
                ASSERT_EQ(set_start_height, start_height);
                ASSERT_EQ(end_height, set_end_height);
            } else {
                if(start_height > 0){
                    ASSERT_NE(set_start_height, start_height);
                }
                ASSERT_NE(end_height, set_end_height);
            }
        }
        set_start_height += 10;
        set_end_height += 10;
    }

    //mock send 
    set_start_height = 0;
    set_end_height = 110;
    for (int sync_type = (int)enum_chain_sync_policy_fast; sync_type < (int)enum_chain_sync_policy_max; sync_type++) {
        std::pair<uint64_t, uint64_t> interval;
        top::common::xnode_address_t self_addr;
        top::common::xnode_address_t target_addr;
        bool ret = chain_downloader->m_chain_objects[sync_type].pick(interval, self_addr, target_addr);
        ASSERT_EQ(set_start_height, interval.first);
        ASSERT_EQ(set_end_height, interval.second);

        if(sync_type != (int)enum_chain_sync_policy_fast) {
            //get real stat height
            int64_t now =0;
            interval.first = chain_downloader->m_chain_objects[sync_type].get_behind_height_real(now, &sync_store, sync_type, address);
            ASSERT_NE(set_start_height, interval.first);
           
           if(sync_type == enum_chain_sync_policy_full) {
              ASSERT_EQ(TEST_CHAIN_HEIGHT+1, interval.first);
           } else {
             ASSERT_EQ(TEST_CHAIN_HEIGHT-2, interval.first);
           }
           
            now = 200000;
            //no new block, height no change
            interval.first = chain_downloader->m_chain_objects[sync_type].get_behind_height_real(now, &sync_store, sync_type, address);
            if(sync_type == enum_chain_sync_policy_full) {
              ASSERT_EQ(TEST_CHAIN_HEIGHT+1, interval.first);
            } else {
              ASSERT_EQ(TEST_CHAIN_HEIGHT-2, interval.first);
            }
        }

        set_start_height += 10;
        set_end_height += 10;
    }
    
}

TEST(test_xsync_chain_downloader, test_chain_downloader_on_behind_with_net_error)
{
   mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    mock::xdatamock_table m_mocktable;
    base::xvblockstore_t* m_blockstore { nullptr };
    m_blockstore = creator.get_blockstore();
    m_mocktable.genrate_table_chain(400, m_blockstore);
    
    const std::vector<xblock_ptr_t>& tables = m_mocktable.get_history_tables();
    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);
    for (uint64_t i = 0; i <= 100; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(m_blockstore->store_block(account, curr_block));
    }
    
    xrole_xips_manager_t role_xips_mgr("");
    xrole_chains_mgr_t role_chain_mgr("");

    xsync_sender_t *sync_sender{};
    xsync_ratelimit_t *ratelimit{};

    xmock_auth_t auth {1};

    std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
    xsync_store_t sync_store("", make_observer(m_blockstore), store_shadow.get());
    for (uint64_t i = 0; i < 300; i++) {
        auto curr_block = tables[i].get();
        store_shadow->on_chain_event(address, curr_block->get_height());
    }
    sync_store.remove_listener(mbus::xevent_major_type_store, store_shadow->m_listener_id);

    auto chain_downloader = std::make_shared<xchain_downloader_t>("", &sync_store, &role_xips_mgr, &role_chain_mgr,
        make_observer(&auth), sync_sender, ratelimit, address);

    top::common::xnode_address_t network_self;
    top::common::xnode_address_t target_address;

    std::string reason = "on_timer";

    uint64_t set_start_height = 100;
    uint64_t set_end_height = 200;
   
    //mock behind event once
    chain_downloader->on_behind(set_start_height, set_end_height, enum_chain_sync_policy_checkpoint, network_self, target_address, reason);        
    uint64_t start_height = chain_downloader->m_chain_objects[enum_chain_sync_policy_checkpoint].m_start_height;
    uint64_t end_height = chain_downloader->get_chain_last_end_height(enum_chain_sync_policy_checkpoint);
    ASSERT_EQ(set_start_height, start_height);
    ASSERT_EQ(end_height, set_end_height);
 
    //mock send ,but net error, so not response
    std::pair<uint64_t, uint64_t> interval;
    top::common::xnode_address_t self_addr;
    top::common::xnode_address_t target_addr;
    bool ret = chain_downloader->m_chain_objects[enum_chain_sync_policy_checkpoint].pick(interval, self_addr, target_addr);
    ASSERT_EQ(set_start_height, interval.first);
    ASSERT_EQ(131, interval.second);

    //new block push or store
    for (uint64_t i = 100; i <= 200; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(m_blockstore->store_block(account, curr_block));
    }

    //error case: heigt is same before
    ret = chain_downloader->m_chain_objects[enum_chain_sync_policy_checkpoint].pick(interval, self_addr, target_addr);
    ASSERT_EQ(set_start_height, interval.first);
    ASSERT_EQ(131, interval.second);
    
    //fix now
    int64_t now = 0;
    interval.first = chain_downloader->m_chain_objects[enum_chain_sync_policy_checkpoint].get_behind_height_real(now,  &sync_store, (int)enum_chain_sync_policy_checkpoint, address);
    ASSERT_EQ(198, interval.first);

}

TEST(test_xsync_chain_downloader, test_chain_downloader_fast_to_cp)
{
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    mock::xdatamock_table m_mocktable;
    base::xvblockstore_t* m_blockstore { nullptr };
    m_blockstore = creator.get_blockstore();
    m_mocktable.genrate_table_chain(400, m_blockstore);
    
    const std::vector<xblock_ptr_t>& tables = m_mocktable.get_history_tables();
    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);
    for (uint64_t i = 0; i <= 100; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(m_blockstore->store_block(account, curr_block));
    }
    
    xrole_xips_manager_t role_xips_mgr("");
    xrole_chains_mgr_t role_chain_mgr("");

    xsync_sender_t *sync_sender{};
    xsync_ratelimit_t *ratelimit{};

    xmock_auth_t auth {1};

    std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
    xsync_store_t sync_store("", make_observer(m_blockstore), store_shadow.get());
    for (uint64_t i = 0; i < 100; i++) {
        auto curr_block = tables[i].get();
        store_shadow->on_chain_event(address, curr_block->get_height());
    }
    sync_store.remove_listener(mbus::xevent_major_type_store, store_shadow->m_listener_id);

    auto chain_downloader = std::make_shared<xchain_downloader_t>("", &sync_store, &role_xips_mgr, &role_chain_mgr,
        make_observer(&auth), sync_sender, ratelimit, address);

    top::common::xnode_address_t network_self;
    top::common::xnode_address_t target_address;

    std::string reason = "on_timer";

    uint64_t set_start_height = 100;
    uint64_t set_end_height = 200;
   
    //mock behind fast 
    chain_downloader->on_behind(set_start_height, set_end_height, enum_chain_sync_policy_fast, network_self, target_address, reason);        
    uint64_t start_height = chain_downloader->m_chain_objects[enum_chain_sync_policy_fast].m_start_height;
    uint64_t end_height = chain_downloader->get_chain_last_end_height(enum_chain_sync_policy_fast);
    ASSERT_EQ(set_start_height, start_height);
    ASSERT_EQ(end_height, set_end_height);
    //fast is ok
     for (uint64_t i = 100; i <= 200; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(m_blockstore->store_block(account, curr_block));
        store_shadow->on_chain_event(address, curr_block->get_height());
    }

    //now cp event
    chain_downloader->on_behind(0, 200, enum_chain_sync_policy_checkpoint, network_self, target_address, reason);    

    std::pair<uint64_t, uint64_t> interval;
    top::common::xnode_address_t self_addr;
    top::common::xnode_address_t target_addr;
    bool ret = chain_downloader->m_chain_objects[enum_chain_sync_policy_checkpoint].pick(interval, self_addr, target_addr);
    ASSERT_EQ(0, interval.first);
    //cureent hieght =1 ,and max count = 130
    ASSERT_EQ(131, interval.second);
    interval.first = chain_downloader->m_chain_objects[enum_chain_sync_policy_checkpoint].height();
    ASSERT_EQ(1, interval.first);

    //fix from db init height
    int64_t now = 0;
    interval.first = chain_downloader->m_chain_objects[enum_chain_sync_policy_checkpoint].get_behind_height_real(now,  &sync_store, (int)enum_chain_sync_policy_checkpoint, address);
    //commit height is 200 -2;
    ASSERT_EQ(198, interval.first);

}
