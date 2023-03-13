// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>
#include <cinttypes>
#include <gtest/gtest.h>
#include "xdata/xchain_param.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xblocktool.h"
#include "xmock_system.h"
#include "../../xblockstore_test/test_blockmock.hpp"
#include "../../mock/xtableblock_util.hpp"
#include "xsync/xsync_util.h"
#include "../../mock/xmock_network_config.hpp"
#include "../../mock/xmock_network.hpp"
#include "xmbus/xevent_blockfetcher.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;

using namespace top::base;
//using namespace top::vnetwork;
//using namespace top::store;
using namespace top::data;
using namespace top::mock;
using namespace top::sync;

static void do_multi_sign(std::vector<std::shared_ptr<xmock_node_t>> &shard_nodes, const xvip2_t &leader_xip, base::xvblock_t* block) {

    //printf("do multi sign : {%" PRIx64 ", %" PRIx64 "}\n", leader_xip.high_addr, leader_xip.low_addr);

    block->get_cert()->set_validator(leader_xip);

    std::map<xvip2_t,std::string,xvip2_compare> validators;

    for (auto &it: shard_nodes) {
        std::shared_ptr<xmock_node_t> &node = it;
        xvip2_t xip = node->m_addr.xip2();

        auto sign = node->m_certauth->do_sign(xip, block, base::xtime_utl::get_fast_random64());
        //block->set_verify_signature(sign);
        //xassert(get_vcertauth()->verify_sign(xip_addr, vote) == base::enum_vcert_auth_result::enum_successful);
        validators[xip] = sign;
    }

    std::string sign = shard_nodes[0]->m_certauth->merge_muti_sign(validators, block->get_cert());
    block->set_verify_signature(sign);
    block->reset_block_flags();
    block->set_block_flag(base::enum_xvblock_flag_authenticated);

    assert(shard_nodes[0]->m_certauth->verify_muti_sign(block) == base::enum_vcert_auth_result::enum_successful);
}

static void create_tableblock(xobject_ptr_t<store::xstore_face_t> &store, xobject_ptr_t<base::xvblockstore_t> &blockstore, std::vector<std::shared_ptr<xmock_node_t>> &shard_nodes, std::string &account_address, uint32_t count) {
    // create data
    test_blockmock_t blockmock(store.get());
    std::string table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});
    std::string property("election_list");
    base::xvblock_t *prev_unit_block = blockmock.create_property_block(nullptr, account_address, property);
    base::xvblock_t* prev_table_block = xblocktool_t::create_genesis_empty_table(table_address);
    base::xvaccount_t _vaddress(table_address);
    for (uint32_t i = 0; i < count; i++) {
        std::string value(std::to_string(i));
        base::xvblock_t *curr_unit_block = blockmock.create_property_block(prev_unit_block, account_address, property, value);

        std::vector<base::xvblock_t*> units;
        units.push_back(curr_unit_block);

        xvip2_t leader_xip = shard_nodes[0]->m_xip;

        base::xvblock_t* curr_table_block = xtableblock_util::create_tableblock_no_sign(units, prev_table_block, leader_xip);
        do_multi_sign(shard_nodes, leader_xip, curr_table_block);
        
        assert(blockstore->store_block(_vaddress, curr_table_block));

        base::xauto_ptr<base::xvblock_t> lock_tableblock = xblocktool_t::create_next_emptyblock(curr_table_block);
        do_multi_sign(shard_nodes, leader_xip, lock_tableblock.get());
        assert(blockstore->store_block(_vaddress, lock_tableblock.get()));

        base::xauto_ptr<base::xvblock_t> cert_tableblock = xblocktool_t::create_next_emptyblock(lock_tableblock.get());
        do_multi_sign(shard_nodes, leader_xip, cert_tableblock.get());
        assert(blockstore->store_block(_vaddress, cert_tableblock.get()));

        base::xauto_ptr<base::xvblock_t> commit_unitblock = blockstore->get_latest_committed_block(account_address);

        prev_unit_block->release_ref();
        prev_unit_block = commit_unitblock.get();
        prev_unit_block->add_ref();

        prev_table_block->release_ref();
        prev_table_block = cert_tableblock.get();;
        prev_table_block->add_ref();
    }

    prev_unit_block->release_ref();
    prev_table_block->release_ref();
}

