#include <gtest/gtest.h>
#include "xsync/xaccount.h"
#include "xsync/xsync_sender.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xsync/xsync_util.h"
#include "../mock/xmock_auth.hpp"
#include "xsync/xsync_message.h"
#include "common.h"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;

class xmock_store_t : public xsync_store_face_mock_t {
public:

    base::xauto_ptr<base::xvblock_t> get_current_block(const std::string & account) override {
        current_block->add_ref();
        return current_block;
    }

    bool store_block(base::xvblock_t* block) override {
        if (block->get_last_block_hash() == current_block->get_block_hash()) {
            current_block->release_ref();
            block->add_ref();
            current_block = block;
            return true;
        }

        return false;
    }

public:
    base::xvblock_t* current_block{};
};

class xsync_mock_ratelimit_t : public xsync_ratelimit_face_t {
public:
    void start() override {
    }
    void stop() override {
    }
    bool consume(int64_t now) override {
        return true;
    }
    void on_response(uint32_t cost, int64_t now) override {
    }
};

TEST(xsync_account, no_response) {

    std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);
    xmock_store_t store;
    xmessage_bus_t mbus;
    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), nullptr);
    xsync_mock_ratelimit_t ratelimit;

    xchain_info_t chain_info;
    chain_info.address = address;


    std::vector<base::xvblock_t*> block_vector;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

    base::xvblock_t* prev_block = genesis_block;
    block_vector.push_back(prev_block);

    for (uint64_t i=1; i<=5; i++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        block_vector.push_back(prev_block);
    }

    store.current_block = genesis_block;

    top::mock::xmock_auth_t auth{1};

    xaccount_face_ptr_t account = std::make_shared<xaccount_general_t>("", &store, make_observer(&mbus), make_observer(&auth), &sync_sender, &ratelimit, chain_info);


    block_vector[4]->add_ref();
    base::xauto_ptr<base::xvblock_t> vblock4 = block_vector[4];
    xblock_ptr_t successor_block4 = autoptr_to_blockptr(vblock4);

    block_vector[5]->add_ref();
    base::xauto_ptr<base::xvblock_t> vblock5 = block_vector[5];
    xblock_ptr_t successor_block5 = autoptr_to_blockptr(vblock5);


    top::common::xnode_address_t network_self;
    top::common::xnode_address_t target_address;

    mbus::xevent_ptr_t ev4 = std::make_shared<mbus::xevent_behind_block_t>(successor_block4, enum_behind_source_consensus, "", network_self, target_address);
    mbus::xevent_ptr_t ev5 = std::make_shared<mbus::xevent_behind_block_t>(successor_block5, enum_behind_source_consensus, "", network_self, target_address);

    account->on_behind_event(ev4);

    account->on_behind_event(ev5);
}

static xblock_ptr_t copy_block(base::xvblock_t *block) {
    base::xstream_t stream(base::xcontext_t::instance());
    {
        dynamic_cast<xblock_t*>(block)->full_block_serialize_to(stream);
    }

    xblock_ptr_t block_ptr = nullptr;
    {
        xblock_t* _data_obj = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream));
        block_ptr.attach(_data_obj);
    }

    block_ptr->reset_block_flags();
    block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

    return block_ptr;
}

TEST(xsync_account, highqc_fork) {

    std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);

    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore = nullptr;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store, ""));

    xsync_store_t sync_store("", make_observer(blockstore));
    xmessage_bus_t mbus;
    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), nullptr);
    xsync_mock_ratelimit_t ratelimit;

    xchain_info_t chain_info;
    chain_info.address = address;

    top::mock::xmock_auth_t auth{1};

    xaccount_face_ptr_t account = std::make_shared<xaccount_general_t>("", &sync_store, make_observer(&mbus), make_observer(&auth), &sync_sender, &ratelimit, chain_info);

    std::vector<base::xvblock_t*> block_vector_1;
    std::vector<base::xvblock_t*> block_vector_2;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

    base::xvblock_t* prev_block = genesis_block;
    block_vector_1.push_back(prev_block);
    block_vector_2.push_back(prev_block);

    for (uint64_t viewid=1; viewid<=19; viewid++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block, 0, viewid);
        block_vector_1.push_back(prev_block);
        block_vector_2.push_back(prev_block);
    }

