#include <gtest/gtest.h>
#include "xsync/xsync_peerset.h"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;

// 1shard(5node)
static xJson::Value build_validators()
{

    xJson::Value v = xJson::objectValue;

    v["group"]["zone0"]["type"] = "zone";

    v["group"]["adv0"]["type"] = "advance";
    v["group"]["adv0"]["parent"] = "zone0";

    v["group"]["shard0"]["type"] = "validator";
    v["group"]["shard0"]["parent"] = "adv0";

    v["node"]["node0"]["parent"] = "shard0";
    v["node"]["node1"]["parent"] = "shard0";
    v["node"]["node2"]["parent"] = "shard0";
    v["node"]["node3"]["parent"] = "shard0";
    v["node"]["node4"]["parent"] = "shard0";

    return v;
}

TEST(xsync_peerset_test, test_random_max)
{

    std::vector<vnetwork::xvnode_address_t> addr_list;

    for (uint32_t i = 0; i < 200; i++) {
        common::xnetwork_id_t nid { i };
        common::xnode_address_t addr(common::build_frozen_sharding_address(nid));
        addr_list.push_back(addr);
    }

    xsync_peerset_t peerset("");
    uint32_t limit = peerset.get_frozen_limit();
    ASSERT_TRUE((addr_list.size() - 1) > limit);

    bool ret = false;
    uint32_t count = 0;

    vnetwork::xvnode_address_t& self_address = addr_list[0];

    peerset.add_group(self_address);
    ret = peerset.get_group_size(self_address, count);
    ASSERT_TRUE(ret);
    ASSERT_EQ(count, 0);

    // update and check
    for (uint32_t i = 1; i < addr_list.size(); i++) {
        vnetwork::xvnode_address_t& peer_address = addr_list[i];

        xchain_state_info_t info;
        std::string test_address = std::to_string(i);
        info.address = test_address;
        info.start_height = 0;
        info.end_height = i;

        std::vector<xchain_state_info_t> info_list;
        info_list.push_back(info);

        peerset.update(self_address, peer_address, info_list);

        uint64_t start_height = 0;
        uint64_t end_height = 0;
        vnetwork::xvnode_address_t peer;
        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer);
        ASSERT_EQ(start_height, 0);
        ASSERT_EQ(end_height, i);

        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer, true);
        ASSERT_EQ(start_height, 0);
        ASSERT_EQ(end_height, i);

        ret = peerset.get_group_size(self_address, count);
        ASSERT_TRUE(ret);

        if (i >= limit)
            ASSERT_EQ(count, limit);
        else
            ASSERT_EQ(count, i);
    }
}


TEST(xsync_peerset_test, test_random_same_height)
{

    std::vector<vnetwork::xvnode_address_t> addr_list;

    for (uint32_t i = 0; i < 200; i++) {
        common::xnetwork_id_t nid { i };
        common::xnode_address_t addr(common::build_frozen_sharding_address(nid));
        addr_list.push_back(addr);
    }

    xsync_peerset_t peerset("");
    uint32_t limit = peerset.get_frozen_limit();
    ASSERT_TRUE((addr_list.size() - 1) > limit);

    bool ret = false;
    uint32_t count = 0;

    vnetwork::xvnode_address_t& self_address = addr_list[0];

    peerset.add_group(self_address);
    ret = peerset.get_group_size(self_address, count);
    ASSERT_TRUE(ret);
    ASSERT_EQ(count, 0);

    // update and check
    for (uint32_t i = 1; i < addr_list.size(); i++) {
        vnetwork::xvnode_address_t& peer_address = addr_list[i];

        xchain_state_info_t info;
        std::string test_address = std::to_string(i);
        info.address = test_address;
        info.start_height = 0;
        info.end_height = 100;

        std::vector<xchain_state_info_t> info_list;
        info_list.push_back(info);

        peerset.update(self_address, peer_address, info_list);

        uint64_t start_height = 0;
        uint64_t end_height = 0;
        vnetwork::xvnode_address_t peer;
        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer);
        ASSERT_EQ(start_height, 0);
        ASSERT_EQ(end_height, 100);

        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer, true);
        ASSERT_EQ(start_height, 0);
        ASSERT_EQ(end_height, 100);

        ret = peerset.get_group_size(self_address, count);
        ASSERT_TRUE(ret);

        if (i >= limit)
            ASSERT_EQ(count, limit);
        else
            ASSERT_EQ(count, i);
    }
}
