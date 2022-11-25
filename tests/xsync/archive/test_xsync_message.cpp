#include <gtest/gtest.h>

#include "xsync/xsync_message.h"
#include "common_func.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_store_shadow.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "xblockstore/src/xvblockhub.h"

using namespace top;
using namespace top::sync;

using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;


TEST(test_xsync_message, test_xsync_msg_t)
{

    sync::xsync_msg_t msg;
    base::xstream_t stream(base::xcontext_t::instance());
    msg.serialize_to(stream);

    auto ptr = make_object_ptr<sync::xsync_msg_t>();
    ptr->serialize_from(stream);
    ASSERT_TRUE(msg.get_sessionID() == ptr->get_sessionID());
}

TEST(test_xsync_message, test_xsync_msg_block_t)
{

    std::string address { "T600044dce5c8961e283786cb31ad7fc072347227d7ea2" };
    uint32_t _request_option = RandomInt32();
    sync::xsync_msg_block_t msg_block(address, _request_option);

    base::xstream_t stream(base::xcontext_t::instance());
    msg_block.serialize_to(stream);
    auto ptr = make_object_ptr<sync::xsync_msg_block_t>();
    ptr->serialize_from(stream);

    ASSERT_TRUE(msg_block.get_sessionID()== ptr->get_sessionID());
    ASSERT_TRUE(msg_block.get_address()== ptr->get_address());
    ASSERT_TRUE(msg_block.get_option()== ptr->get_option());
}

TEST(test_xsync_message, test_xsync_msg_block_request_t) {
  
    std::string address{"T600044dce5c8961e283786cb31ad7fc072347227d7ea2"};
    uint32_t _request_option =  SYNC_MSG_OPTION_SET(enum_sync_block_request_ontime, enum_sync_block_by_height, enum_sync_data_all, enum_sync_block_object_xvblock, 2);;
    uint64_t _height = RandomUint64();
    uint32_t _count = RandomInt32();
    std::string _param = "abcdefgh";
    sync::xsync_msg_block_request_t msg_blok_request(address, _request_option, _height, _count, _param);

    base::xstream_t stream(base::xcontext_t::instance());
    msg_blok_request.serialize_to(stream);
    auto ptr = make_object_ptr<sync::xsync_msg_block_request_t>();
    ptr->serialize_from(stream);

    ASSERT_TRUE(msg_blok_request.get_sessionID()== ptr->get_sessionID());
    ASSERT_TRUE(msg_blok_request.get_address()== ptr->get_address());
    ASSERT_TRUE(msg_blok_request.get_option()== ptr->get_option());
    ASSERT_TRUE(msg_blok_request.get_request_start_height()== ptr->get_request_start_height());
    ASSERT_TRUE(msg_blok_request.get_requeset_param_str()== ptr->get_requeset_param_str());
    ASSERT_TRUE(msg_blok_request.get_count()== ptr->get_count());

    ASSERT_TRUE(sync::enum_sync_block_request_ontime == ptr->get_request_type());
    ASSERT_TRUE(sync::enum_sync_block_by_height == ptr->get_requeset_param_type());
    ASSERT_TRUE(sync::enum_sync_data_all == ptr->get_data_type());
    ASSERT_TRUE(sync::enum_sync_block_object_xvblock == ptr->get_block_object_type());
    ASSERT_TRUE(2 == ptr->get_extend_bits());

    ASSERT_FALSE(msg_blok_request.get_time() == ptr->get_time());
}

TEST(test_xsync_message, test_xsync_msg_block_response_t) {
  
    std::string address{"T600044dce5c8961e283786cb31ad7fc072347227d7ea2"};
    uint32_t sessionID = RandomUint64();
    uint64_t _height = RandomUint64();
    uint32_t _count = RandomInt32();
    uint32_t _extend_option = 1;
    std::string _extend_data = "abcdefgh";

    uint32_t _request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_demand, enum_sync_block_by_height, enum_sync_data_all, enum_sync_block_object_xvblock, 1);
    std::vector<data::xblock_ptr_t> block_vec = create_emptyblock_with_address(address, 10);
    std::vector<std::string> block_data_vec = convert_blocks_to_stream(enum_sync_data_all, block_vec);

    sync::xsync_msg_block_response_t msg_blok_response(sessionID, address, _request_option, block_data_vec, _extend_option, _extend_data);

    base::xstream_t stream(base::xcontext_t::instance());
    msg_blok_response.serialize_to(stream);
    auto ptr = make_object_ptr<sync::xsync_msg_block_response_t>();
    ptr->serialize_from(stream);

    ASSERT_TRUE(msg_blok_response.get_sessionID() == ptr->get_sessionID());
    ASSERT_TRUE(msg_blok_response.get_address()== ptr->get_address());
    ASSERT_TRUE(msg_blok_response.get_option()== ptr->get_option());
    ASSERT_TRUE(msg_blok_response.get_response_option()== ptr->get_response_option());
    ASSERT_TRUE(msg_blok_response.get_extend_data()== ptr->get_extend_data());
    ASSERT_TRUE(msg_blok_response.get_blocks_data().size()== ptr->get_blocks_data().size());

    
    ASSERT_TRUE(sync::enum_sync_block_request_demand == ptr->get_request_type());
    ASSERT_TRUE(sync::enum_sync_block_by_height == ptr->get_requeset_param_type());
    ASSERT_TRUE(sync::enum_sync_data_all == ptr->get_data_type());
    ASSERT_TRUE(sync::enum_sync_block_object_xvblock == ptr->get_block_object_type());
    ASSERT_TRUE(1 == ptr->get_extend_bits());

    std::vector<data::xblock_ptr_t> block_vec_out = ptr->get_all_xblock_ptr();
    for(size_t i=0; i< block_vec_out.size(); i++) {
       ASSERT_TRUE( block_vec_out[i]->get_block_hash() == block_vec[i]->get_block_hash());
    }
}