static int duplicate_block(xobject_ptr_t<base::xvblockstore_t> &from, xobject_ptr_t<base::xvblockstore_t> &to, const std::string & address, uint64_t height) {
    base::xstream_t stream(base::xcontext_t::instance());

    base::xvaccount_t _vaddress(address);
    xblock_vector block = from->load_block_object(_vaddress, height);
    if (!block.get_vector().empty()) {
        dynamic_cast<xblock_t*>(block.get_vector()[0])->full_block_serialize_to(stream);
    } else {
        xauto_ptr<xvblock_t> block2 = from->get_latest_cert_block(_vaddress);
        if (block2 == nullptr)
            return -1;

        if (block2->get_height() != height)
            return -2;

        dynamic_cast<xblock_t*>(block2.get())->full_block_serialize_to(stream);
    }

    xblock_ptr_t block_ptr = nullptr;
    {
        xblock_t* _data_obj = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream));
        block_ptr.attach(_data_obj);
    }

    block_ptr->reset_block_flags();
    block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

    bool ret = to->store_block(_vaddress, block_ptr.get());

    if (ret)
        return 0;

    return -2;
}

// 1shard(2node)
static Json::Value test_behind() {

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

TEST(test_xsync, behind) {

    Json::Value validators = test_behind();
    xmock_network_config_t cfg_network(validators);
    xmock_network_t network(cfg_network);
    xmock_system_t sys(network);

    std::vector<std::shared_ptr<xmock_node_t>> shard_nodes = sys.get_group_node("shard0");

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});
    base::xvaccount_t _vaddress(table_address);
    create_tableblock(store, blockstore, shard_nodes, account_address, 1000);

    // set block to node2
    for (uint64_t h=1; h<=1000; h++) {
        duplicate_block(blockstore, shard_nodes[1]->m_blockstore, table_address, h);
    }

    sys.start();

    sleep(1);

    while (1) {
        sleep(1);
        base::xauto_ptr<base::xvblock_t> blk = shard_nodes[0]->m_blockstore->get_latest_cert_block(_vaddress);
        printf("height=%lu expect:%u\n", blk->get_height(), 1000);
    }
}

// 1shard(2node) + 1 archive(6node)
static Json::Value test_push_and_broadcast() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["arc0"]["type"] = "archive";
    v["group"]["arc0"]["parent"] = "zone0";

    v["group"]["adv0"]["type"] = "advance";
    v["group"]["adv0"]["parent"] = "zone0";

    v["group"]["shard0"]["type"] = "validator";
    v["group"]["shard0"]["parent"] = "adv0";

    v["node"]["node0"]["parent"] = "shard0";
    v["node"]["node1"]["parent"] = "shard0";

    v["node"]["node2"]["parent"] = "arc0";
    v["node"]["node3"]["parent"] = "arc0";
    v["node"]["node4"]["parent"] = "arc0";
    v["node"]["node5"]["parent"] = "arc0";
    v["node"]["node6"]["parent"] = "arc0";
    v["node"]["node7"]["parent"] = "arc0";

    return v;
}