/*
                           /--block(h=20,view=20)
                          /
    block(h=19,view=19)--
                          \
                           \--block(h=20,view=21)---block(h=21,view=22)---block(h=22,view=23)
*/

    {
        base::xvblock_t* blk = test_blocktuil::create_next_emptyblock(block_vector_1[19], 0, 20);
        block_vector_1.push_back(blk);
    }

    for (uint64_t viewid=21; viewid<=41; viewid++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block, 0, viewid);
        block_vector_2.push_back(prev_block);
    }

    // set local block  1-20
    {
        for (uint64_t h = 1; h<=20; h++) {
            base::xvblock_t* blk = block_vector_1[h];
            xblock_ptr_t block = copy_block(blk);
            sync_store.store_block(block.get());
        }
    }

    base::xauto_ptr<base::xvblock_t> auto_successor_block = block_vector_2[30];
    xblock_ptr_t successor_block = autoptr_to_blockptr(auto_successor_block);

    top::common::xnode_address_t network_self;
    top::common::xnode_address_t target_address;
    mbus::xevent_ptr_t behind_event = std::make_shared<mbus::xevent_behind_block_t>(successor_block, enum_behind_source_consensus, "", network_self, target_address);
    account->on_behind_event(behind_event);

    // sync
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

        xsync_message_get_blocks_ptr_t ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);

        const std::string &owner = ptr->owner;
        uint64_t start_height = ptr->start_height;
        uint32_t count = ptr->count;
        ASSERT_EQ(start_height, 21);
        ASSERT_EQ(count, 9);

        std::vector<xblock_ptr_t> vector_blocks;
        for (uint64_t h = start_height; h<=(start_height+count); h++) {
            base::xvblock_t* blk = block_vector_2[h];
            xblock_ptr_t block = copy_block(blk);
            vector_blocks.push_back(block);
        }

        mbus::xevent_ptr_t response_event = std::make_shared<mbus::xevent_sync_response_blocks_t>(vector_blocks, network_self, target_address);
        account->on_response_event(response_event);
    }

    // head fork and sync
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

        xsync_message_get_blocks_ptr_t ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);

        const std::string &owner = ptr->owner;
        uint64_t start_height = ptr->start_height;
        uint32_t count = ptr->count;
        ASSERT_EQ(start_height, 20);
        ASSERT_EQ(count, 10);

        std::vector<xblock_ptr_t> vector_blocks;
        for (uint64_t h = start_height; h<=(start_height+count); h++) {
            base::xvblock_t* blk = block_vector_2[h];
            xblock_ptr_t block = copy_block(blk);
            vector_blocks.push_back(block);
        }

        mbus::xevent_ptr_t response_event = std::make_shared<mbus::xevent_sync_response_blocks_t>(vector_blocks, network_self, target_address);
        account->on_response_event(response_event);
    }

    base::xauto_ptr<base::xvblock_t> cur_block = sync_store.get_current_block(address);
    ASSERT_EQ(cur_block->get_height(), 30);

}

TEST(xsync_account, lockedqc_fork) {

    std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);

    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore = nullptr;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store, ""));

    xsync_store_t sync_store("", make_observer(blockstore));
    xmessage_bus_t mbus;
    xmock_vhost_sync_t vhost;
    xsync_sender_t sync_sender("", make_observer(&vhost), nullptr);
    xsync_mock_ratelimit_t ratelimit;

    xchain_info_t chain_info;
    chain_info.address = address;

    top::mock::xmock_auth_t auth{1};

    xaccount_face_ptr_t account = std::make_shared<xaccount_general_t>("", &sync_store, make_observer(&mbus), make_observer(&auth), &sync_sender, &ratelimit, chain_info);

    std::vector<base::xvblock_t*> block_vector_1;
    std::vector<base::xvblock_t*> block_vector_2;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

    base::xvblock_t* prev_block = genesis_block;
    block_vector_1.push_back(prev_block);
    block_vector_2.push_back(prev_block);

    for (uint64_t viewid=1; viewid<=19; viewid++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block, 0, viewid);
        block_vector_1.push_back(prev_block);
        block_vector_2.push_back(prev_block);
    }

