#include <gtest/gtest.h>
#include <map>
#include <list>
#include <memory>
#include "xdata/xdata_common.h"
#include "xsync/xsync_range_mgr.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::data;
using namespace top::store;
using namespace top::sync;
using namespace top::sync;
#if 0
TEST(xsync_range_mgr, lack) {

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::shared_ptr<mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store = make_object_ptr<store::xstore_face_t>(*(creator.get_xstore()));
    xobject_ptr_t<base::xvblockstore_t> blockstore = make_object_ptr<base::xvblockstore_t>(*(creator.get_blockstore()));

    test_blockstore_datamock_t datamock(store.get(), blockstore);
    

    syncbase::xdata_mgr_ptr_t data_mgr = std::make_shared<syncbase::xdata_mgr_t>("", store.get(), blockstore, address);
    xsync_policy_lack_t policy("", data_mgr);

    uint32_t count;
    std::list<uint64_t> next_heights;
    std::unordered_map<uint64_t,data::xblock_ptr_t> next_blocks;

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 0);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(policy.is_lack_done(), true);
    ASSERT_EQ(count, 100);

    for (uint64_t i=1; i<=200; i++)
        policy.add_lack_height(i);

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 100);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(policy.is_lack_done(), false);
    ASSERT_EQ(count, 0);

    std::map<std::string, std::string> prop_list;
    uint64_t timer_height = 0;
    for (uint64_t i=1; i<=100; i++) {
        xblock_ptr_t block = datamock.create_unit(address, prop_list, timer_height);
        data_mgr->on_data(block);
        policy.on_data(block);
    }

    next_heights.clear();
    next_blocks.clear();

    count = 101;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 100);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(policy.is_lack_done(), false);
    ASSERT_EQ(count, 1);

    for (uint64_t i=1; i<=100; i++) {
        xblock_ptr_t block = datamock.create_unit(address, prop_list, timer_height);
        data_mgr->on_data(block);
        policy.on_data(block);
    }

    next_heights.clear();
    next_blocks.clear();

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 0);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(policy.is_lack_done(), true);
    ASSERT_EQ(count, 100);
}

TEST(xsync_range_mgr, sequence) {

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::shared_ptr<mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
    auto store = store::xstore_factory::create_store_with_memdb(mbus);
    
    base::xvblockstore_t* blockstore = store::xblockstorehub_t::instance().create_block_store(*store, "");
    syncbase::xdata_mgr_ptr_t data_mgr = std::make_shared<syncbase::xdata_mgr_t>("", store.get(), blockstore, address);
    xsync_policy_sequence_t policy("", data_mgr);
    test_blockstore_datamock_t datamock(store.get(), blockstore);

    uint32_t count;
    std::list<uint64_t> next_heights;
    std::unordered_map<uint64_t,data::xblock_ptr_t> next_blocks;

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 0);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(policy.get_sequence_height(), 0);
    ASSERT_EQ(count, 100);

    policy.add_sequence_height(200);
    ASSERT_EQ(policy.get_sequence_height(), 200);

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 100);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(count, 0);

    std::map<std::string, std::string> prop_list;
    uint64_t timer_height = 0;
    for (uint64_t i=1; i<=100; i++) {
        xblock_ptr_t block = datamock.create_unit(address, prop_list, timer_height);
        data_mgr->on_data(block);
        policy.on_data(block);
    }

    next_heights.clear();
    next_blocks.clear();

    count = 101;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 100);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(count, 1);

    for (uint64_t i=1; i<=100; i++) {
        xblock_ptr_t block = datamock.create_unit(address, prop_list, timer_height);
        data_mgr->on_data(block);
        policy.on_data(block);
    }

    next_heights.clear();
    next_blocks.clear();

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 0);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(count, 100);
}

TEST(xsync_range_mgr, history) {

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::shared_ptr<mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
    auto store = store::xstore_factory::create_store_with_memdb(mbus);
    base::xvblockstore_t* blockstore = store::xblockstorehub_t::instance().create_block_store(*store, "");
    test_blockstore_datamock_t datamock(store.get(), blockstore);

    syncbase::xdata_mgr_ptr_t data_mgr = std::make_shared<syncbase::xdata_mgr_t>("", store.get(), blockstore, address);
    xsync_policy_history_t policy("", data_mgr);

    uint32_t count;
    std::list<uint64_t> next_heights;
    std::unordered_map<uint64_t,data::xblock_ptr_t> next_blocks;

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 0);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(count, 100);

    data_mgr->update_max_height(200, 100);

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 100);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(count, 0);

    std::map<std::string, std::string> prop_list;
    uint64_t timer_height = 0;
    for (uint64_t i=1; i<=100; i++) {
        xblock_ptr_t block = datamock.create_unit(address, prop_list, timer_height);
        data_mgr->on_data(block);
        policy.on_data(block);
    }

    next_heights.clear();
    next_blocks.clear();

    count = 101;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 100);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(count, 1);

    for (uint64_t i=1; i<=100; i++) {
        xblock_ptr_t block = datamock.create_unit(address, prop_list, timer_height);
        data_mgr->on_data(block);
        policy.on_data(block);
    }

    next_heights.clear();
    next_blocks.clear();

    count = 100;
    policy.get_next(count, next_heights, next_blocks);
    ASSERT_EQ(next_heights.size(), 0);
    ASSERT_EQ(next_blocks.size(), 0);
    ASSERT_EQ(count, 100);
}


TEST(xsync_range_mgr, on_role_changed) {

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    xsync_latest_active_policy latest_policy_none;
    xsync_history_policy history_policy_none;

    xsync_store_face_mock_t sync_store;
    syncbase::xdata_mgr_ptr_t data_mgr = std::make_shared<syncbase::xdata_mgr_t>("", &sync_store, address);
    syncbase::xchain_info_t chain_info(address, latest_policy_none, latest_policy_none, latest_policy_none, history_policy_none);

    xsync_range_mgr_t range_mgr("", data_mgr, nullptr);
    range_mgr.on_role_changed(chain_info);
    ASSERT_EQ(range_mgr.on_role_changed(chain_info), enum_role_changed_result::enum_role_changed_result_none);

    auto history_policy_full = xsync_history_policy::full_policy();
    syncbase::xchain_info_t chain_info_history(address, latest_policy_none, latest_policy_none, latest_policy_none, history_policy_full);
    ASSERT_EQ(range_mgr.on_role_changed(chain_info_history), enum_role_changed_result::enum_role_changed_result_add_history);

    ASSERT_EQ(range_mgr.on_role_changed(chain_info), enum_role_changed_result::enum_role_changed_result_remove_history);
}
#endif