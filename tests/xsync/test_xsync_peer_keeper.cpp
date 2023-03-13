#include <gtest/gtest.h>
#include "xsync/xsync_sender.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xdata/tests/test_blockutl.hpp"
#include "../mock/xmock_network_config.hpp"
#include "../mock/xmock_network.hpp"
#include "xsync/xsync_peer_keeper.h"
#include "common.h"
#include "xsync/xsync_util.h"
#include "xcommon/xmessage_id.h"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;
using namespace top::mock;

// 1shard(5node)
static Json::Value build_validators() {

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

class xmock_sync_store_peer_keeper_t : public xsync_store_face_mock_t {
public:

    // TODO
    base::xauto_ptr<base::xvblock_t> get_latest_start_block(const std::string & account, enum_chain_sync_policy sync_policy) override {
        auto it = m_blocks.find(account);
        if (it == m_blocks.end())
            return nullptr;

        xblock_ptr_t blk = it->second.front();
        blk->add_ref();

        return blk.get();
    }

    base::xauto_ptr<base::xvblock_t> get_latest_end_block(const std::string & account, enum_chain_sync_policy sync_policy) override {
        auto it = m_blocks.find(account);
        if (it == m_blocks.end())
            return nullptr;

        xblock_ptr_t blk = it->second.back();
        blk->add_ref();

        return blk.get();
    }

    void add_block(const xblock_ptr_t &blk) {
        auto it = m_blocks.find(blk->get_account());
        if (it == m_blocks.end()) {
            std::list<data::xblock_ptr_t> list_blk;
            list_blk.push_back(blk);
            m_blocks[blk->get_account()] = list_blk;
        } else {
            it->second.push_back(blk);
        }
    }

public:
    std::map<std::string, std::list<data::xblock_ptr_t>> m_blocks;
};

TEST(xsync_peer_keeper, test_shard) {
    xmock_sync_store_peer_keeper_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("");
    xrole_xips_manager_t role_xips_mgr("");
    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xsync_peerset_t peerset("");
    xsync_peer_keeper_t peer_keeper("", &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender, &peerset);

    Json::Value validators = build_validators();
    xmock_network_config_t cfg_network(validators);
    xmock_network_t network(cfg_network);

    std::vector<vnetwork::xvnode_address_t> addr_list;
    {
        std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();
        for (auto &it: nodes)
            addr_list.push_back(it->m_addr);
    }

    common::xnode_address_t self_address = addr_list[0];
    bool ret = false;

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_address, table_ids);
    const map_chain_info_t & chains = role_chains->get_chains_wrapper().get_chains();

    // add block
    for (const auto &it: chains) {
        const std::string &address = it.first;
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
        base::xvblock_t* prev_block = genesis_block;
        prev_block->add_ref();
        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        sync_store.add_block(block_ptr);

        for (uint64_t i=1; i<=3; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();

            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr2 = autoptr_to_blockptr(autoptr);
            sync_store.add_block(block_ptr2);
        }
    }

    // add role
    role_chains_mgr.add_role(role_chains);

    std::vector<common::xnode_address_t> neighbor_addresses;
    std::vector<common::xnode_address_t> parent_addresses;
    std::vector<common::xnode_address_t> archive_addresses;
    for (uint32_t i=1; i<addr_list.size(); i++) {
        neighbor_addresses.push_back(addr_list[i]);
    }

    role_xips_mgr.add_role(self_address, neighbor_addresses, parent_addresses, archive_addresses, table_ids);
    peer_keeper.add_role(self_address);

    std::vector<vnetwork::xvnode_address_t> neighbors;
    ret = peerset.get_group_nodes(self_address, neighbors);
    ASSERT_TRUE(ret);
    ASSERT_TRUE(neighbors.size() > 0);
    // not contain self
    for (auto &it: neighbors) {
        if (it == self_address)
            ASSERT_TRUE(false);
    }

    // after add role, check message
    for (auto &it: neighbors) {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_broadcast_chain_state);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_chain_state_info_t>();
        ptr->serialize_from(stream);

        std::vector<xchain_state_info_t> &info_list = ptr->info_list;

        std::set<std::string> chain_set;
        for (const auto &it: chains) {
            chain_set.insert(it.first);
        }

        ASSERT_EQ(info_list.size(), chain_set.size());

        for (auto &it: info_list) {
            ASSERT_EQ(it.end_height, 3);
            chain_set.erase(it.address);
        }
        ASSERT_EQ(chain_set.size(), 0);
    }

    // no message
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    // handle message
    {
        for (auto &it: neighbors) {
            vnetwork::xvnode_address_t &peer_address = it;

            std::vector<xchain_state_info_t> info_list;
            for (const auto &it: chains) {
                xchain_state_info_t info;
                info.address = it.first;
                info.start_height = 0;
                info.end_height = 5;
                info_list.push_back(info);
            }

            peer_keeper.handle_message(self_address, peer_address, info_list);

            for (const auto &it: chains) {
                uint64_t start_height = 0;
                uint64_t end_height = 0;
                vnetwork::xvnode_address_t peer;
                ret = peerset.get_newest_peer(self_address, it.first, start_height, end_height, peer);
                ASSERT_TRUE(ret);
                ASSERT_EQ(start_height, 0);
                ASSERT_EQ(end_height, 5);
            }
        }
    }

    // remove role, check message
    role_chains_mgr.remove_role(role_chains);
    role_xips_mgr.remove_role(self_address);
    peer_keeper.remove_role(self_address);

    // no message
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }
}

