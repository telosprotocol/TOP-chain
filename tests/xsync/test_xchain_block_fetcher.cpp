#include <gtest/gtest.h>
#include "xsync/xblock_fetcher.h"
#include "xsync/xsync_sender.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xsync/xsync_util.h"
#include "xsync/xchain_block_fetcher.h"
#include "../mock/xmock_auth.hpp"
#include "common.h"
#include "xcommon/xmessage_id.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;

TEST(xchain_block_fetcher, block) {
    std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xrole_xips_manager_t role_xips_mgr("");
    xsync_peerset_t peerset("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xsync_broadcast_t sync_broadcast("", &peerset, &sync_sender);

    std::unique_ptr<mbus::xmessage_bus_face_t> mbus = top::make_unique<mbus::xmessage_bus_t>();
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    xsync_store_t sync_store("", make_observer(blockstore));

    xchain_block_fetcher_t chain_block_fetcher("", address, make_observer(&auth), &sync_store, &sync_broadcast, &sync_sender);

    xblock_ptr_t block4 = nullptr;

    // set block
    {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
        base::xvblock_t* prev_block = genesis_block;
        for (uint64_t i=1; i<=3; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();
            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
            sync_store.store_block(block_ptr.get());
        }

        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();
        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        block4 = autoptr_to_blockptr(autoptr);
    }

    vnetwork::xvnode_address_t network_self;
    vnetwork::xvnode_address_t from_address;
    chain_block_fetcher.on_newblock(block4, network_self, from_address);

    base::xauto_ptr<base::xvblock_t> current_block = sync_store.get_latest_end_block(address, enum_chain_sync_policy_full);
    ASSERT_EQ(current_block->get_height(), 4);
}

TEST(xchain_block_fetcher, block_hash) {
    std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xrole_xips_manager_t role_xips_mgr("");
    xsync_peerset_t peerset("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xsync_broadcast_t sync_broadcast("", &peerset, &sync_sender);

    std::unique_ptr<mbus::xmessage_bus_face_t> mbus = top::make_unique<mbus::xmessage_bus_t>();
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    
    xsync_store_t sync_store("", make_observer(blockstore));

    xchain_block_fetcher_t chain_block_fetcher("", address, make_observer(&auth), &sync_store, &sync_broadcast, &sync_sender);

    xblock_ptr_t block4 = nullptr;
    xblock_ptr_t block5 = nullptr;
    xblock_ptr_t block6 = nullptr;

    // set block
    {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
        base::xvblock_t* prev_block = genesis_block;
        for (uint64_t i=1; i<=6; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();
            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);

            if (i<=3)
                sync_store.store_block(block_ptr.get());
            else {
                if (i == 4)
                    block4 = block_ptr;
                else if (i == 5)
                    block5 = block_ptr;
                else if (i == 6)
                    block6 = block_ptr;
            }
        }
    }

    // on newblockhash
    vnetwork::xvnode_address_t network_self;
    vnetwork::xvnode_address_t target_address;
    chain_block_fetcher.on_newblockhash(block4->get_height(), block4->get_viewid(), block4->get_block_hash(), network_self, target_address);
    chain_block_fetcher.on_newblockhash(block5->get_height(), block5->get_viewid(), block5->get_block_hash(), network_self, target_address);

    // less than 400ms
    chain_block_fetcher.on_timer();
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    // send request
    sleep(1);
    chain_block_fetcher.on_timer();
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_get_blocks_by_hashes);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_blocks_by_hashes_t>();
        ptr->serialize_from(stream);

        std::vector<xblock_hash_t> &info_list = ptr->info_list;
        ASSERT_EQ(info_list.size(), 1);
        ASSERT_EQ(info_list[0].address, address);
        ASSERT_EQ(info_list[0].height, block4->get_height());
        ASSERT_EQ(info_list[0].hash, block4->get_block_hash());
    }

    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_get_blocks_by_hashes);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_blocks_by_hashes_t>();
        ptr->serialize_from(stream);

        std::vector<xblock_hash_t> &info_list = ptr->info_list;
        ASSERT_EQ(info_list.size(), 1);
        ASSERT_EQ(info_list[0].address, address);
        ASSERT_EQ(info_list[0].height, block5->get_height());
        ASSERT_EQ(info_list[0].hash, block5->get_block_hash());
    }

    // on response
    chain_block_fetcher.on_response_blocks(block4, network_self, target_address);
    {
        base::xauto_ptr<base::xvblock_t> current_block = sync_store.get_latest_end_block(address, enum_chain_sync_policy_full);
        ASSERT_EQ(current_block->get_height(), 4);
    }

    chain_block_fetcher.on_response_blocks(block5, network_self, target_address);
    {
        base::xauto_ptr<base::xvblock_t> current_block = sync_store.get_latest_end_block(address, enum_chain_sync_policy_full);
        ASSERT_EQ(current_block->get_height(), 5);
    }

    chain_block_fetcher.on_response_blocks(block6, network_self, target_address);
    {
        base::xauto_ptr<base::xvblock_t> current_block = sync_store.get_latest_end_block(address, enum_chain_sync_policy_full);
        ASSERT_EQ(current_block->get_height(), 5);
    }
}

TEST(xchain_block_fetcher, timeout) {
    std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);
    top::mock::xmock_auth_t auth{1};
    xmock_vhost_sync_t vhost;

    xrole_xips_manager_t role_xips_mgr("");
    xsync_peerset_t peerset("");
    xsync_sender_t sync_sender("", make_observer(&vhost), &role_xips_mgr);
    xsync_broadcast_t sync_broadcast("", &peerset, &sync_sender);

    std::unique_ptr<mbus::xmessage_bus_face_t> mbus = top::make_unique<mbus::xmessage_bus_t>();

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    xsync_store_t sync_store("", make_observer(blockstore));

    xchain_block_fetcher_t chain_block_fetcher("", address, make_observer(&auth), &sync_store, &sync_broadcast, &sync_sender);

    xblock_ptr_t block4 = nullptr;

    // set block
    {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
        base::xvblock_t* prev_block = genesis_block;
        for (uint64_t i=1; i<=4; i++) {
            prev_block = test_blocktuil::create_next_emptyblock(prev_block);
            prev_block->add_ref();
            base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);

            if (i<=3)
                sync_store.store_block(block_ptr.get());
            else {
                if (i == 4)
                    block4 = block_ptr;
            }
        }
    }

    // on newblockhash
    vnetwork::xvnode_address_t network_self;
    vnetwork::xvnode_address_t target_address;
    chain_block_fetcher.on_newblockhash(block4->get_height(), block4->get_viewid(), block4->get_block_hash(), network_self, target_address);

    // send request
    sleep(1);
    chain_block_fetcher.on_timer();
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);

        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_get_blocks_by_hashes);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_blocks_by_hashes_t>();
        ptr->serialize_from(stream);

        std::vector<xblock_hash_t> &info_list = ptr->info_list;
        ASSERT_EQ(info_list.size(), 1);
        ASSERT_EQ(info_list[0].address, address);
        ASSERT_EQ(info_list[0].height, block4->get_height());
        ASSERT_EQ(info_list[0].hash, block4->get_block_hash());
    }

    sleep(5);

    // check request
    chain_block_fetcher.on_timer();
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }
}