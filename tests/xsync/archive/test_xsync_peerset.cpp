#include <gtest/gtest.h>
#include "xsync/xsync_peerset.h"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;

// 1shard(5node)
static Json::Value build_validators()
{

    Json::Value v = Json::objectValue;

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
#define CREATE_ADDR_COUNT  (200)

std::vector<vnetwork::xvnode_address_t> create_addr_list(const uint32_t count = CREATE_ADDR_COUNT) {
    std::vector<vnetwork::xvnode_address_t> addr_list;

    for (uint32_t i = 0; i < count; i++) {
        common::xnetwork_id_t nid { i };
        common::xnode_address_t addr(common::build_frozen_sharding_address(nid));
        addr_list.push_back(addr);
    }
    return addr_list;
}


TEST(xsync_peerset_test, test_random_max)
{
    std::vector<vnetwork::xvnode_address_t> addr_list = create_addr_list();

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
    std::vector<vnetwork::xvnode_address_t> addr_list = create_addr_list();

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

std::vector<vnetwork::xvnode_address_t> create_addr_consensus_list(const uint32_t count = CREATE_ADDR_COUNT) {
    std::vector<vnetwork::xvnode_address_t> addr_list;

    for (uint32_t i = 0; i < count; i++) {
        common::xnetwork_id_t nid{i};
        common::xgroup_id_t gid{1};
        common::xnode_address_t addr(common::build_consensus_sharding_address(gid, nid));
        addr_list.push_back(addr);
    }

    return addr_list;
}

TEST(xsync_peerset_test, test_behind_info_different_height_map) {
    std::vector<vnetwork::xvnode_address_t> addr_list = create_addr_consensus_list();

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
    std::string table_address = "Ta0001@0";
    // update and check
    for (uint32_t i = 0; i < limit; i++) {
        vnetwork::xvnode_address_t& peer_address = addr_list[i+1];

        xchain_state_info_t info;
        info.address = table_address;
        info.start_height = i;
        info.end_height = i + 400;

        std::vector<xchain_state_info_t> info_list;
        info_list.push_back(info);
        peerset.add_group(peer_address);
        peerset.add_peer(self_address,peer_address);
        peerset.update(self_address, peer_address, info_list);
        ret = peerset.get_group_size(self_address, count);
    }

    for (uint32_t i = 0; i < limit; i++) {
        uint64_t latest_start_block_height = 0;
        uint64_t latest_end_block_height = i + 399;
        std::multimap<uint64_t, mbus::chain_behind_event_address> chain_behind_address_map{};
        auto ret = peerset.get_peer_height_info_map(self_address, table_address, latest_start_block_height,
                                                   latest_end_block_height, chain_behind_address_map);
        ASSERT_EQ(ret, true);
        ASSERT_EQ(chain_behind_address_map.size(), (limit-i)); 
    }
}

