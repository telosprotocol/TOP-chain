#include <gtest/gtest.h>
#include <map>
#include <list>
#include <memory>
#include "xdata/xdata_common.h"
#include "xsync/xsync_mgr.h"

using namespace top;
using namespace top::data;
using namespace top::sync;

TEST(xsync_mgr, test) {
#if 0
    xsync_mgr_t mgr("", "owner");

    std::unordered_map<uint64_t,syncbase::xsync_height_info_t> heights;
    syncbase::xsync_height_info_t info1;
    info1.block = nullptr;
    info1.bits = 2;
    heights[1] = info1;

    mgr.handle_blocks(heights, syncbase::enum_target_policy_default);

    xentire_block_ptr_t block = nullptr;
    vnetwork::xvnode_address_t self_address;
    vnetwork::xvnode_address_t from_address;
    std::string elect_address = "aaa";
    std::set<uint64_t> elect_heights;
    elect_heights.insert(1);
    elect_heights.insert(2);

    // not exist
    ASSERT_EQ(mgr.set_pending_status(0, block, self_address, from_address), false);

    // exist
    ASSERT_EQ(mgr.set_pending_status(1, block, self_address, from_address), true);

    xentire_block_ptr_t pending_block = make_object_ptr<xentire_block_t>();;
    vnetwork::xvnode_address_t pending_self_address;
    vnetwork::xvnode_address_t pending_from_address;

    // not exist
    ASSERT_EQ(mgr.get_pending_info(0, pending_block, pending_self_address, from_address), false);
    ASSERT_EQ(mgr.get_pending_info(1, pending_block, pending_self_address, from_address), true);

    {
        std::map<uint64_t, xsync_ctx_ptr_t> pending_list = mgr.get_pending_list();
        ASSERT_EQ(pending_list.size(), 1);
    }

    {
        mgr.clear_pending(0);
        std::map<uint64_t, xsync_ctx_ptr_t> pending_list = mgr.get_pending_list();
        ASSERT_EQ(pending_list.size(), 1);
    }

    {
        mgr.clear_pending(1);
        std::map<uint64_t, xsync_ctx_ptr_t> pending_list = mgr.get_pending_list();
        ASSERT_EQ(pending_list.size(), 0);
    }
#endif
}