TEST(test_xsync, push_and_broadcast) {

    Json::Value virtual_network = test_push_and_broadcast();
    xmock_network_config_t cfg_network(virtual_network);
    xmock_network_t network(cfg_network);
    xmock_system_t sys(network);

    std::vector<std::shared_ptr<xmock_node_t>> shard_nodes = sys.get_group_node("shard0");

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});
    base::xvaccount_t _vaddress(table_address);

    create_tableblock(store, blockstore, shard_nodes, account_address, 1000);

    sys.start();

    sleep(1);

    for (uint64_t h=1; h<=1000; h++) {
        duplicate_block(blockstore, shard_nodes[0]->m_blockstore, table_address, h);
        duplicate_block(blockstore, shard_nodes[1]->m_blockstore, table_address, h);

        base::xblock_vector vblock = blockstore->load_block_object(_vaddress, h);

        vblock.get_vector()[0]->add_ref();
        mbus::xevent_ptr_t ev =  make_object_ptr<mbus::xevent_consensus_data_t>(vblock.get_vector()[0], false);
        shard_nodes[0]->m_mbus->push_event(ev);
        shard_nodes[1]->m_mbus->push_event(ev);

        usleep(10);
    }

    sleep(1);

    std::vector<std::shared_ptr<xmock_node_t>> arc_nodes = sys.get_group_node("arc0");
    while (1) {
        for (auto &it: arc_nodes) {
            //printf("%s height=%lu\n", it->m_vnode_id.c_str(), it->m_blockstore->get_genesis_current_block(table_address)->get_height());
        }
        sleep(1);
    } 
}

// 1 archive(2node)
static Json::Value test_v1_newblockhash() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["arc0"]["type"] = "archive";
    v["group"]["arc0"]["parent"] = "zone0";

    v["node"]["node0"]["parent"] = "arc0";
    v["node"]["node1"]["parent"] = "arc0";

    return v;
}

TEST(test_xsync, v1_newblockhash) {

    Json::Value virtual_network = test_v1_newblockhash();
    xmock_network_config_t cfg_network(virtual_network);
    xmock_network_t network(cfg_network);
    xmock_system_t sys(network);

    std::vector<std::shared_ptr<xmock_node_t>> arc_nodes = sys.get_group_node("arc0");

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});
    base::xvaccount_t _vaddress(table_address);
    create_tableblock(store, blockstore, arc_nodes, account_address, 1000);

    sys.start();

    sleep(1);

    for (uint64_t h=1; h<=10; h++) {
        duplicate_block(blockstore, arc_nodes[0]->m_blockstore, table_address, h);

        xblock_vector block = blockstore->load_block_object(table_address, h);

        base::xstream_t stream(base::xcontext_t::instance());
        auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
        header->serialize_to(stream);

        auto body = make_object_ptr<sync::xsync_message_v1_newblockhash_t>(block.get_vector()[0]->get_account(), block.get_vector()[0]->get_height(), block.get_vector()[0]->get_viewid());
        body->serialize_to(stream);
        vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_v1_newblockhash);

        xmessage_t msg;
        xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= DEFAULT_MIN_COMPRESS_THRESHOLD, msg);

        arc_nodes[1]->m_vnet->on_message(arc_nodes[0]->m_addr, msg);

        usleep(10);
    }

    sleep(1);

    while (1) {
        //printf("%s height=%lu\n", arc_nodes[1]->m_vnode_id.c_str(), arc_nodes[1]->m_blockstore->get_genesis_current_block(table_address)->get_height());
        sleep(1);
    } 
}

#if 0
class test_filter_t {
public:
    test_filter_t(const std::string &table_address, const xobject_ptr_t<base::xvblockstore_t> &blockstore, const std::vector<std::shared_ptr<xmock_node_t>> &shard_nodes):
    m_table_address(table_address),
    m_blockstore(blockstore),
    m_shard_nodes(shard_nodes) {
    }

    int32_t filter(const vnetwork::xmessage_t &in, vnetwork::xmessage_t &out) {

        auto const message_id = in.id();

        // discard this request and send lower newblock message
        if (message_id == xmessage_id_sync_get_blocks) {

            xbyte_buffer_t message;
            xmessage_pack_t::unpack_message(in.payload(), message);
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

            xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
            header->serialize_from(stream);

            auto ptr = make_object_ptr<xsync_message_get_blocks_t>();
            ptr->serialize_from(stream);

            const std::string &owner = ptr->owner;
            uint64_t start_height = ptr->start_height;
            uint32_t count = ptr->count;

            bool filter = false;
            if (start_height > 70 && start_height < 100 && !filter) {
                filter = true;
                send_lower_newblock_message();
                printf("send lower newblock\n");
                return -1;
            }
        }

        return 1;
    }

private:
    void send_lower_newblock_message() {

        uint64_t height = 95;
        base::xauto_ptr<base::xvblock_t> block = m_blockstore->load_block_object(m_table_address, height);

        base::xstream_t stream(base::xcontext_t::instance());
        auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
        header->serialize_to(stream);

        xblock_ptr_t block_ptr = autoptr_to_blockptr(block);
        auto body = make_object_ptr<sync::xsync_message_push_newblock_t>(block_ptr);
        body->serialize_to(stream);
        vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_push_newblock);