TEST(test_xsync_message, test_xsync_msg_block_push_t) {
  
    std::string address{"T600044dce5c8961e283786cb31ad7fc072347227d7ea2"};
    uint32_t sessionID = RandomUint64();
    uint64_t _height = RandomUint64();
    uint32_t _count = RandomInt32();
    uint32_t _extend_option = 1;
    std::string _extend_data = "abcdefgh";

    uint32_t _request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_push, 0, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    std::vector<data::xblock_ptr_t> block_vec = create_emptyblock_with_address(address, 10);
    std::vector<std::string> block_data_vec = convert_blocks_to_stream(enum_sync_data_all, block_vec);

    sync::xsync_msg_block_push_t msg_blok_push(address, _request_option, block_data_vec, _extend_option, _extend_data);

    base::xstream_t stream(base::xcontext_t::instance());
    msg_blok_push.serialize_to(stream);
    auto ptr = make_object_ptr<sync::xsync_msg_block_push_t>();
    ptr->serialize_from(stream);

    ASSERT_TRUE(msg_blok_push.get_sessionID() == ptr->get_sessionID());
    ASSERT_TRUE(msg_blok_push.get_address()== ptr->get_address());
    ASSERT_TRUE(msg_blok_push.get_option()== ptr->get_option());
    ASSERT_TRUE(msg_blok_push.get_push_option()== ptr->get_push_option());
    ASSERT_TRUE(1== ptr->get_push_option());
    ASSERT_TRUE(msg_blok_push.get_extend_data()== ptr->get_extend_data());
    ASSERT_TRUE(msg_blok_push.get_blocks_data().size() == ptr->get_blocks_data().size());

    std::vector<data::xblock_ptr_t> block_vec_out = ptr->get_all_xblock_ptr();
    for(size_t i=0; i< block_vec_out.size(); i++) {
       ASSERT_TRUE( block_vec_out[i]->get_block_hash() == block_vec[i]->get_block_hash());
    }

}

TEST(test_xsync_message, test_xsync_connect_test)
{
    mock::xvchain_creator creator(true);
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    //create block
    uint64_t count = 100;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t>& tables = mocktable.get_history_tables();
    mocktable.store_genesis_units(blockstore);
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    xvaccount_t account(address);


    //test connect,0-20
    {
        uint64_t connect_height = 20;
        for (uint64_t i = 1; i < connect_height; i++) {
            auto curr_block = tables[i].get();
            ASSERT_TRUE(blockstore->store_block(account, curr_block));
        }
        
        std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
        xsync_store_t sync_store("", make_observer(blockstore), store_shadow.get());

        std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, &sync_store, address);
        chain_spans_new->initialize();
        uint64_t cp_height = sync_store.get_latest_end_block_height(address, enum_chain_sync_policy_checkpoint);
        ASSERT_TRUE((connect_height-1) == cp_height);
        // ASSERT_TRUE( == cp_height);
        std::pair<uint64_t, uint64_t> height_interval(cp_height, cp_height);
        auto pair_result = chain_spans_new->get_continuous_unused_interval(height_interval);
        ASSERT_TRUE(pair_result.first == pair_result.second);
        ASSERT_TRUE(pair_result.first == cp_height);
    }

     //test disconnect,[0-20], [30-40],
    {
        uint64_t disconnect_height = 30, max_height = 40;
        for (uint64_t i = disconnect_height; i < max_height; i++) {
            auto curr_block = tables[i].get();
            ASSERT_TRUE(blockstore->store_block(account, curr_block));
        }
        
        std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
        xsync_store_t sync_store("", make_observer(blockstore), store_shadow.get());

        std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, &sync_store, address);
        chain_spans_new->initialize();
        //mock store event
        for (uint64_t i = disconnect_height; i < max_height; i++) {
           chain_spans_new->set(i);
        }

        uint64_t cp_height = sync_store.get_latest_end_block_height(address, enum_chain_sync_policy_checkpoint);
        std::pair<uint64_t, uint64_t> height_interval(cp_height, max_height);
        auto pair_result = chain_spans_new->get_continuous_unused_interval(height_interval);
        ASSERT_TRUE(pair_result.first == cp_height);
        ASSERT_TRUE(pair_result.second == (disconnect_height-1));
    }

}