#include <gtest/gtest.h>
#include <map>
#include <list>
#include <memory>
#include "xsync/xsync_latest.h"
#include "../mock/xmock_auth.hpp"
#include "../mock/xmock_addr_generator.hpp"
#include "../mock/xmock_network_config.hpp"
#include "../mock/xmock_network.hpp"
#include "xdata/xblocktool.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xsync/xsync_util.h"
#include "common.h"

using namespace top;
using namespace top::data;
using namespace top::store;
using namespace top::sync;
using namespace top::mock;
using namespace top::vnetwork;

class xmock_sync_store_t : public xsync_store_face_mock_t {
public:
    base::xauto_ptr<base::xvblock_t> get_latest_cert_block(const std::string & account) override {
        auto it = map_cert_block.find(account);
        if (it == map_cert_block.end())
            return nullptr;

        it->second->add_ref();
        base::xauto_ptr<base::xvblock_t> blk(it->second.get());

        return blk;
    }

    bool store_block(base::xvblock_t* block) override {

        auto it = map_cert_block.find(block->get_account());
        if (it == map_cert_block.end()) {
            block->add_ref();
            base::xauto_ptr<base::xvblock_t> autoptr = block;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
            map_cert_block[block->get_account()] = block_ptr;
            return true;
        }

        if (block->get_viewid() < it->second->get_viewid())
            return true;

        if (block->get_viewid() == it->second->get_viewid())
            return true;

        block->add_ref();
        base::xauto_ptr<base::xvblock_t> autoptr = block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        map_cert_block[block->get_account()] = block_ptr;
        return true;
    }

public:
    std::map<std::string,xblock_ptr_t> map_cert_block;
};

TEST(xsync_latest, add_role_validator) {

    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;
    xmock_sync_store_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("", &sync_store);
    xdeceit_node_manager_t blacklist;
    xrole_xips_manager_t role_xips_mgr("", nullptr, &blacklist);
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xsync_latest_t sync_latest("", make_observer(&auth), &sync_store, &role_chains_mgr, &sync_sender);

    std::vector<std::string> addr_list;
    {
        std::string beacon_address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);
        addr_list.push_back(beacon_address);

        for (uint16_t i=0; i<4; i++) {
            std::string zec_address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, i);
            addr_list.push_back(zec_address);
        }

        for (uint16_t i=0; i<256; i++) {
            std::string shard_address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);
            addr_list.push_back(shard_address);
        }
    }

    for (auto &it: addr_list) {
        const std::string &address = it;

        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

        base::xvblock_t* prev_block = genesis_block;
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);

        sync_store.map_cert_block[address] = block_ptr;
    }

    xmock_network_config_t cfg_network("xsync_latest");
    xmock_network_t network(cfg_network);
    std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();

    vnetwork::xvnode_address_t self_addr;
    std::vector<vnetwork::xvnode_address_t> neighbors;
    std::vector<vnetwork::xvnode_address_t> parents;
    std::vector<vnetwork::xvnode_address_t> archives;

    for (auto &it: nodes) {
        vnetwork::xvnode_address_t &addr = it->m_addr;

        common::xnode_type_t type = real_part_type(addr.type());

        if (type == common::xnode_type_t::consensus_validator) {
            if (self_addr.empty()) {
                self_addr = addr;
            }
            neighbors.push_back(addr);
        }

        if (type == common::xnode_type_t::archive)
            archives.push_back(addr);
    }

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives);

    std::vector<std::uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.push_back(i);
    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);
    sync_latest.add_role(self_addr);

    uint32_t validator_size = 0;
    uint32_t archive_size = 0;

    for (int i=0; i<4; i++) {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        xsync_message_latest_block_info_ptr_t ptr = make_object_ptr<xsync_message_latest_block_info_t>();
        ptr->serialize_from(stream);

        std::vector<xlatest_block_info_t> &info_list = ptr->info_list;
        ASSERT_EQ(info_list.size(), 256);

        for (auto &it: info_list) {
            xlatest_block_info_t &info = it;
            base::xauto_ptr<base::xvblock_t> latest_block = sync_store.get_latest_cert_block(info.address);
            ASSERT_NE(latest_block, nullptr);
            ASSERT_EQ(latest_block->get_account(), info.address);
            ASSERT_EQ(latest_block->get_height(), info.height);
            ASSERT_EQ(latest_block->get_viewid(), info.view_id);
            ASSERT_EQ(latest_block->get_block_hash(), info.hash);
        }

        if (real_part_type(dst.type()) == common::xnode_type_t::consensus_validator)
            validator_size++;

        if (real_part_type(dst.type()) == common::xnode_type_t::archive)
            archive_size++;
    }

    xmessage_t msg;
    xvnode_address_t src;
    xvnode_address_t dst;
    ASSERT_EQ(vhost.read_msg(msg, src, dst), false);

    ASSERT_EQ(validator_size, 2);
    ASSERT_EQ(archive_size, 2);
}

