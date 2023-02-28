#include <gtest/gtest.h>
#include <map>
#include <list>
#include <memory>
#include "xsync/xsync_on_demand.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "../mock/xmock_auth.hpp"
#include "../mock/xmock_network_config.hpp"
#include "../mock/xmock_network.hpp"
#include "xdata/xblocktool.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xsync/xsync_util.h"
#include "xmbus/xmessage_bus.h"
#include "common.h"
#include "xmbus/xevent_behind.h"
#include "xmbus/xevent_sync.h"
#include "../xblockstore_test/test_blockmock.hpp"

using namespace top;
using namespace top::data;
using namespace top::store;
using namespace top::sync;
using namespace top::mock;
using namespace top::vnetwork;

class xmock_sync_store_unit_t : public xsync_store_face_mock_t {
public:
    bool store_block(base::xvblock_t* block) override {
        m_blocks.push_back(block);
        return true;
    }

public:
    std::vector<base::xvblock_t*> m_blocks;
};

// 1shard(1node) + 1 archive(1node)
static Json::Value build_network_xsync_on_demand() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["arc0"]["type"] = "archive";
    v["group"]["arc0"]["parent"] = "zone0";

    v["group"]["adv0"]["type"] = "advance";
    v["group"]["adv0"]["parent"] = "zone0";

    v["group"]["shard0"]["type"] = "validator";
    v["group"]["shard0"]["parent"] = "adv0";

    v["node"]["node0"]["parent"] = "shard0";

    v["node"]["node1"]["parent"] = "arc0";

    return v;
}

TEST(xsync_on_demand, on_behind_unit) {

    top::mbus::xmessage_bus_t mbus;
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xmock_sync_store_unit_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("");

    xrole_xips_manager_t role_xips_mgr("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);

    xsync_on_demand_t sync_on_demand("", make_observer(&mbus), make_observer(&auth), &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender);

    // add network
    Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
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

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives, table_ids);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111111");

    std::string reason = "test";

    // test1
    {
        mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_behind_on_demand_t>(account_address, 1, 100, true, reason);
        sync_on_demand.on_behind_event(e);

        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_on_demand_blocks_t>();
        ptr->serialize_from(stream);

        const std::string address;
        uint64_t start_height;
        uint32_t count;
        bool is_consensus;

        ASSERT_EQ(ptr->address, account_address);
        ASSERT_EQ(ptr->start_height, 1);
        ASSERT_EQ(ptr->count, 100);
        ASSERT_EQ(ptr->is_consensus, true);

        ASSERT_TRUE(real_part_type(dst.type()) == common::xnode_type_t::archive);
    }
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    // test2
    {
        mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_behind_on_demand_t>(account_address, 1, 100, false, reason);
        sync_on_demand.on_behind_event(e);

        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_on_demand_blocks_t>();
        ptr->serialize_from(stream);

        const std::string address;
        uint64_t start_height;
        uint32_t count;
        bool is_consensus;

        ASSERT_EQ(ptr->address, account_address);
        ASSERT_EQ(ptr->start_height, 1);
        ASSERT_EQ(ptr->count, 100);
        ASSERT_EQ(ptr->is_consensus, false);

        ASSERT_TRUE(real_part_type(dst.type()) == common::xnode_type_t::archive);
    }
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }
}

TEST(xsync_on_demand, on_response_unit) {

    top::mbus::xmessage_bus_t mbus;
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xmock_sync_store_unit_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("");

    xrole_xips_manager_t role_xips_mgr("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);

    xsync_on_demand_t sync_on_demand("", make_observer(&mbus), make_observer(&auth), &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender);

    // add network
     Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
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

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives, table_ids);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111111");

    bool sync_complete = false;
    mbus.add_listener(top::mbus::xevent_major_type_sync,
            [&](const top::mbus::xevent_ptr_t& e) {

        if (e->minor_type == top::mbus::xevent_sync_t::type_complete) {

            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_complete_t>(e);
            sync_complete = true;
            ASSERT_EQ(bme->address, account_address);
        }
    });

    std::vector<xblock_ptr_t> blocks;

    {
        base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account_address);
        std::string to_account_address = xblocktool_t::make_address_user_account("11111111111111111112");
        base::xvblock_t* prev_block = genesis_block;

        for (int i=0; i<10; i++) {
            // create a transaction
            xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
            data::xproperty_asset asset(1000000);
            tx->make_tx_transfer(asset);
            tx->set_different_source_target_address(account_address, to_account_address);
            tx->set_digest();

            xlightunit_block_para_t para;
            para.set_one_input_tx(tx);
            para.set_balance_change(-100);

            base::xvblock_t* current_block = test_blocktuil::create_next_lightunit_with_consensus(para, prev_block, 1);
            prev_block = current_block;

            {
                base::xstream_t stream(base::xcontext_t::instance());
                dynamic_cast<xblock_t*>(current_block)->full_block_serialize_to(stream);

                xblock_ptr_t block_ptr = nullptr;
                xblock_t* _data_obj = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream));
                block_ptr.attach(_data_obj);

                block_ptr->reset_block_flags();
                block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

                blocks.push_back(block_ptr);
            }
        }
    }

    ::sleep(3);
    std::string reason = "test";
    mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_behind_on_demand_t>(account_address, 1, 100, false, reason);
    sync_on_demand.on_behind_event(e);
    xsync_download_tracer tracer;
    ASSERT_EQ(sync_on_demand.download_tracer_mgr()->get(account_address, tracer), true);
    std::map<std::string, std::string> context = tracer.context();
    uint32_t i = 0;
    for (i = 0; i < archives.size(); i++){
        if (archives[i].to_string() == context["dst"]){
            break;
        }
    }
    sync_on_demand.handle_blocks_response(blocks, archives[i], self_addr);
    xmessage_t msg;
    xvnode_address_t src;
    xvnode_address_t dst;
    ASSERT_EQ(vhost.read_msg(msg, src, dst), true);
    ASSERT_EQ(sync_store.m_blocks.size(), 10);

    for (int i=0; i<10; i++) {
        ASSERT_EQ(sync_store.m_blocks[i]->get_account(), account_address);
        ASSERT_EQ(sync_store.m_blocks[i]->get_height(), i+1); 
    }
}

