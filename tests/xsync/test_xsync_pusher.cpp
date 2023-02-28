#include <gtest/gtest.h>
#include "xsync/xsync_pusher.h"
#include "common.h"
#include "../mock/xmock_network_config.hpp"
#include "../mock/xmock_network.hpp"
#include "xdata/xdatautil.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xsync/xsync_util.h"

using namespace top;
using namespace top::sync;
using namespace top::mock;
using namespace top::data;

TEST(xsync_pusher, test_algorithm) {
    uint32_t src_count = 3;
    uint32_t dst_count = 10;

    for (uint32_t random = 0; random < 10; random++) {
        printf("####random=%u\n", random);
        for (uint32_t i=0; i<src_count; i++) {
            std::vector<uint32_t> results = calc_push_mapping(src_count, dst_count, i, random);
            printf("%u=", i);
            for (auto &it: results)
                printf("%u ", it);
            printf("\n");
        }

        std::set<uint32_t> results = calc_push_select(dst_count, random);
        printf("block= ");
        for (auto &it: results)
            printf("%u ", it);
        printf("\n");
    }
}

static Json::Value build_validators() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";

    v["group"]["adv0"]["type"] = "advance";
    v["group"]["adv0"]["parent"] = "zone0";

    v["group"]["shard0"]["type"] = "validator";
    v["group"]["shard0"]["parent"] = "adv0";

    v["node"]["node0"]["parent"] = "shard0";
    v["node"]["node1"]["parent"] = "shard0";

    return v;
}

static Json::Value build_archives() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["arc0"]["type"] = "archive";
    v["group"]["arc0"]["parent"] = "zone0";

    v["node"]["node2"]["parent"] = "arc0";
    v["node"]["node3"]["parent"] = "arc0";

    return v;
}

TEST(xsync_pusher, test_push_block) {

    xrole_xips_manager_t role_xips_mgr("");
    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);

    xsync_pusher_t sync_pusher("", &role_xips_mgr, &sync_sender);

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<64; i++)
        table_ids.insert(i);

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

    // archive
    std::vector<common::xnode_address_t> archive_addresses;
    {
        Json::Value archives = build_archives();
        xmock_network_config_t cfg_network(archives);
        xmock_network_t network(cfg_network);
        std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();
        for (auto &it: nodes)
            archive_addresses.push_back(it->m_addr);
    }

    vnetwork::xvnode_address_t &self_address = addr_list[0];

    std::vector<common::xnode_address_t> neighbor_addresses;
    std::vector<common::xnode_address_t> parent_addresses;
    for (uint32_t i=1; i<addr_list.size(); i++) {
        neighbor_addresses.push_back(addr_list[i]);
    }

    role_xips_mgr.add_role(self_address, neighbor_addresses, parent_addresses, archive_addresses, table_ids);

    std::string table_address0 = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);
    std::string table_address65 = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 65);

    std::vector<data::xblock_ptr_t> vector_blocks_table0;
    std::vector<data::xblock_ptr_t> vector_blocks_table65;

    {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(table_address0);
        base::xvblock_t* prev_block = genesis_block;
        for (uint64_t i=0; i<10; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();
            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
            vector_blocks_table0.push_back(block_ptr);
        }
    }

    {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(table_address65);
        base::xvblock_t* prev_block = genesis_block;
        for (uint64_t i=0; i<10; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();
            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
            vector_blocks_table65.push_back(block_ptr);
        }
    }

    sync_pusher.push_newblock_to_archive(vector_blocks_table0[0]);

    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);
    }

    sync_pusher.push_newblock_to_archive(vector_blocks_table65[0]);

     {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }
}
