#include <gtest/gtest.h>
#include "xsync/xsync_v1_block_fetcher.h"
#include "xsync/xsync_sender.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xsync/xsync_util.h"
#include "xsync/xchain_info.h"
#include "../mock/xmock_auth.hpp"
#include "common.h"
#include "xcommon/xmessage_id.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;

TEST(xsync_v1_block_fetcher, not_exist1) {

    std::string address = xdatautil::serialize_owner_str(common::rec_table_base_address.to_string(), 0);

    top::mock::xmock_auth_t auth{1};

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());

    xsync_store_t sync_store("", make_observer(blockstore));

    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), nullptr);

    xsync_v1_block_fetcher_t block_fetcher("", make_observer(&auth), &sync_store, &sync_sender);

    std::vector<data::xblock_ptr_t> block_vector;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
    base::xvblock_t* prev_block = genesis_block;
    prev_block->add_ref();
    base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
    xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
    block_vector.push_back(block_ptr);

    for (uint64_t i=1; i<=5; i++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        block_vector.push_back(block_ptr);
    }

    vnetwork::xvnode_address_t from_address;
    vnetwork::xvnode_address_t network_self;

    block_fetcher.handle_v1_newblockhash(address, block_vector[1]->get_height(), block_vector[1]->get_viewid(), from_address, network_self);

    block_fetcher.on_timer_check_v1_newblockhash();
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    sleep(1);

    block_fetcher.on_timer_check_v1_newblockhash();

    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);
        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_get_blocks);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);

        ASSERT_EQ(ptr->owner, address);
        ASSERT_EQ(ptr->start_height, block_vector[1]->get_height());
        ASSERT_EQ(ptr->count, 1);

        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    ASSERT_FALSE(block_fetcher.filter_block(block_vector[2]));
    ASSERT_TRUE(block_fetcher.filter_block(block_vector[1]));

    base::xauto_ptr<base::xvblock_t> blk = sync_store.query_block(address, block_vector[1]->get_height(), block_vector[1]->get_block_hash());
    ASSERT_TRUE(blk != nullptr);
}

TEST(xsync_v1_block_fetcher, not_exist2) {

    std::string address = xdatautil::serialize_owner_str(common::rec_table_base_address.to_string(), 0);

    top::mock::xmock_auth_t auth{1};

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    xsync_store_t sync_store("", make_observer(blockstore));

    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), nullptr);

    xsync_v1_block_fetcher_t block_fetcher("", make_observer(&auth), &sync_store, &sync_sender);

    std::vector<data::xblock_ptr_t> block_vector;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
    base::xvblock_t* prev_block = genesis_block;
    prev_block->add_ref();
    base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
    xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
    block_vector.push_back(block_ptr);

    for (uint64_t i=1; i<=5; i++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        block_vector.push_back(block_ptr);
    }

    // set block 2
    sync_store.store_block(block_vector[2].get());

    vnetwork::xvnode_address_t from_address;
    vnetwork::xvnode_address_t network_self;

    block_fetcher.handle_v1_newblockhash(address, block_vector[1]->get_height(), block_vector[1]->get_viewid(), from_address, network_self);

    block_fetcher.on_timer_check_v1_newblockhash();
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    sleep(1);

    block_fetcher.on_timer_check_v1_newblockhash();

    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);
        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_get_blocks);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);

        ASSERT_EQ(ptr->owner, address);
        ASSERT_EQ(ptr->start_height, block_vector[1]->get_height());
        ASSERT_EQ(ptr->count, 1);

        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    ASSERT_FALSE(block_fetcher.filter_block(block_vector[2]));
    ASSERT_TRUE(block_fetcher.filter_block(block_vector[1]));

    base::xauto_ptr<base::xvblock_t> blk = sync_store.query_block(address, block_vector[1]->get_height(), block_vector[1]->get_block_hash());
    ASSERT_TRUE(blk != nullptr);
}

TEST(xsync_v1_block_fetcher, exist) {

    std::string address = xdatautil::serialize_owner_str(common::rec_table_base_address.to_string(), 0);

    top::mock::xmock_auth_t auth{1};

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    xsync_store_t sync_store("", make_observer(blockstore));

    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), nullptr);

    xsync_v1_block_fetcher_t block_fetcher("", make_observer(&auth), &sync_store, &sync_sender);

    std::vector<data::xblock_ptr_t> block_vector;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
    base::xvblock_t* prev_block = genesis_block;
    prev_block->add_ref();
    base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
    xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
    block_vector.push_back(block_ptr);

    for (uint64_t i=1; i<=5; i++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        block_vector.push_back(block_ptr);
    }

    // set block 1
    sync_store.store_block(block_vector[1].get());

    vnetwork::xvnode_address_t from_address;
    vnetwork::xvnode_address_t network_self;

    block_fetcher.handle_v1_newblockhash(address, block_vector[1]->get_height(), block_vector[1]->get_viewid(), from_address, network_self);

    block_fetcher.on_timer_check_v1_newblockhash();
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    sleep(1);

    block_fetcher.on_timer_check_v1_newblockhash();

    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    base::xauto_ptr<base::xvblock_t> blk = sync_store.query_block(address, block_vector[1]->get_height(), block_vector[1]->get_block_hash());
    ASSERT_TRUE(blk != nullptr);
}

TEST(xsync_v1_block_fetcher, timeout) {

    std::string address = xdatautil::serialize_owner_str(common::rec_table_base_address.to_string(), 0);

    top::mock::xmock_auth_t auth{1};

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    xsync_store_t sync_store("", make_observer(blockstore));

    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), nullptr);

    xsync_v1_block_fetcher_t block_fetcher("", make_observer(&auth), &sync_store, &sync_sender);

    std::vector<data::xblock_ptr_t> block_vector;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);
    base::xvblock_t* prev_block = genesis_block;
    prev_block->add_ref();
    base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
    xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
    block_vector.push_back(block_ptr);

    for (uint64_t i=1; i<=5; i++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        block_vector.push_back(block_ptr);
    }

    vnetwork::xvnode_address_t from_address;
    vnetwork::xvnode_address_t network_self;

    block_fetcher.handle_v1_newblockhash(address, block_vector[1]->get_height(), block_vector[1]->get_viewid(), from_address, network_self);

    block_fetcher.on_timer_check_v1_newblockhash();
    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    sleep(1);

    block_fetcher.on_timer_check_v1_newblockhash();

    {
        xmessage_t msg;
        xvnode_address_t src;
        xvnode_address_t dst;
        ASSERT_EQ(vhost.read_msg(msg, src, dst), true);
        vnetwork::xmessage_t::message_type msg_type = msg.id();
        ASSERT_EQ(msg_type, xmessage_id_sync_get_blocks);

        xbyte_buffer_t message;
        xmessage_pack_t::unpack_message(msg.payload(), message);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());

        auto header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);

        auto ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);

        ASSERT_EQ(ptr->owner, address);
        ASSERT_EQ(ptr->start_height, block_vector[1]->get_height());
        ASSERT_EQ(ptr->count, 1);

        ASSERT_EQ(vhost.read_msg(msg, src, dst), false);
    }

    sleep(6);

    block_fetcher.on_timer_check_v1_newblockhash();

    ASSERT_FALSE(block_fetcher.filter_block(block_vector[1]));

    base::xauto_ptr<base::xvblock_t> blk = sync_store.query_block(address, block_vector[1]->get_height(), block_vector[1]->get_block_hash());
    ASSERT_TRUE(blk == nullptr);
}
