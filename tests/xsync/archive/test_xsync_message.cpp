#include <gtest/gtest.h>

#include "xsync/xsync_message.h"
#include "common_func.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_session_manager.h"

using namespace top;
using namespace top::sync;

TEST(test_xsync_message, test_xsync_msg_t) {
  
    sync::xsync_msg_t msg;
    base::xstream_t stream(base::xcontext_t::instance());
    msg.serialize_to(stream);

    auto ptr = make_object_ptr<sync::xsync_msg_t>();
    ptr->serialize_from(stream);
    ASSERT_TRUE(msg.get_sessionID()== ptr->get_sessionID());
}

TEST(test_xsync_message, test_xsync_msg_block_t) {
  
    std::string address{"T600044dce5c8961e283786cb31ad7fc072347227d7ea2"};
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

TEST(test_xsync_message, test_xsync_msg_manager_request_check)
{
    bool ret = true;
    std::string address { "T600044dce5c8961e283786cb31ad7fc072347227d7ea2" };
    sync::xsync_session_manager_t session_manager(10, 1000);

    for (int request_type = (int)enum_sync_block_request_push; request_type <= (int)enum_sync_block_request_max; request_type++) {
       for (int request_param = (int)enum_sync_block_by_none; request_param <= (int)enum_sync_block_by_max; request_param++) {
             uint32_t request_option = SYNC_MSG_OPTION_SET(request_type, request_param, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
             auto body = make_object_ptr<xsync_msg_block_request_t>(address, request_option, 1, 10, "");
             ret = session_manager.sync_block_request_valid_check(body);
             if (request_type == enum_sync_block_request_max || request_param == enum_sync_block_by_max ||
                ((request_param == enum_sync_block_by_hash || request_param == enum_sync_block_by_txhash) && body->get_requeset_param_str().empty())) {
                 ASSERT_FALSE(ret);
             } else if(request_type != enum_sync_block_request_push && (request_param == enum_sync_block_by_none || request_param == enum_sync_block_by_max)){
                 ASSERT_FALSE(ret);
             }
             else {
                 ASSERT_TRUE(ret);
             }
       }
    }
}

TEST(test_xsync_message, test_xsync_msg_manager_count_limit)
{
    bool ret = true;
    std::string address { "T600044dce5c8961e283786cb31ad7fc072347227d7ea2" };
    sync::xsync_session_manager_t session_manager(10, 1000);

    for (int i = 0; i < 10; i++) {
        uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_demand, enum_sync_block_by_height, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
        auto body = make_object_ptr<xsync_msg_block_request_t>(address, request_option, 1, 10, "");
        ret = session_manager.sync_block_request_insert(body);
        ASSERT_TRUE(ret);
    }

    uint32_t request_option_11 = SYNC_MSG_OPTION_SET(enum_sync_block_request_demand, enum_sync_block_by_height, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    auto body_11 = make_object_ptr<xsync_msg_block_request_t>(address, request_option_11, 1, 10, "");
    ret = session_manager.sync_block_request_insert(body_11);
    ASSERT_FALSE(ret);
}

TEST(test_xsync_message, test_xsync_msg_manager_same_hash)
{
    bool ret = true;
    std::string address { "T600044dce5c8961e283786cb31ad7fc072347227d7ea2" };
    sync::xsync_session_manager_t session_manager(10, 1000);

    uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_demand, enum_sync_block_by_height, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    auto body = make_object_ptr<xsync_msg_block_request_t>(address, request_option, 1, 10, "");
    ret = session_manager.sync_block_request_insert(body);
    ASSERT_TRUE(ret);
    ret = session_manager.sync_block_request_insert(body);
    ASSERT_FALSE(ret);
}

