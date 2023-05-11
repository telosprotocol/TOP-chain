#include <gtest/gtest.h>
#include "xdata/tests/test_blockutl.hpp"
#include "../mock/xmock_network_config.hpp"
#include "../mock/xmock_network.hpp"
#include "xsync/xsync_peerset.h"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;
using namespace top::mock;

// 1shard(5node)
static Json::Value build_validators() {

    Json::Value v = xJson::objectValue;

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

TEST(xsync_peerset, test_shard) {

    std::vector<vnetwork::xvnode_address_t> addr_list;

    {
        Json::Value validators = build_validators();

        xmock_network_config_t cfg_network(validators);
        xmock_network_t network(cfg_network);
        std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();

        for (auto &it: nodes) {
            addr_list.push_back(it->m_addr);
        }
    }

    xsync_peerset_t peerset("");
    bool ret = false;
    uint32_t count = 0;

    vnetwork::xvnode_address_t &self_address = addr_list[0];

    peerset.add_group(self_address);
    ret = peerset.get_group_size(self_address, count);
    ASSERT_TRUE(ret);
    ASSERT_EQ(count, 0);

    // add all peers
    for (uint32_t i=1; i<addr_list.size(); i++) {
        vnetwork::xvnode_address_t &peer_address = addr_list[i];
        peerset.add_peer(self_address, peer_address);

        ret = peerset.get_group_size(self_address, count);
        ASSERT_TRUE(ret);
        ASSERT_EQ(count, i);
    }

    std::string test_address = "123";

    // check
    {
        uint64_t start_height;
        uint64_t end_height;
        vnetwork::xvnode_address_t peer;
        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer);
        ASSERT_TRUE(ret);
        ASSERT_EQ(end_height, 0);
    }

    // update and check 
    for (uint32_t i=1; i<addr_list.size(); i++) {
        vnetwork::xvnode_address_t &peer_address = addr_list[i];

        xchain_state_info_t info;
        info.address = test_address;
        info.start_height = 0;
        info.end_height = i;

        std::vector<xchain_state_info_t> info_list;
        info_list.push_back(info);

        peerset.update(self_address, peer_address, info_list);

        uint64_t start_height;
        uint64_t end_height;
        vnetwork::xvnode_address_t peer;
        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer);
        ASSERT_TRUE(ret);
        ASSERT_EQ(start_height, 0);
        ASSERT_EQ(end_height, i);

        ret = peerset.get_group_size(self_address, count);
        ASSERT_TRUE(ret);
        ASSERT_EQ(count, addr_list.size()-1);
    }

    // remove peer and check
    for (uint32_t i=addr_list.size()-1; i>=1; i--) {

        peerset.remove_peer(self_address, addr_list[i]);

        uint64_t start_height;
        uint64_t end_height;
        vnetwork::xvnode_address_t peer;
        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer);
        ASSERT_TRUE(ret);
        ASSERT_EQ(start_height, 0);
        ASSERT_EQ(end_height, i-1);

        ret = peerset.get_group_size(self_address, count);
        ASSERT_TRUE(ret);
        ASSERT_EQ(count, i-1);
    }

    // remove group and check
    {
        peerset.remove_group(self_address);
        uint64_t start_height;
        uint64_t end_height;
        vnetwork::xvnode_address_t peer;
        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer);
        ASSERT_FALSE(ret);

        ret = peerset.get_group_size(self_address, count);
        ASSERT_FALSE(ret);
    }
}

TEST(xsync_peerset, test_frozen) {

    std::vector<vnetwork::xvnode_address_t> addr_list;

    for (uint32_t i=0; i<200; i++) {
        common::xnetwork_id_t nid{i};
        common::xnode_address_t addr(common::build_frozen_sharding_address(nid));
        addr_list.push_back(addr);
    }

    xsync_peerset_t peerset("");
    uint32_t limit = peerset.get_frozen_limit();
    ASSERT_TRUE((addr_list.size()-1) > limit);

    bool ret = false;
    uint32_t count = 0;

    vnetwork::xvnode_address_t &self_address = addr_list[0];

    peerset.add_group(self_address);
    ret = peerset.get_group_size(self_address, count);
    ASSERT_TRUE(ret);
    ASSERT_EQ(count, 0);

    std::string test_address = "123";

    // update and check 
    for (uint32_t i=1; i<addr_list.size(); i++) {
        vnetwork::xvnode_address_t &peer_address = addr_list[i];

        xchain_state_info_t info;
        info.address = test_address;
        info.start_height = 0;
        info.end_height = i;

        std::vector<xchain_state_info_t> info_list;
        info_list.push_back(info);

        peerset.update(self_address, peer_address, info_list);

        uint64_t start_height;
        uint64_t end_height;
        vnetwork::xvnode_address_t peer;
        ret = peerset.get_newest_peer(self_address, test_address, start_height, end_height, peer);
        ASSERT_TRUE(ret);
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

static Json::Value build_archives() {

    Json::Value v = xJson::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["arc0"]["type"] = "archive";
    v["group"]["arc0"]["parent"] = "zone0";

    v["node"]["node0"]["parent"] = "arc0";
    v["node"]["node1"]["parent"] = "arc0";
    v["node"]["node2"]["parent"] = "arc0";
    v["node"]["node3"]["parent"] = "arc0";
    v["node"]["node4"]["parent"] = "arc0";
    v["node"]["node5"]["parent"] = "arc0";

    return v;
}

TEST(xsync_peerset, test_archive_role) {

    std::vector<vnetwork::xvnode_address_t> addr_list;

    {
        Json::Value validators = build_archives();

        xmock_network_config_t cfg_network(validators);
        xmock_network_t network(cfg_network);
        std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();

        for (auto &it: nodes) {
            addr_list.push_back(it->m_addr);
        }
    }

    xsync_peerset_t peerset("");
    bool ret = false;
    uint32_t count = 0;

    vnetwork::xvnode_address_t &self_address = addr_list[0];

    // check 1
    {
        vnetwork::xvnode_address_t self_addr;
        std::vector<vnetwork::xvnode_address_t> neighbors;
        ret = peerset.get_archive_group(self_addr, neighbors);
        ASSERT_FALSE(ret);
    }

    peerset.add_group(self_address);
    ret = peerset.get_group_size(self_address, count);
    ASSERT_TRUE(ret);
    ASSERT_EQ(count, 0);

    // check 2
    {
        vnetwork::xvnode_address_t self_addr;
        std::vector<vnetwork::xvnode_address_t> neighbors;
        ret = peerset.get_archive_group(self_addr, neighbors);
        ASSERT_TRUE(ret);
        ASSERT_EQ(self_addr, self_address);
        ASSERT_EQ(neighbors.size(), 0);
    }

    // add all peers
    for (uint32_t i=1; i<addr_list.size(); i++) {
        vnetwork::xvnode_address_t &peer_address = addr_list[i];
        peerset.add_peer(self_address, peer_address);

        ret = peerset.get_group_size(self_address, count);
        ASSERT_TRUE(ret);
        ASSERT_EQ(count, i);
    }

    // check 3
    {
        vnetwork::xvnode_address_t self_addr;
        std::vector<vnetwork::xvnode_address_t> neighbors;
        ret = peerset.get_archive_group(self_addr, neighbors);
        ASSERT_TRUE(ret);
        ASSERT_EQ(self_addr, self_address);
        ASSERT_EQ(neighbors.size(), addr_list.size()-1);
    }
}
