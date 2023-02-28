#include <gtest/gtest.h>
#include "xdata/tests/test_blockutl.hpp"
#include "../mock/xmock_network_config.hpp"
#include "../mock/xmock_network.hpp"
#include "xsync/xsync_cross_cluster_chain_state.h"
#include "common.h"
#include "xsync/xsync_util.h"
#include "xcommon/xmessage_id.h"
#include "tests/mock/xvchain_creator.hpp"

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

static Json::Value build_beacons() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["rec0"]["type"] = "beacon";
    v["group"]["rec0"]["parent"] = "zone0";

    v["node"]["node5"]["parent"] = "rec0";
    v["node"]["node6"]["parent"] = "rec0";
    v["node"]["node7"]["parent"] = "rec0";
    v["node"]["node8"]["parent"] = "rec0";
    v["node"]["node9"]["parent"] = "rec0";

    return v;
}

static Json::Value build_zecs() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["zec0"]["type"] = "zec";
    v["group"]["zec0"]["parent"] = "zone0";

    v["node"]["node10"]["parent"] = "zec0";
    v["node"]["node11"]["parent"] = "zec0";
    v["node"]["node12"]["parent"] = "zec0";
    v["node"]["node13"]["parent"] = "zec0";
    v["node"]["node14"]["parent"] = "zec0";

    return v;
}

static Json::Value build_archives() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["arc0"]["type"] = "archive";
    v["group"]["arc0"]["parent"] = "zone0";

    v["node"]["node15"]["parent"] = "arc0";
    v["node"]["node16"]["parent"] = "arc0";
    v["node"]["node17"]["parent"] = "arc0";
    v["node"]["node18"]["parent"] = "arc0";
    v["node"]["node19"]["parent"] = "arc0";

    return v;
}

class xcross_cluster_mock_downloader_t : public xdownloader_face_t {
public:
    void push_event(const mbus::xevent_ptr_t &e) override {
        if (e->major_type==xevent_major_type_behind && e->minor_type==xevent_behind_download_t::type_download) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_download_t>(e);
            m_counter++;
        }
    }

    int m_counter{0};
};

TEST(xsync_cross_cluster_chain_state, test) {

    std::unique_ptr<mbus::xmessage_bus_face_t> mbus = top::make_unique<mbus::xmessage_bus_t>();
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    xsync_store_t sync_store("", make_observer(blockstore));
    xrole_chains_mgr_t role_chains_mgr("");
    xrole_xips_manager_t role_xips_mgr("");
    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xcross_cluster_mock_downloader_t downloader;

    xsync_cross_cluster_chain_state_t cross_cluster_chain_state("", &sync_store, &role_chains_mgr, &role_xips_mgr, &sync_sender, &downloader);

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

    // add validator
    {
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

        role_chains_mgr.add_role(role_chains);

        std::vector<common::xnode_address_t> neighbor_addresses;
        std::vector<common::xnode_address_t> parent_addresses;
        for (uint32_t i=1; i<addr_list.size(); i++) {
            neighbor_addresses.push_back(addr_list[i]);
        }

        role_xips_mgr.add_role(self_address, neighbor_addresses, parent_addresses, archive_addresses, table_ids);
    }
#if 0
    // add beacons
    {
        Json::Value beacons = build_beacons();
        xmock_network_config_t cfg_network(beacons);
        xmock_network_t network(cfg_network);

        std::vector<vnetwork::xvnode_address_t> addr_list;
        {
            std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();
            for (auto &it: nodes)
                addr_list.push_back(it->m_addr);
        }

        common::xnode_address_t self_address = addr_list[0];
        bool ret = false;

        std::vector<uint16_t> table_ids;
        for (uint16_t i=0; i<256; i++)
            table_ids.push_back(i);

        std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_address, table_ids);
        const map_chain_info_t & chains = role_chains->get_chains_wrapper().get_chains();

        role_chains_mgr.add_role(role_chains);

        std::vector<common::xnode_address_t> neighbor_addresses;
        std::vector<common::xnode_address_t> parent_addresses;
        for (uint32_t i=1; i<addr_list.size(); i++) {
            neighbor_addresses.push_back(addr_list[i]);
        }

        role_xips_mgr.add_role(self_address, neighbor_addresses, parent_addresses, archive_addresses);
    }

    // add zec
    {
        Json::Value zecs = build_zecs();
        xmock_network_config_t cfg_network(zecs);
        xmock_network_t network(cfg_network);

        std::vector<vnetwork::xvnode_address_t> addr_list;
        {
            std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();
            for (auto &it: nodes)
                addr_list.push_back(it->m_addr);
        }

        common::xnode_address_t self_address = addr_list[0];
        bool ret = false;

        std::vector<uint16_t> table_ids;
        for (uint16_t i=0; i<256; i++)
            table_ids.push_back(i);

        std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_address, table_ids);
        const map_chain_info_t & chains = role_chains->get_chains_wrapper().get_chains();

        role_chains_mgr.add_role(role_chains);

        std::vector<common::xnode_address_t> neighbor_addresses;
        std::vector<common::xnode_address_t> parent_addresses;
        for (uint32_t i=1; i<addr_list.size(); i++) {
            neighbor_addresses.push_back(addr_list[i]);
        }

        role_xips_mgr.add_role(self_address, neighbor_addresses, parent_addresses, archive_addresses);
    }
#endif
    map_chain_info_t all_chains = role_chains_mgr.get_all_chains();
    for (const auto &it: all_chains) {
        const std::string &address = it.first;

        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
        base::xvblock_t* prev_block = genesis_block;
        for (uint64_t i=1; i<=3; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();
            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
            sync_store.store_block(block_ptr.get());
        }
    }

    for (uint32_t i=1; i<=599; i++) {
        cross_cluster_chain_state.on_timer();
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    {
        cross_cluster_chain_state.on_timer();
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        // src is consensus, dst is archive

        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_cross_cluster_chain_state);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_chain_state_info_t>();
        ptr->serialize_from(stream);

        std::vector<xchain_state_info_t> &info_list = ptr->info_list;

        std::set<std::string> chain_set;
        for (const auto &it: all_chains) {
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

    {
        std::vector<xchain_state_info_t> info_list;
        for (auto &it: all_chains) {
            xchain_state_info_t info;
            info.address = it.first;
            info.start_height = 0;
            info.end_height = 6;

            info_list.push_back(info);
        }

        vnetwork::xvnode_address_t network_self;
        vnetwork::xvnode_address_t from_address;
        cross_cluster_chain_state.handle_message(network_self, from_address, info_list);

        ASSERT_EQ(downloader.m_counter, info_list.size());
    }
}