TEST(xsync_peer_keeper, test_frozen) {
    xmock_sync_store_peer_keeper_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("");
    xrole_xips_manager_t role_xips_mgr("");
    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xsync_peerset_t peerset("");
    xsync_peer_keeper_t peer_keeper("", &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender, &peerset);

    std::vector<vnetwork::xvnode_address_t> addr_list;
    {
        for (uint32_t i=0; i<200; i++) {
            common::xnetwork_id_t nid{i};
            common::xnode_address_t addr(common::build_frozen_sharding_address(nid));
            addr_list.push_back(addr);
        }
    }

    common::xnode_address_t self_address = addr_list[0];
    bool ret = false;

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_address, table_ids);
    const map_chain_info_t & chains = role_chains->get_chains_wrapper().get_chains();

    // add block
    for (const auto &it: chains) {
        const std::string &address = it.first;
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
        base::xvblock_t* prev_block = genesis_block;
        prev_block->add_ref();
        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        sync_store.add_block(block_ptr);

        for (uint64_t i=1; i<=3; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();

            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr2 = autoptr_to_blockptr(autoptr);
            sync_store.add_block(block_ptr2);
        }
    }

    // add role
    role_chains_mgr.add_role(role_chains);

    std::vector<common::xnode_address_t> neighbor_addresses;
    std::vector<common::xnode_address_t> parent_addresses;
    std::vector<common::xnode_address_t> archive_addresses;
    for (uint32_t i=1; i<addr_list.size(); i++) {
        neighbor_addresses.push_back(addr_list[i]);
    }

    role_xips_mgr.add_role(self_address, neighbor_addresses, parent_addresses, archive_addresses, table_ids);
    peer_keeper.add_role(self_address);

    std::vector<vnetwork::xvnode_address_t> neighbors;
    ret = peerset.get_group_nodes(self_address, neighbors);
    ASSERT_TRUE(ret);
    ASSERT_EQ(neighbors.size(), 0);

    // after add role, check message
    for (uint32_t i=1; i<=peer_keeper.get_frozen_broadcast_factor(); i++) {
        xmessage_t msg;
        xvnode_address_t src;
        common::xip2_t dst;
        ASSERT_EQ(vhost.read_frozen_broadcast_msg(msg, src, dst), true);

        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_frozen_broadcast_chain_state);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_chain_state_info_t>();
        ptr->serialize_from(stream);

        std::vector<xchain_state_info_t> &info_list = ptr->info_list;

        std::set<std::string> chain_set;
        for (const auto &it: chains) {
            chain_set.insert(it.first);
        }

        ASSERT_EQ(info_list.size(), chain_set.size());

        for (auto &it: info_list) {
            ASSERT_EQ(it.start_height, 0);
            ASSERT_EQ(it.end_height, 3);
            chain_set.erase(it.address);
        }
        ASSERT_EQ(chain_set.size(), 0);
    }

    // no message
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    {
        xmessage_t msg;
        xvnode_address_t src;
        common::xip2_t dst;
        ASSERT_EQ(vhost.read_frozen_broadcast_msg(msg, src, dst), false);
    }

    // handle message
    {
        for (uint32_t i=1; i<addr_list.size(); i++) {
            vnetwork::xvnode_address_t &peer_address = addr_list[i];

            std::vector<xchain_state_info_t> info_list;
            for (const auto &it: chains) {
                xchain_state_info_t info;
                info.address = it.first;
                info.start_height = 0;
                info.end_height = 5;

                info_list.push_back(info);
            }

            peer_keeper.handle_message(self_address, peer_address, info_list);

            for (const auto &it: chains) {
                uint64_t start_height;
                uint64_t end_height;
                vnetwork::xvnode_address_t peer;
                ret = peerset.get_newest_peer(self_address, it.first, start_height, end_height, peer);
                ASSERT_TRUE(ret);

                ASSERT_EQ(start_height, 0);
                ASSERT_EQ(end_height, 5);
            }
        }
    }

    // remove role, check message
    role_chains_mgr.remove_role(role_chains);
    role_xips_mgr.remove_role(self_address);
    peer_keeper.remove_role(self_address);

    // no message
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }
    {
        xmessage_t msg;
        xvnode_address_t src;
        common::xip2_t dst;
        ASSERT_EQ(vhost.read_frozen_broadcast_msg(msg, src, dst), false);
    }
}