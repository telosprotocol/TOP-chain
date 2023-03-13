#include <gtest/gtest.h>
#include "xsync/xchain_downloader.h"
#include "xsync/xsync_sender.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xsync/xsync_util.h"
#include "../mock/xmock_auth.hpp"
#include "xsync/xsync_message.h"
#include "common.h"
#include "xmbus/xevent_executor.h"
#include "xsync/xsync_behind_checker.h"
#include "../mock/xmock_network_config.hpp"
#include "../mock/xmock_network.hpp"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;
using namespace top::mock;

class xmock_downloader_t : public xdownloader_face_t {
public:
    void push_event(const mbus::xevent_ptr_t &e) override {
        if (e->major_type==xevent_major_type_behind && e->minor_type==xevent_behind_download_t::type_download) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_download_t>(e);
            m_counter++;
        }
    }

    int m_counter{0};
};

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

TEST(xsync_behind_checker, test) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    xsync_store_t sync_store("", make_observer(blockstore));

    xrole_chains_mgr_t role_chains_mgr("");
    xsync_peerset_t peerset("");
    xmock_downloader_t downloader;

    xsync_behind_checker_t checker("", &sync_store, &role_chains_mgr, &peerset, &downloader);

    // create network
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
    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<256; i++)
        table_ids.insert(i);

    std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(self_address, table_ids);
    const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();

     // add block
    uint64_t chain_height_base = 0;
    for (const auto &it: chains) {
        const std::string &address = it.first;
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
        EXPECT_EQ(sync_store.store_block(genesis_block), true);
    }
#if 0
    // add block
    uint64_t chain_height_base = 0;
    for (const auto &it: chains) {
        const std::string &address = it.first;
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
        base::xvblock_t* prev_block = genesis_block;
        for (uint64_t i=1; i<=1; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();
        }

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        sync_store.m_blocks[address] = block_ptr;
    }
#endif

    // add role
    role_chains_mgr.add_role(role_chains);
    peerset.add_group(self_address);
    for (uint32_t i=1; i<addr_list.size(); i++)
        peerset.add_peer(self_address, addr_list[i]);

    // update peer info
    for (uint32_t i=1; i<addr_list.size(); i++) {
        std::vector<xchain_state_info_t> info_list;

        for (auto &it: chains) {
            xchain_state_info_t info;
            info.address = it.first;
            info.start_height = 0;
            info.end_height = 1;
            info_list.push_back(info);
        }

        peerset.update(self_address, addr_list[i], info_list);
    }

    // repeat check 100 times
    for (uint32_t a = 0; a<100; a++) {
        // it's 10s timeout 
        for (uint32_t i=1; i<=9; i++) {
            checker.on_timer();
            ASSERT_EQ(downloader.m_counter, 0);
        }

        checker.on_timer();
        ASSERT_EQ(downloader.m_counter, chains.size());
        downloader.m_counter = 0;
    }
}
