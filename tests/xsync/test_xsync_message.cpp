#include <gtest/gtest.h>
#include <map>
#include <list>
#include <memory>
#include "xutility/xhash.h"

#include "xmbus/xevent.h"
#include "xsync/xsync_message.h"
#include "xsync/xgossip_message.h"
// #include "xblockstore/test/xblockstore_face_mock.h"
#include "xdata/xblocktool.h"
#include "xstore/xstore.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xblockstore/xblockstore_face.h"
#include "xsync/xsync_util.h"
#include "../xblockstore_test/test_blockmock.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xdata/xnative_contract_address.h"

using namespace top;
using namespace top::data;
using namespace top::base;
using namespace top::mbus;
using namespace top::sync;


TEST(xsync_message, header) {
    base::xstream_t stream(base::xcontext_t::instance());

    {
        auto header = make_object_ptr<xsync_message_header_t>((uint64_t)100);
        header->serialize_to(stream);
    }

    {
        xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);
        ASSERT_EQ(header->random, 100);
    }
}

TEST(xsync_message, get_blocks) {
    base::xstream_t stream(base::xcontext_t::instance());

    std::string address = "abc";
    uint64_t start_height = 1;
    uint32_t count = 2;

    {
        auto q = make_object_ptr<xsync_message_get_blocks_t>(
                address, start_height, count);
        q->serialize_to(stream);
    }

    {
        auto ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);
        ASSERT_EQ(ptr->owner, address);
        ASSERT_EQ(ptr->start_height, 1);
        ASSERT_EQ(ptr->count, 2);
    }
}

TEST(xsync_message, blocks) {
    base::xstream_t stream(base::xcontext_t::instance());

    std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);
    std::vector<xblock_ptr_t> vector_blocks;

    std::vector<base::xvblock_t*> block_vector;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

    base::xvblock_t* prev_block = genesis_block;
    for (uint64_t i=1; i<=2; i++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->add_ref();

        base::xauto_ptr<base::xvblock_t> autoptr = prev_block;

        xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
        vector_blocks.push_back(block_ptr);
    }

    {
        auto ptr = make_object_ptr<xsync_message_blocks_t>(address, vector_blocks);
        ptr->serialize_to(stream);
    }

    {
        auto ptr = make_object_ptr<xsync_message_blocks_t>();
        ptr->serialize_from(stream);
        ASSERT_EQ(ptr->owner, address);
        ASSERT_EQ(ptr->blocks.size(), 2);
        ASSERT_EQ(ptr->blocks[0]->get_height(), 1);
        ASSERT_EQ(ptr->blocks[1]->get_height(), 2);
    }
}

static xcons_transaction_ptr_t create_cons_transfer_tx(const std::string & from, const std::string & to, uint64_t amount = 100) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(from, to);
    tx->set_digest();
    tx->set_tx_subtype(enum_transaction_subtype_send);

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    return cons_tx;
}

TEST(xsync_message, push_newblock) {
    base::xstream_t stream(base::xcontext_t::instance());

    {
        //xblock_ptr_t blk = datamock.create_unit(owner, prop_list, 0);

        std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
        std::string to_account = xblocktool_t::make_address_user_account("11111111111111122222");
        base::xvblock_t* account1_genesis_block = test_blocktuil::create_genesis_empty_unit(account1);
        xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account);
        xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, to_account);
        xlightunit_block_para_t para1;
        para1.set_one_input_tx(account1_tx1);
        para1.set_one_input_tx(account1_tx2);
        xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

        base::xauto_ptr<base::xvblock_t> autoptr = lightunit1;
        xblock_ptr_t blk = autoptr_to_blockptr(autoptr);

        auto req = make_object_ptr<xsync_message_push_newblock_t>(blk);
        req->serialize_to(stream);
    }

    {
        auto ptr = make_object_ptr<xsync_message_push_newblock_t>();
        ptr->serialize_from(stream);

        xblock_ptr_t block = ptr->block;

        ASSERT_NE(block, nullptr);
        ASSERT_EQ(block->get_height(), 1);
    }
}

TEST(xsync_message, old_newblockhash) {
    base::xstream_t stream(base::xcontext_t::instance());

    std::string owner = data::xblocktool_t::make_address_user_account("11111111111111111111");
    std::string hash = "123";

    {
        auto req = make_object_ptr<xsync_message_v1_newblockhash_t>(owner, 1, 2);
            req->serialize_to(stream);
    }

    {
        auto ptr = make_object_ptr<xsync_message_v1_newblockhash_t>();
        ptr->serialize_from(stream);

        ASSERT_EQ(ptr->address, owner);
        ASSERT_EQ(ptr->height, 1);
        ASSERT_EQ(ptr->view_id, 2);
    }
}

TEST(xsync_message, general_newblockhash) {
    base::xstream_t stream(base::xcontext_t::instance());

    std::string owner = data::xblocktool_t::make_address_user_account("11111111111111111111");
    std::string hash = "123";

    {
        auto req = make_object_ptr<xsync_message_general_newblockhash_t>(owner, 1, 2, hash);
            req->serialize_to(stream);
    }

    {
        auto ptr = make_object_ptr<xsync_message_general_newblockhash_t>();
        ptr->serialize_from(stream);

        ASSERT_EQ(ptr->address, owner);
        ASSERT_EQ(ptr->height, 1);
        ASSERT_EQ(ptr->view_id, 2);
        ASSERT_EQ(ptr->hash, hash);
    }
}