class xmock_sync_latest_t : public xsync_latest_t {
public:
    xmock_sync_latest_t(const std::string &vnode_id, const observer_ptr<base::xvcertauth_t> &certauth, xsync_store_face_t *sync_store, xrole_chains_mgr_t *role_chains_mgr, xsync_sender_t *sync_sender):
    xsync_latest_t(vnode_id, certauth, sync_store, role_chains_mgr, sync_sender) {
    }

    void reset_timer() {
        m_last_send_time = 0;
    }
};

TEST(xsync_latest, on_timer) {

    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;
    xmock_sync_store_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("", &sync_store);
    xdeceit_node_manager_t blacklist;
    xrole_xips_manager_t role_xips_mgr("", nullptr, &blacklist);
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xmock_sync_latest_t sync_latest("", make_observer(&auth), &sync_store, &role_chains_mgr, &sync_sender);

    std::vector<std::string> addr_list;
    {
        std::string beacon_address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);
        addr_list.push_back(beacon_address);

        for (uint16_t i=0; i<4; i++) {
            std::string zec_address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, i);
            addr_list.push_back(zec_address);
        }

        for (uint16_t i=0; i<256; i++) {
            std::string shard_address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);
            addr_list.push_back(shard_address);
        }
    }

    for (auto &it: addr_list) {
        const std::string &address = it;

        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

        base::xvblock_t* prev_block = genesis_block;
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);

        sync_store.map_cert_block[address] = block_ptr;
    }

    xmock_network_config_t cfg_network("xsync_latest");
    xmock_network_t network(cfg_network);
    std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();

    vnetwork::xvnode_address_t self_addr;
    std::vector<vnetwork::xvnode_address_t> neighbors;
    std::vector<vnetwork::xvnode_address_t> parents;
    std::vector<vnetwork::xvnode_address_t> archives;

    for (auto &it: nodes) {
        vnetwork::xvnode_address_t &addr = it->m_addr;

        common::xnode_type_t type = real_part_type(addr.type());

        if (type == common::xnode_type_t::consensus_validator) {
            if (self_addr.empty()) {
                self_addr = addr;
            }
            neighbors.push_back(addr);
        }

        if (type == common::xnode_type_t::archive)
            archives.push_back(addr);
    }

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives);

    std::vector<std::uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.push_back(i);
    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);

    sync_latest.reset_timer();
    sync_latest.on_timer();

    uint32_t validator_size = 0;

    for (int i=0; i<1; i++) {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        xsync_message_latest_block_info_ptr_t ptr = make_object_ptr<xsync_message_latest_block_info_t>();
        ptr->serialize_from(stream);

        std::vector<xlatest_block_info_t> &info_list = ptr->info_list;
        ASSERT_EQ(info_list.size(), 256);

        for (auto &it: info_list) {
            xlatest_block_info_t &info = it;
            base::xauto_ptr<base::xvblock_t> latest_block = sync_store.get_latest_cert_block(info.address);
            ASSERT_NE(latest_block, nullptr);
            ASSERT_EQ(latest_block->get_account(), info.address);
            ASSERT_EQ(latest_block->get_height(), info.height);
            ASSERT_EQ(latest_block->get_viewid(), info.view_id);
            ASSERT_EQ(latest_block->get_block_hash(), info.hash);
        }

        if (real_part_type(dst.type()) == common::xnode_type_t::consensus_validator)
            validator_size++;
    }

    ASSERT_EQ(validator_size, 1);

    sync_latest.on_timer();

    xmessage_t msg;
    xvnode_address_t src;
    xvnode_address_t dst;
    ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
}