TEST(xsync_on_demand, on_behind_table) {

    top::mbus::xmessage_bus_t mbus;
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xmock_sync_store_unit_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("");

    xrole_xips_manager_t role_xips_mgr("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);

    xsync_on_demand_t sync_on_demand("", make_observer(&mbus), make_observer(&auth), &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender);

    // add network
    Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
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

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives, table_ids);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);

    std::string table_address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);

    std::string reason = "test";

    // test1
    {
        mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_behind_on_demand_t>(table_address, 1, 100, true, reason);
        sync_on_demand.on_behind_event(e);

        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_on_demand_blocks_t>();
        ptr->serialize_from(stream);

        const std::string address;
        uint64_t start_height;
        uint32_t count;
        bool is_consensus;

        ASSERT_EQ(ptr->address, table_address);
        ASSERT_EQ(ptr->start_height, 1);
        ASSERT_EQ(ptr->count, 100);
        ASSERT_EQ(ptr->is_consensus, true);

        ASSERT_TRUE(real_part_type(dst.type()) == common::xnode_type_t::archive);
    }
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    // test2
    {
        ::sleep(3);
        std::string reason = "test";      
        mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_behind_on_demand_t>(table_address, 1, 100, false, reason);
        sync_on_demand.on_behind_event(e);

        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_on_demand_blocks_t>();
        ptr->serialize_from(stream);

        const std::string address;
        uint64_t start_height;
        uint32_t count;
        bool is_consensus;

        ASSERT_EQ(ptr->address, table_address);
        ASSERT_EQ(ptr->start_height, 1);
        ASSERT_EQ(ptr->count, 100);
        ASSERT_EQ(ptr->is_consensus, false);

        ASSERT_TRUE(real_part_type(dst.type()) == common::xnode_type_t::archive);
    }
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }
}

TEST(xsync_on_demand, on_response_table) {

    top::mbus::xmessage_bus_t mbus;
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xmock_sync_store_unit_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("");

    xrole_xips_manager_t role_xips_mgr("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);

    xsync_on_demand_t sync_on_demand("", make_observer(&mbus), make_observer(&auth), &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender);

    // add network
     Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
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

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives, table_ids);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);

    std::string table_address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);

    bool sync_complete = false;
    mbus.add_listener(top::mbus::xevent_major_type_sync,
            [&](const top::mbus::xevent_ptr_t& e) {

        if (e->minor_type == top::mbus::xevent_sync_t::type_complete) {

            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_complete_t>(e);
            
            sync_complete = true;
            ASSERT_EQ(bme->address, table_address);
        }
    });

    std::vector<xblock_ptr_t> blocks;

    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(table_address);
    base::xvblock_t* prev_block = genesis_block;
    for (uint64_t i=0; i<10; i++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();
        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        blocks.push_back(block_ptr);
    }

    ::sleep(3);
    std::string reason = "test";
    mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_behind_on_demand_t>(table_address, 1, 10, false, reason);
    sync_on_demand.on_behind_event(e);
    xsync_download_tracer tracer;
    ASSERT_EQ(sync_on_demand.download_tracer_mgr()->get(table_address, tracer), true);
    std::map<std::string, std::string> context = tracer.context();
    uint32_t i = 0;
    for (i = 0; i< archives.size(); i++){
        if (archives[i].to_string() == context["dst"]){
            break;
        }
    }

    sync_on_demand.handle_blocks_response(blocks, archives[i], self_addr);
    sync_on_demand.on_response_event(table_address);

    ASSERT_EQ(sync_store.m_blocks.size(), 10);

    for (int i=0; i<10; i++) {
        ASSERT_EQ(sync_store.m_blocks[i]->get_account(), table_address);
        ASSERT_EQ(sync_store.m_blocks[i]->get_height(), i+1); 
    }

    ASSERT_EQ(sync_complete, true);
}