        xmessage_t msg;
        xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= DEFAULT_MIN_COMPRESS_THRESHOLD, msg);

        m_shard_nodes[0]->m_vnet->on_message(m_shard_nodes[1]->m_addr, msg);
    }

private:
    std::string m_table_address;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore{};
    std::vector<std::shared_ptr<xmock_node_t>> m_shard_nodes;
};

// 1shard(2node)
static Json::Value test_exception() {

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

TEST(test_xsync, exception) {

    Json::Value validators = test_exception();
    xmock_network_config_t cfg_network(validators);
    xmock_network_t network(cfg_network);
    xmock_system_t sys(network);

    std::vector<std::shared_ptr<xmock_node_t>> shard_nodes = sys.get_group_node("shard0");

    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore = nullptr;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store, ""));

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});

    create_tableblock(store, blockstore, shard_nodes, account_address, 1000);

    sys.start();

    sleep(1);

    // set filter
    test_filter_t filter(table_address, blockstore, shard_nodes);
    shard_nodes[1]->m_vhost->set_recv_filter(std::bind(&test_filter_t::filter, &filter, std::placeholders::_1, std::placeholders::_2));

    {
        // set block to node2
        for (uint64_t h=1; h<=100; h++) {
            duplicate_block(blockstore, shard_nodes[1]->m_blockstore, table_address, h);
        }

        // tell node1 table chain is behind and sync from node2
        mbus::xevent_behind_ptr_t ev = std::make_shared<mbus::xevent_behind_check_t>(
                    table_address, mbus::enum_behind_type_common, "test");
        shard_nodes[0]->m_mbus->push_event(ev);
    }

    sleep(1);

    {
        // set block to node2
        for (uint64_t h=101; h<=200; h++) {
            duplicate_block(blockstore, shard_nodes[1]->m_blockstore, table_address, h);
        }

        // tell node1 table chain is behind and sync from node2
        mbus::xevent_behind_ptr_t ev = std::make_shared<mbus::xevent_behind_check_t>(
                    table_address, mbus::enum_behind_type_common, "test");
        shard_nodes[0]->m_mbus->push_event(ev);
    }

    sleep(2);
    //base::xauto_ptr<base::xvblock_t> blk = shard_nodes[0]->m_blockstore->get_latest_current_block(table_address);
    //assert(blk->get_height() == 200);

    while (1)
        sleep(1);
}
#endif

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