TEST(xsync_latest, handle_latest_block_info) {

    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;
    xmock_sync_store_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("", &sync_store);
    xdeceit_node_manager_t blacklist;
    xrole_xips_manager_t role_xips_mgr("", nullptr, &blacklist);
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xsync_latest_t sync_latest("", make_observer(&auth), &sync_store, &role_chains_mgr, &sync_sender);

    std::vector<std::string> addr_list;
    {
        std::string beacon_address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);
        addr_list.push_back(beacon_address);

        for (uint16_t i=0; i<4; i++) {
            std::string zec_address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, i);
            addr_list.push_back(zec_address);
        }

        for (uint16_t i=0; i<256; i++) {
            std::string shard_address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);
            addr_list.push_back(shard_address);
        }
    }

    for (auto &it: addr_list) {
        const std::string &address = it;

        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

        base::xvblock_t* prev_block = genesis_block;
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);

        sync_store.map_cert_block[address] = block_ptr;
    }

    xmock_network_config_t cfg_network("xsync_latest");
    xmock_network_t network(cfg_network);
    std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();

    vnetwork::xvnode_address_t self_addr;
    std::vector<vnetwork::xvnode_address_t> neighbors;
    std::vector<vnetwork::xvnode_address_t> parents;
    std::vector<vnetwork::xvnode_address_t> archives;

    for (auto &it: nodes) {
        vnetwork::xvnode_address_t &addr = it->m_addr;

        common::xnode_type_t type = real_part_type(addr.type());

        if (type == common::xnode_type_t::consensus_validator) {
            if (self_addr.empty()) {
                self_addr = addr;
            }
            neighbors.push_back(addr);
        }

        if (type == common::xnode_type_t::archive)
            archives.push_back(addr);
    }

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives);

    std::vector<std::uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.push_back(i);
    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);

    std::vector<xlatest_block_info_t> info_list;
    std::string shard_address0 = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);
    std::string shard_address1 = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 1);
    std::string shard_address2 = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 2);
    {
        xlatest_block_info_t info;
        info.address = shard_address0;
        info.height = 0;
        info.view_id = 0;
        info.hash = "abc";
        info_list.push_back(info);
    }
    {
        xlatest_block_info_t info;
        info.address = shard_address1;
        info.height = 1;
        info.view_id = 1;
        info.hash = "def";
        info_list.push_back(info);
    }
    {
        xlatest_block_info_t info;
        info.address = shard_address2;
        info.height = 2;
        info.view_id = 2;
        info.hash = "xyz";
        info_list.push_back(info);
    }

    vnetwork::xvnode_address_t from_address = archives[0];
    vnetwork::xvnode_address_t network_self = self_addr;

    sync_latest.handle_latest_block_info(info_list, from_address, network_self);

    // peer is higher
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        xsync_message_get_latest_blocks_ptr_t ptr = make_object_ptr<xsync_message_get_latest_blocks_t>();
        ptr->serialize_from(stream);

        std::string &address = ptr->address;
        std::vector<xlatest_block_item_t> &list = ptr->list;

        ASSERT_EQ(address, shard_address2);
        ASSERT_EQ(list.size(), 1);
        ASSERT_EQ(list[0].height, 2);
        ASSERT_EQ(list[0].hash, "xyz");
    }

    // local is higher
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        xsync_message_latest_block_info_ptr_t ptr = make_object_ptr<xsync_message_latest_block_info_t>();
        ptr->serialize_from(stream);

        std::vector<xlatest_block_info_t> &info_list = ptr->info_list;
        ASSERT_EQ(info_list.size(), 1);

        xlatest_block_info_t &info = info_list[0];
        base::xauto_ptr<base::xvblock_t> latest_block = sync_store.get_latest_cert_block(shard_address0);
        ASSERT_NE(latest_block, nullptr);
        ASSERT_EQ(latest_block->get_account(), info.address);
        ASSERT_EQ(latest_block->get_height(), info.height);
        ASSERT_EQ(latest_block->get_viewid(), info.view_id);
        ASSERT_EQ(latest_block->get_block_hash(), info.hash);
    }

    xmessage_t msg;
    xvnode_address_t src;
    xvnode_address_t dst;
    ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
}