TEST(xsync_on_demand, on_behind_by_hash_unit) {

    top::mbus::xmessage_bus_t mbus;
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xmock_sync_store_unit_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("");

    xrole_xips_manager_t role_xips_mgr("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);

    xsync_on_demand_t sync_on_demand("", make_observer(&mbus), make_observer(&auth), &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender);

    // add network
    Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
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

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives, table_ids);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string hash = "11111111111111111111111111111";

    std::string reason = "test";

    // test1
    {
        mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_behind_on_demand_by_hash_t>(account_address, hash, reason);
        sync_on_demand.on_behind_event(e);

        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_on_demand_by_hash_blocks_t>();
        ptr->serialize_from(stream);

        const std::string address;
        uint64_t start_height;
        uint32_t count;
        bool is_consensus;

        ASSERT_EQ(ptr->address, account_address);
        ASSERT_EQ(ptr->hash, hash);
        ASSERT_TRUE(real_part_type(dst.type()) == common::xnode_type_t::archive);
    }

    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }
}

TEST(xsync_on_demand, on_response_by_hash_unit) {

    top::mbus::xmessage_bus_t mbus;
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xmock_sync_store_unit_t sync_store;
    xrole_chains_mgr_t role_chains_mgr("");

    xrole_xips_manager_t role_xips_mgr("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);

    xsync_on_demand_t sync_on_demand("", make_observer(&mbus), make_observer(&auth), &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender);

    // add network
     Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
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

    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    role_xips_mgr.add_role(self_addr, neighbors, parents, archives, table_ids);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_addr, table_ids);
    role_chains_mgr.add_role(role_chains);

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string hash = "11111111111111111111111111111";
    bool sync_complete = false;
    mbus.add_listener(top::mbus::xevent_major_type_sync,
            [&](const top::mbus::xevent_ptr_t& e) {

        if (e->minor_type == top::mbus::xevent_sync_t::type_complete) {

            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_complete_t>(e);
            sync_complete = true;
            ASSERT_EQ(bme->address, account_address);
        }
    });

    std::vector<xblock_ptr_t> blocks;

    {
        base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account_address);
        std::string to_account_address = xblocktool_t::make_address_user_account("11111111111111111112");
        base::xvblock_t* prev_block = genesis_block;

        for (int i=0; i<10; i++) {
            // create a transaction
            xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
            data::xproperty_asset asset(1000000);
            tx->make_tx_transfer(asset);
            tx->set_different_source_target_address(account_address, to_account_address);
            tx->set_digest();

            xlightunit_block_para_t para;
            para.set_one_input_tx(tx);
            para.set_balance_change(-100);

            base::xvblock_t* current_block = test_blocktuil::create_next_lightunit_with_consensus(para, prev_block, 1);
            prev_block = current_block;

            {
                base::xstream_t stream(base::xcontext_t::instance());
                dynamic_cast<xblock_t*>(current_block)->full_block_serialize_to(stream);

                xblock_ptr_t block_ptr = nullptr;
                xblock_t* _data_obj = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream));
                block_ptr.attach(_data_obj);

                block_ptr->reset_block_flags();
                block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

                blocks.push_back(block_ptr);
            }
        }
    }

    ::sleep(3);
    std::string reason = "test";
    mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_behind_on_demand_by_hash_t>(account_address, hash, reason);
    sync_on_demand.on_behind_event(e);
    xsync_download_tracer tracer;
    ASSERT_EQ(sync_on_demand.download_tracer_mgr()->get(account_address, tracer), true);
    std::map<std::string, std::string> context = tracer.context();
    uint32_t i = 0;
    for (i = 0; i < archives.size(); i++){
        if (archives[i].to_string() == context["dst"]){
            break;
        }
    }
    sync_on_demand.handle_blocks_by_hash_response(blocks, archives[i], self_addr);
    xmessage_t msg;
    xvnode_address_t src;
    xvnode_address_t dst;
    ASSERT_EQ(vhost.read_msg(msg, src, dst), true);
    ASSERT_EQ(sync_store.m_blocks.size(), 10);

    for (int i=0; i<10; i++) {
        ASSERT_EQ(sync_store.m_blocks[i]->get_account(), account_address);
        ASSERT_EQ(sync_store.m_blocks[i]->get_height(), i+1); 
    }
}