TEST(xsync_message, gossip) {
    base::xstream_t stream(base::xcontext_t::instance());

    std::string address = xdatautil::serialize_owner_str(common::rec_table_base_address.to_string(), 0);

    {
        std::vector<xgossip_chain_info_ptr_t> info_list;
        xgossip_chain_info_ptr_t info = std::make_shared<xgossip_chain_info_t>();
        info->owner = address;
        info->max_height = 100;
        info->view_id = 101;
        info_list.push_back(info);

        xbyte_buffer_t bloom_data(32, 0);

        auto req = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
        req->serialize_to(stream);
    }

    {
        auto ptr = make_object_ptr<xsync_message_gossip_t>();
        ptr->serialize_from(stream);

        std::vector<xgossip_chain_info_ptr_t> &info_list = ptr->info_list;
        xbyte_buffer_t bloom_data;
        ASSERT_EQ(info_list.size(), 1);
        ASSERT_EQ(info_list[0]->owner, address);
        ASSERT_EQ(info_list[0]->max_height, 100);
        ASSERT_EQ(info_list[0]->view_id, 101);
    }
}

TEST(xsync_message, header_body) {
    base::xstream_t stream(base::xcontext_t::instance());
    std::string address = "abc";
    uint64_t start_height = 1;
    uint32_t count = 2;

    {
        auto header = make_object_ptr<xsync_message_header_t>((uint64_t)100);
        header->serialize_to(stream);
        auto q = make_object_ptr<xsync_message_get_blocks_t>(
                address, start_height, count);
        q->serialize_to(stream);
    }

    {
        xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream);
        ASSERT_EQ(header->random, 100);
        auto ptr = make_object_ptr<xsync_message_get_blocks_t>();
        ptr->serialize_from(stream);
        ASSERT_EQ(ptr->owner, address);
        ASSERT_EQ(ptr->start_height, 1);
        ASSERT_EQ(ptr->count, 2);
    }
}


TEST(xsync_message, get_on_demand_blocks) {

    std::string address = "abc";
    uint64_t start_height = 1;
    uint32_t count = 2;

    {
        base::xstream_t stream(base::xcontext_t::instance());

        auto q = make_object_ptr<xsync_message_get_on_demand_blocks_t>(
                address, start_height, count, true);
        q->serialize_to(stream);

        auto ptr = make_object_ptr<xsync_message_get_on_demand_blocks_t>();
        ptr->serialize_from(stream);
        ASSERT_EQ(ptr->address, address);
        ASSERT_EQ(ptr->start_height, 1);
        ASSERT_EQ(ptr->count, 2);
        ASSERT_EQ(ptr->is_consensus, true);
    }

    {
        base::xstream_t stream(base::xcontext_t::instance());

        auto q = make_object_ptr<xsync_message_get_on_demand_blocks_t>(
                address, start_height, count, false);
        q->serialize_to(stream);

        auto ptr = make_object_ptr<xsync_message_get_on_demand_blocks_t>();
        ptr->serialize_from(stream);
        ASSERT_EQ(ptr->address, address);
        ASSERT_EQ(ptr->start_height, 1);
        ASSERT_EQ(ptr->count, 2);
        ASSERT_EQ(ptr->is_consensus, false);
    }
}

TEST(xsync_message, on_demand_blocks) {
    base::xstream_t stream(base::xcontext_t::instance());

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    xobject_ptr_t<store::xstore_face_t> store;
    store.attach(creator.get_xstore());
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(creator.get_blockstore());
    test_blockmock_t blockmock(store.get());

    std::string account_address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::vector<xblock_ptr_t> blocks;
    {
        std::string property("election_list");
        base::xvblock_t *prev_unit_block = blockmock.create_property_block(nullptr, account_address, property);
        for (uint32_t i = 0; i < 1; i++) {
            std::string value(std::to_string(i));
            base::xvblock_t *curr_unit_block = blockmock.create_property_block(prev_unit_block, account_address, property, value);
            curr_unit_block->add_ref();

            base::xauto_ptr<base::xvblock_t> autoptr = curr_unit_block;
            xblock_ptr_t blk = autoptr_to_blockptr(autoptr);
            blocks.push_back(blk);
            blocks.push_back(blk);

            prev_unit_block = curr_unit_block;
        }

        auto req = make_object_ptr<xsync_message_general_blocks_t>(blocks);
        req->serialize_to(stream);
    }

    {
        auto ptr = make_object_ptr<xsync_message_general_blocks_t>();
        ptr->serialize_from(stream);
        ASSERT_EQ(ptr->blocks.size(), 2);
        ASSERT_EQ(ptr->blocks[0]->get_height(), 1);
        ASSERT_EQ(ptr->blocks[1]->get_height(), 1);
    }
}

TEST(xsync_message, chain_state_info) {

    base::xstream_t stream(base::xcontext_t::instance());
    {
        std::vector<xchain_state_info_t> info_list;
        for (uint32_t i=0; i<5; i++) {
            xchain_state_info_t info;
            info.address = std::to_string(i);
            info.start_height = 0;
            info.end_height = (uint64_t)i;
            info_list.push_back(info);
        }

        auto req = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
        req->serialize_to(stream);
    }

    {
        auto ptr = make_object_ptr<xsync_message_chain_state_info_t>();
        ptr->serialize_from(stream);

        std::vector<xchain_state_info_t> &info_list = ptr->info_list;
        ASSERT_EQ(info_list.size(), 5);
        for (uint32_t i=0; i<5; i++) {
            ASSERT_EQ(info_list[i].address, std::to_string(i));
            ASSERT_EQ(info_list[i].start_height, 0);
            ASSERT_EQ(info_list[i].end_height, (uint64_t)i);
        }
    }
}

// TODO add exception test