TEST(xsync_latest, handle_latest_block) {

    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;
    xmock_sync_store_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("", &sync_store);
    xdeceit_node_manager_t blacklist;
    xrole_xips_manager_t role_xips_mgr("", nullptr, &blacklist);
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xsync_latest_t sync_latest("", make_observer(&auth), &sync_store, &role_chains_mgr, &sync_sender);

    std::vector<std::string> addr_list;
    {
        for (uint16_t i=0; i<256; i++) {
            std::string shard_address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);
            addr_list.push_back(shard_address);
        }
    }

    std::map<std::string,std::vector<xblock_ptr_t>> all_blocks;

    for (auto &it: addr_list) {

        std::vector<xblock_ptr_t> vector_blocks;

        const std::string &address = it;
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

        base::xvblock_t* prev_block = genesis_block;
        for (int i=0; i<5; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();
            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);

            vector_blocks.push_back(block_ptr);
        }

        // height=4
        sync_store.map_cert_block[address] = vector_blocks[3];
        all_blocks[address] = vector_blocks;
    }

    xmock_network_config_t cfg_network("xsync_latest");
    xmock_network_t network(cfg_network);
    std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();

    vnetwork::xvnode_address_t self_addr;
    std::vector<vnetwork::xvnode_address_t> neighbors;
    std::vector<vnetwork::xvnode_address_t> parents;
    std::vector<vnetwork::xvnode_address_t> archives;

    for (auto &it: nodes) {
        vnetwork::xvnode_address_t &addr = it->m_addr;

        common::xnode_type_t type = real_part_type(addr.type());

        if (type == common::xnode_type_t::consensus_validator) {
            if (self_addr.empty()) {
                self_addr = addr;
            }
            neighbors.push_back(addr);
        }

        if (type == common::xnode_type_t::archive)
            archives.push_back(addr);
    }

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives);

    std::vector<std::uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.push_back(i);
    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);


    std::string address = addr_list[0];
    std::vector<xblock_ptr_t> vector_blocks = all_blocks[address];

    std::vector<xblock_ptr_t> blocks;
    blocks.push_back(vector_blocks[2]);
    sync_latest.handle_latest_blocks(blocks, self_addr);
    base::xauto_ptr<base::xvblock_t> blk1 = sync_store.get_latest_cert_block(address);
    ASSERT_NE(blk1, nullptr);
    ASSERT_EQ(blk1->get_height(), 4);

    blocks.clear();
    blocks.push_back(vector_blocks[3]);
    sync_latest.handle_latest_blocks(blocks, self_addr);
    base::xauto_ptr<base::xvblock_t> blk2 = sync_store.get_latest_cert_block(address);
    ASSERT_NE(blk2, nullptr);
    ASSERT_EQ(blk2->get_height(), 4);

    blocks.clear();
    blocks.push_back(vector_blocks[4]);
    sync_latest.handle_latest_blocks(blocks, self_addr);
    base::xauto_ptr<base::xvblock_t> blk3 = sync_store.get_latest_cert_block(address);
    ASSERT_NE(blk3, nullptr);
    ASSERT_EQ(blk3->get_height(), 5);
}