TEST(test_xsync, on_demand_sync_unit_no_consensus) {

    Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
    xmock_network_t network(cfg_network);
    xmock_system_t sys(network);

    std::vector<std::shared_ptr<xmock_node_t>> shard_nodes = sys.get_group_node("shard0");
    std::vector<std::shared_ptr<xmock_node_t>> archive_nodes = sys.get_group_node("arc0");

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});
    base::xvaccount_t _vaddress(table_address);

    create_tableblock(store, blockstore, shard_nodes, account_address, 100);

    base::xauto_ptr<base::xvblock_t> last_table_block = blockstore->get_latest_cert_block(_vaddress);
    for (uint64_t h=1 ; h<=last_table_block->get_height(); h++) {
        duplicate_block(blockstore, archive_nodes[0]->m_blockstore, table_address, h);
    }

    sys.start();

    sleep(1);

    mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(
                    account_address, 1, 99, false, "test");
    shard_nodes[0]->m_mbus->push_event(ev);

    base::xvaccount_t account_vaddress(account_address);
    while (1) {
        base::xauto_ptr<base::xvblock_t> blk_current = shard_nodes[0]->m_blockstore->get_latest_cert_block(account_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_commit = shard_nodes[0]->m_blockstore->get_latest_committed_block(account_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_lock = shard_nodes[0]->m_blockstore->get_latest_locked_block(account_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_cert = shard_nodes[0]->m_blockstore->get_latest_cert_block(account_vaddress);
        printf("height current=%lu %lu,%lu,%lu\n", blk_current->get_height(), blk_commit->get_height(), blk_lock->get_height(), blk_cert->get_height());
        sleep(1);
    }
}

TEST(test_xsync, on_demand_sync_unit_consensus) {

    Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
    xmock_network_t network(cfg_network);
    xmock_system_t sys(network);

    std::vector<std::shared_ptr<xmock_node_t>> shard_nodes = sys.get_group_node("shard0");
    std::vector<std::shared_ptr<xmock_node_t>> archive_nodes = sys.get_group_node("arc0");

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});
    base::xvaccount_t _vaddress(table_address);

    create_tableblock(store, blockstore, shard_nodes, account_address, 100);

    base::xauto_ptr<base::xvblock_t> last_table_block = blockstore->get_latest_cert_block(_vaddress);
    for (uint64_t h=1 ; h<=last_table_block->get_height(); h++) {
        duplicate_block(blockstore, archive_nodes[0]->m_blockstore, table_address, h);
    }

    sys.start();

    sleep(1);

    mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(
                    account_address, 86, 100, true, "test");
    shard_nodes[0]->m_mbus->push_event(ev);

    base::xvaccount_t account_vaddress(account_address);
    while (1) {
        base::xauto_ptr<base::xvblock_t> blk_current = shard_nodes[0]->m_blockstore->get_latest_cert_block(account_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_commit = shard_nodes[0]->m_blockstore->get_latest_committed_block(account_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_lock = shard_nodes[0]->m_blockstore->get_latest_locked_block(account_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_cert = shard_nodes[0]->m_blockstore->get_latest_cert_block(account_vaddress);
        printf("height current=%lu %lu,%lu,%lu\n", blk_current->get_height(), blk_commit->get_height(), blk_lock->get_height(), blk_cert->get_height());
        sleep(1);
    }
}

TEST(test_xsync, on_demand_sync_table_no_consensus) {

    Json::Value network_on_demand = build_network_xsync_on_demand();
    xmock_network_config_t cfg_network(network_on_demand);
    xmock_network_t network(cfg_network);
    xmock_system_t sys(network);

    std::vector<std::shared_ptr<xmock_node_t>> shard_nodes = sys.get_group_node("shard0");
    std::vector<std::shared_ptr<xmock_node_t>> archive_nodes = sys.get_group_node("arc0");

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});
    base::xvaccount_t _vaddress(table_address);

    create_tableblock(store, blockstore, shard_nodes, account_address, 100);

    base::xauto_ptr<base::xvblock_t> last_table_block = blockstore->get_latest_cert_block(_vaddress);
    for (uint64_t h=1 ; h<=last_table_block->get_height(); h++) {
        duplicate_block(blockstore, archive_nodes[0]->m_blockstore, table_address, h);
    }

    sys.start();

    sleep(1);

    mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(
                    table_address, 1, 99, false, "test");
    shard_nodes[0]->m_mbus->push_event(ev);

    while (1) {
        sleep(1);
        base::xauto_ptr<base::xvblock_t> blk_current = shard_nodes[0]->m_blockstore->get_latest_cert_block(_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_commit = shard_nodes[0]->m_blockstore->get_latest_committed_block(_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_lock = shard_nodes[0]->m_blockstore->get_latest_locked_block(_vaddress);
        base::xauto_ptr<base::xvblock_t> blk_cert = shard_nodes[0]->m_blockstore->get_latest_cert_block(_vaddress);
        printf("height current=%lu %lu,%lu,%lu\n", blk_current->get_height(), blk_commit->get_height(), blk_lock->get_height(), blk_cert->get_height());
    }
}