/*
                           /--block(h=20,view=20)---block(h=21,view=21)
                          /
    block(h=19,view=19)--
                          \
                           \--block(h=20,view=22)---block(h=21,view=23)---block(h=22,view=24)
*/
    {
        base::xvblock_t* blk = test_blocktuil::create_next_emptyblock(block_vector_1[19], 0, 20);
        block_vector_1.push_back(blk);

        blk = test_blocktuil::create_next_emptyblock(block_vector_1[20], 0, 21);
        block_vector_1.push_back(blk);
    }

    for (uint64_t viewid=22; viewid<=41; viewid++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block, 0, viewid);
        block_vector_2.push_back(prev_block);
    }

    // set local block  1-21
    {
        for (uint64_t h = 1; h<=block_vector_1.size()-1; h++) {
            base::xvblock_t* blk = block_vector_1[h];
            xblock_ptr_t block = copy_block(blk);
            sync_store.store_block(block.get());
        }
    }

    base::xauto_ptr<base::xvblock_t> auto_successor_block = block_vector_2[30];
    xblock_ptr_t successor_block = autoptr_to_blockptr(auto_successor_block);

    top::common::xnode_address_t network_self;
    top::common::xnode_address_t target_address;
    mbus::xevent_ptr_t behind_event = std::make_shared<mbus::xevent_behind_block_t>(successor_block, enum_behind_source_consensus, "", network_self, target_address);
    account->on_behind_event(behind_event);

    // sync
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

        xsync_message_get_blocks_ptr_t ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);

        const std::string &owner = ptr->owner;
        uint64_t start_height = ptr->start_height;
        uint32_t count = ptr->count;
        ASSERT_EQ(start_height, 22);
        ASSERT_EQ(count, 8);

        std::vector<xblock_ptr_t> vector_blocks;
        for (uint64_t h = start_height; h<=(start_height+count); h++) {
            base::xvblock_t* blk = block_vector_2[h];
            xblock_ptr_t block = copy_block(blk);
            vector_blocks.push_back(block);
        }

        mbus::xevent_ptr_t response_event = std::make_shared<mbus::xevent_sync_response_blocks_t>(vector_blocks, network_self, target_address);
        account->on_response_event(response_event);
    }

    // head fork and sync
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

        xsync_message_get_blocks_ptr_t ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);

        const std::string &owner = ptr->owner;
        uint64_t start_height = ptr->start_height;
        uint32_t count = ptr->count;
        ASSERT_EQ(start_height, 21);
        ASSERT_EQ(count, 9);

        std::vector<xblock_ptr_t> vector_blocks;
        for (uint64_t h = start_height; h<=(start_height+count); h++) {
            base::xvblock_t* blk = block_vector_2[h];
            xblock_ptr_t block = copy_block(blk);
            vector_blocks.push_back(block);
        }

        mbus::xevent_ptr_t response_event = std::make_shared<mbus::xevent_sync_response_blocks_t>(vector_blocks, network_self, target_address);
        account->on_response_event(response_event);
    }

    // head fork and sync
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

        xsync_message_get_blocks_ptr_t ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);

        const std::string &owner = ptr->owner;
        uint64_t start_height = ptr->start_height;
        uint32_t count = ptr->count;
        ASSERT_EQ(start_height, 20);
        ASSERT_EQ(count, 10);

        std::vector<xblock_ptr_t> vector_blocks;
        for (uint64_t h = start_height; h<=(start_height+count); h++) {
            base::xvblock_t* blk = block_vector_2[h];
            xblock_ptr_t block = copy_block(blk);
            vector_blocks.push_back(block);
        }

        mbus::xevent_ptr_t response_event = std::make_shared<mbus::xevent_sync_response_blocks_t>(vector_blocks, network_self, target_address);
        account->on_response_event(response_event);
    }

    base::xauto_ptr<base::xvblock_t> cur_block = sync_store.get_current_block(address);
    ASSERT_EQ(cur_block->get_height(), 30);

}
