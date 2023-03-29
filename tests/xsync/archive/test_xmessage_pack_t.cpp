#include <gtest/gtest.h>

#include "xsync/xsync_message.h"
#include "common_func.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_store_shadow.h"
#include "xsync/xchain_downloader.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "xblockstore/src/xvblockhub.h"
#include "xdata/xblocktool.h"
#include "xsync/xmessage_pack.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "test_message_class_codec.hpp"

#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>

using namespace top;
using namespace top::sync;

using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;

long get_time_nanosecond()
{
    struct timespec timestamp = {};
    if (0 == clock_gettime(CLOCK_REALTIME, &timestamp))
        return timestamp.tv_sec * 1000000000 + timestamp.tv_nsec;
    else
        return 0;
}

long get_time_microsecond()
{
    struct timeval timestamp = {};
    if (0 == gettimeofday(&timestamp, NULL))
        return timestamp.tv_sec * 1000000 + timestamp.tv_usec;
    else
        return 0;
}

long get_time_millisecond()
{
    struct timeb timestamp = {};

    if (0 == ftime(&timestamp))
        return timestamp.time * 1000 + timestamp.millitm;
    else
        return 0;
}

void test_stream_string_bytes(uint32_t bytes_count, uint32_t loop, uint64_t& total_size, uint64_t& pack_total_time, uint64_t& unpack_total_time)
{
    uint64_t start = 0, end = 0, pack_time = 0, unpack_time = 0;
    std::string address = "T800001753d40631a3ad31568c3141272cac45692888d1";

    int index = 0;
    do {
        base::xstream_t stream_1(base::xcontext_t::instance());
        do {
            start = get_time_microsecond();
            stream_1 << address;
            end = get_time_microsecond();
            pack_time += (end - start);
        } while (stream_1.size() < bytes_count);
        index++;
        total_size += stream_1.size();

        do {
            std::string address_dst;
            start = get_time_microsecond();
            stream_1 >> address_dst;
            end = get_time_microsecond();
            unpack_time += (end - start);
        } while (stream_1.size() > 0);

    } while (index < loop);
    pack_total_time += pack_time;
    unpack_total_time += unpack_time;
}

TEST(test_xmessage_pack_t, test_xsync_message_by_string_BENCH)
{
    uint64_t bytes_base = 1024;
    for (uint32_t loop = 0; loop < 5; loop++) {
        uint64_t total_size = 0, pack_total_time = 0, unpack_total_time = 0;
        uint64_t bytes_set = 1024 * pow(10, (int)loop);
        test_stream_string_bytes(bytes_set, 10, total_size, pack_total_time, unpack_total_time);
        double pack_time_avg = (double)pack_total_time / 10;
        double unpack_time_avg = (double)unpack_total_time / 10;
        uint64_t size_avg = total_size / 10;
        std::cout << " test set_bytes:" << bytes_set << " size_avg:" << size_avg << " pack_time_avg:"
                  << std::fixed << std::setprecision(0) << pack_time_avg << "us unpack_time_avg:" << std::fixed << std::setprecision(0) << unpack_time_avg << "us " << std::endl;
    }
}

void xsync_message_by_xstrem(uint32_t count, int64_t& pack_time, int64_t& unpack_time, uint64_t& total_size)
{
    mock::xvchain_creator* m_creator = new mock::xvchain_creator(true);
    m_creator->create_blockstore_with_xstore();
    base::xvblockstore_t* m_blockstore = m_creator->get_blockstore();
    mock::xdatamock_table m_mocktable;
    m_mocktable.genrate_table_chain(count, m_blockstore);

    const std::vector<xblock_ptr_t>& tables = m_mocktable.get_history_tables();
    std::string address = m_mocktable.get_account();

    uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_ontime, enum_sync_block_by_height_lists, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    auto body = make_object_ptr<xsync_msg_block_request_t>(address, request_option, 0, tables.size(), "");
    auto block_str_vec = convert_blocks_to_stream(body->get_data_type(), tables);
    auto response_body = make_object_ptr<xsync_msg_block_response_t>(body->get_sessionID(), body->get_address(), body->get_option(), block_str_vec, 0, "");

    auto start = get_time_microsecond();
    base::xstream_t stream_1(base::xcontext_t::instance());
    response_body->serialize_to(stream_1);
    auto end = get_time_microsecond();
    pack_time += (end - start);
    total_size += stream_1.size();

    start = get_time_microsecond();
    auto response_ptr = make_object_ptr<xsync_msg_block_response_t>();
    response_ptr->serialize_from(stream_1);
    end = get_time_microsecond();
    unpack_time += (end - start);

    auto blocks_vec = response_ptr->get_all_xblock_ptr();
    ASSERT_EQ(response_body->get_request_type(), response_ptr->get_request_type());
    ASSERT_EQ(response_body->get_requeset_param_type(), response_ptr->get_requeset_param_type());
    ASSERT_EQ(blocks_vec.size(), tables.size());
    int32_t index = 0;
    for (auto block_ptr : tables) {
        ASSERT_EQ(block_ptr->get_height(), blocks_vec[index]->get_height());
        ASSERT_EQ(block_ptr->get_block_hash(), blocks_vec[index]->get_block_hash());
        index++;
    }
}

TEST(test_xmessage_pack_t, test_xsync_message_by_xstrem_BENCH)
{
    uint32_t test_count = 2, block_num_rate = 100;

    for (int block_num = 1; block_num <= block_num_rate; block_num *= 10) {
        for (int block_num_increase = 1; block_num_increase < 10; block_num_increase += 2) {
            int64_t pack_time = 0, unpack_time = 0;
            uint64_t total_size = 0;
            uint64_t block_num_set_count = block_num * block_num_increase * 10;
            if (block_num_set_count > 4000) {
                // xsync_message_by_xstrem not support 16M
                break;
            }
            for (int i = 0; i < test_count; i++) {
                xsync_message_by_xstrem(block_num_set_count, pack_time, unpack_time, total_size);
            }
            float pack_time_avg = (float)pack_time / test_count;
            float unpack_time_avg = (float)unpack_time / test_count;
            std::cout << std::left << std::fixed << std::setprecision(0) << "xstrem block:" << std::setw(6) << block_num_set_count
                      << " pack_time(us):" << std::setw(8) << pack_time << " pack_time_avg(us): " << std::setw(8) << pack_time_avg
                      << " unpack_time(us):" << std::setw(8) << unpack_time << " unpack_time_avg(us):" << std::setw(8) << unpack_time_avg
                      << " total size " << std::setw(10) << (total_size / test_count) << std::endl;
        }
    }
}

void test_stream_string_bytes_compress(uint32_t bytes_count, uint32_t loop, uint64_t& total_size, uint64_t& pack_total_time, uint64_t& unpack_total_time)
{
    uint64_t start = 0, end = 0, pack_time = 0, unpack_time = 0;
    std::string address = "T800001753d40631a3ad31568c3141272cac45692888d1";

    int index = 0;
    do {
        base::xstream_t stream_1(base::xcontext_t::instance());
        do {
            start = get_time_microsecond();
            stream_1.write_compact_var(address);
            end = get_time_microsecond();
            pack_time += (end - start);
        } while (stream_1.size() < bytes_count);
        index++;
        total_size += stream_1.size();

        do {
            std::string address_dst;
            start = get_time_microsecond();
            stream_1.read_compact_var(address_dst);
            end = get_time_microsecond();
            unpack_time += (end - start);
        } while (stream_1.size() > 0);

    } while (index < loop);
    pack_total_time += pack_time;
    unpack_total_time += unpack_time;
}

TEST(test_xmessage_pack_t, test_xsync_message_by_string_compress_BENCH)
{
    uint64_t bytes_base = 1024;
    for (uint32_t loop = 0; loop < 5; loop++) {
        uint64_t total_size = 0, pack_total_time = 0, unpack_total_time = 0;
        uint64_t bytes_set = 1024 * pow(10, (int)loop);
        test_stream_string_bytes_compress(bytes_set, 10, total_size, pack_total_time, unpack_total_time);
        double pack_time_avg = (double)pack_total_time / 10;
        double unpack_time_avg = (double)unpack_total_time / 10;
        uint64_t size_avg = total_size / 10;
        std::cout << " test compress set_bytes:" << bytes_set << " size_avg:" << size_avg << " pack_time_avg:"
                  << std::fixed << std::setprecision(0) << pack_time_avg << "us unpack_time_avg:" << std::fixed << std::setprecision(0) << unpack_time_avg << "us " << std::endl;
    }
}

void send_message_mock(const xobject_ptr_t<basic::xserialize_face_t> serializer,
    const common::xmessage_id_t msgid,
    vnetwork::xmessage_t& msg, bool require_compress, bool without_dataunit_serialize = false)
{
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    if(!without_dataunit_serialize) {
        serializer->serialize_to(stream);
    } else {
        serializer->do_write(stream);
    }

    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({ stream.data(), stream.data() + stream.size() }, msgid);
    xmessage_pack_t::pack_message(_msg, require_compress, msg, true);
}

TEST(test_xmessage_pack_t, test_xsync_message_by_stream_compress__BENCH)
{
    mock::xvchain_creator* m_creator = new mock::xvchain_creator(true);
    m_creator->create_blockstore_with_xstore();
    base::xvblockstore_t* m_blockstore = m_creator->get_blockstore();
    mock::xdatamock_table m_mocktable;
    m_mocktable.genrate_table_chain(10, m_blockstore);

    const std::vector<xblock_ptr_t>& tables = m_mocktable.get_history_tables();
    std::string address = m_mocktable.get_account();

    uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_ontime, enum_sync_block_by_height_lists, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    auto body = make_object_ptr<xsync_msg_block_request_t>(address, request_option, 0, tables.size(), "");

    auto block_str_vec = convert_blocks_to_stream(body->get_data_type(), tables);
    auto response_body = make_object_ptr<xsync_msg_block_response_t>(body->get_sessionID(), body->get_address(), body->get_option(),
        block_str_vec, 0, "");
    vnetwork::xmessage_t msg;
    send_message_mock(response_body, xmessage_id_sync_block_response, msg, true, true);

    xbyte_buffer_t msg_unpack;
    vnetwork::xmessage_t::message_type msg_type = msg.id();
    xmessage_pack_t::unpack_message(msg.payload(), msg_type, msg_unpack);

    base::xstream_t stream_unpack(base::xcontext_t::instance(), (uint8_t*)msg_unpack.data(), msg_unpack.size());
    xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
    header->serialize_from(stream_unpack);
    xsync_msg_block_response_t response_body_unpack;
    response_body_unpack.do_read_from(stream_unpack);

    auto blocks_vec = response_body_unpack.get_all_xblock_ptr();
    ASSERT_EQ(response_body->get_address(), response_body_unpack.get_address());
    ASSERT_EQ(response_body->get_request_type(), response_body_unpack.get_request_type());
    ASSERT_EQ(response_body->get_requeset_param_type(), response_body_unpack.get_requeset_param_type());
    ASSERT_EQ(blocks_vec.size(), tables.size());
    int32_t index = 0;
    for (auto block_ptr : tables) {
        ASSERT_EQ(block_ptr->get_height(), blocks_vec[index]->get_height());
        ASSERT_EQ(block_ptr->get_block_hash(), blocks_vec[index]->get_block_hash());
        index++;
    }
}

void message_test_by_xstrem_msgpack(uint32_t size_count, int64_t& pack_time, int64_t& unpack_time, uint64_t& total_size, int32_t pack_type)
{
    message_class_test class_test_pack(size_count);
    ASSERT_EQ(class_test_pack.get_max_size(), size_count);
    ASSERT_EQ(class_test_pack.get_string_a().length(), size_count);
    ASSERT_EQ(class_test_pack.get_string_a().length(), size_count);

    int64_t pack_time_start = 0, pack_time_end = 0, unpack_time_start = 0, unpack_time_end = 0;
    uint64_t _size = 0;

    message_class_test class_test_unpack(0);

    // xstream
    if (pack_type == 0) {

        pack_time_start = get_time_microsecond();
        base::xstream_t stream_pack(base::xcontext_t::instance());
        class_test_pack.do_write(stream_pack);
        pack_time_end = get_time_microsecond();
        _size = stream_pack.size();

        unpack_time_start = get_time_microsecond();
        base::xstream_t stream_unpack(base::xcontext_t::instance(), (uint8_t*)stream_pack.data(), stream_pack.size());
        class_test_unpack.do_read(stream_pack);
        unpack_time_end = get_time_microsecond();

    } else if (pack_type == 1) {
        // msgpack
        pack_time_start = get_time_microsecond();
        auto bytes = codec::msgpack_encode(class_test_pack);
        pack_time_end = get_time_microsecond();
        _size = bytes.size();
        unpack_time_start = get_time_microsecond();
        class_test_unpack = top::codec::msgpack_decode<message_class_test>(bytes);
        unpack_time_end = get_time_microsecond();
    } else if (pack_type == 2) {

        if (size_count > 4000)
            return; // pdu not support 16m
        pack_time_start = get_time_microsecond();
        base::xstream_t stream_pack(base::xcontext_t::instance());
        class_test_pack.do_write(stream_pack);
        vnetwork::xmessage_t _msg = vnetwork::xmessage_t({ stream_pack.data(), stream_pack.data() + stream_pack.size() }, xmessage_id_sync_block_response);
        std::cout << "before pdu compress msg  size:" << _msg.payload().size() << std::endl;
        vnetwork::xmessage_t msg;
        xmessage_pack_t::pack_message(_msg, ((int)_msg.payload().size()) >= 100, msg, true);
        std::cout << "after pdu compress msg  size:" << msg.payload().size() << std::endl;
        pack_time_end = get_time_microsecond();
        _size = msg.payload().size();

        unpack_time_start = get_time_microsecond();
        xbyte_buffer_t msg_unpack;
        vnetwork::xmessage_t::message_type msg_type = msg.id();
        xmessage_pack_t::unpack_message(msg.payload(), msg_type, msg_unpack);
        base::xstream_t stream_unpack(base::xcontext_t::instance(), (uint8_t*)msg_unpack.data(), msg_unpack.size());
        class_test_unpack.do_read(stream_unpack);
        unpack_time_end = get_time_microsecond();
    } else if (pack_type == 3) {

        pack_time_start = get_time_microsecond();
        base::xstream_t unpack_msg(base::xcontext_t::instance());
        class_test_pack.do_write(unpack_msg);
        std::cout << "before big pack compress msg  size:" << unpack_msg.size() << std::endl;
        base::xstream_t stream_dst(base::xcontext_t::instance());
        vnetwork::xmessage_t _msg = vnetwork::xmessage_t({ unpack_msg.data(), unpack_msg.data() + unpack_msg.size() }, xmessage_id_sync_block_response);
        int size = xmessage_pack_t::serialize_to_big_pack(_msg, stream_dst, true);
        uint32_t msgid_int = static_cast<std::uint32_t>(xmessage_id_sync_block_response);
        vnetwork::xmessage_t::message_type msg_type = static_cast<common::xmessage_id_t>(msgid_int + sync_big_pack_increase_index);
        vnetwork::xmessage_t msg = vnetwork::xmessage_t({ stream_dst.data(), stream_dst.data() + stream_dst.size() }, msg_type);
        pack_time_end = get_time_microsecond();
        _size = msg.payload().size();
        std::cout << "after big pack compress msg  size:" << stream_dst.size() << std::endl;

        unpack_time_start = get_time_microsecond();
        xbyte_buffer_t msg_unpack;
        xmessage_pack_t::unpack_message(msg.payload(), msg_type, msg_unpack);
        base::xstream_t stream_unpack(base::xcontext_t::instance(), (uint8_t*)msg_unpack.data(), msg_unpack.size());
        class_test_unpack.do_read(stream_unpack);
        unpack_time_end = get_time_microsecond();
    } else if (pack_type == 4) {
        /*pack_time_start = get_time_microsecond();
          base::xstream_t unpack_msg(base::xcontext_t::instance());
          class_test_pack.do_write(unpack_msg);
          std::cout << "before lz4 compress msg  size:" << unpack_msg.size() << std::endl;
          const int max_body_bound_size = xcompress_t::lz4_get_compressed_bound_size(unpack_msg.size())+8;
          uint8_t *pdu = (uint8_t*)malloc(max_body_bound_size);
           std::cout << "after lz4 compress msg  size:" << max_body_bound_size << std::endl;

          //compress data of local and write to target packet
          int pdu_body_size = xcompress_t::lz4_compress((const char*)unpack_msg.data(), (char*)pdu, unpack_msg.size(), max_body_bound_size);

          uint32_t msgid_int = static_cast<std::uint32_t>(xmessage_id_sync_block_response);
          vnetwork::xmessage_t::message_type msg_type = static_cast<common::xmessage_id_t>(msgid_int +sync_big_pack_increase_index);

          uint8_t *pdu_dst = (uint8_t*)malloc(unpack_msg.size());
          xcompress_t::lz4_decompress((char*)pdu, (char*)pdu_dst, max_body_bound_size, unpack_msg.size());

           base::xstream_t stream_unpack(base::xcontext_t::instance(), (uint8_t*)pdu_dst, unpack_msg.size());
          class_test_unpack.do_read(stream_unpack);
          unpack_time_end = get_time_microsecond();

          free(pdu);
          free(pdu_dst);
          return;*/
    } else {
        return;
    }
    pack_time += (pack_time_end - pack_time_start);
    unpack_time += (unpack_time_end - unpack_time_start);
    total_size += _size;

    ASSERT_EQ(class_test_pack.get_max_size(), class_test_unpack.get_max_size());
    ASSERT_EQ(class_test_pack.get_string_a().length(), class_test_unpack.get_string_a().length());
    ASSERT_EQ(class_test_pack.get_string_b().length(), class_test_unpack.get_string_b().length());
}

TEST(test_xmessage_pack_t, test_message_test_by_xstrem_msgpack_BENCH)
{
    uint32_t test_count = 1;
    // ：1K，10K，100K，1M，2M，4M，8M，16M，32M
    uint64_t test_cases[] = { 1, 10, 100, 1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000 };
    for (auto _case : test_cases) {
        for (int test_pack_type = 0; test_pack_type < 4; test_pack_type++) {
            if (test_pack_type > 1)
                continue;
            int64_t pack_time = 0, unpack_time = 0;
            uint64_t total_size = 0;
            uint64_t set_case_size = 1024 * _case;
            for (int test_num = 0; test_num < test_count; test_num++) {
                message_test_by_xstrem_msgpack(set_case_size, pack_time, unpack_time, total_size, test_pack_type);
            }
            float pack_time_avg = (float)pack_time / test_count;
            float unpack_time_avg = (float)unpack_time / test_count;

            if (test_pack_type == 0) {
                std::cout << "xstream";
            } else if (test_pack_type == 1) {
                std::cout << "mspack  ";
            } else if (test_pack_type == 2) {
                std::cout << "xstreamcompress ";
            } else if (test_pack_type == 3) {
                std::cout << "big pack compress ";
            } else if (test_pack_type == 4) {
                std::cout << "lz4 compress ";
            }

            std::cout << std::left << std::fixed << std::setprecision(0) << " test size:" << std::setw(10) << set_case_size
                      << " pack_time(us):" << std::setw(8) << pack_time << " pack_time_avg(us): " << std::setw(8) << pack_time_avg
                      << " unpack_time(us):" << std::setw(8) << unpack_time << " unpack_time_avg(us):" << std::setw(8) << unpack_time_avg
                      << " total size " << std::setw(10) << (total_size / test_count) << std::endl;
        }
    }
}

TEST(test_xmessage_pack_t, test_xmessage_pack_t_BENCH)
{
    xbyte_buffer_t _bs;
    for (uint32_t i = 0; i < 80000000; i++) {
        uint8_t t = i % 2;
        _bs.push_back(t);
    }
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t(_bs, xmessage_id_sync_block_response);

    std::cout << "test_xmessage_pack_t_BENCH before compress msg  size:" << _msg.payload().size() << std::endl;
    vnetwork::xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, true, msg, true);
    std::cout << "test_xmessage_pack_t_BENCH after compress msg  size:" << msg.payload().size() << std::endl;
}

void big_pack_serialize_unserialize_test(vnetwork::xmessage_t& unpack_msg, bool require_compress)
{
    base::xstream_t stream_dst(base::xcontext_t::instance());
    int32_t serialize_size = xmessage_pack_t::serialize_to_big_pack(unpack_msg, stream_dst, require_compress);
    ASSERT_TRUE(serialize_size > 0);
    xbyte_buffer_t packed_buffer = { stream_dst.data(), stream_dst.data() + stream_dst.size() };
    base::xstream_t unpack_stream_dst(base::xcontext_t::instance());
    int32_t unserialize_size = xmessage_pack_t::serialize_from_big_pack(packed_buffer, unpack_stream_dst);
    ASSERT_TRUE(unserialize_size > 0);
    xbyte_buffer_t unpack_buffer2 = { unpack_stream_dst.data(), unpack_stream_dst.data() + unpack_stream_dst.size() };
    ASSERT_EQ(unpack_buffer2, unpack_msg.payload());
    ASSERT_EQ(unpack_buffer2.size(), unpack_msg.payload().size());
    std::cout << "require_compress " << require_compress << " unpack_size " << unpack_buffer2.size() << " pack_size " << packed_buffer.size() << std::endl;
}

TEST(test_xmessage_pack_t, xmessage_pack_big_test)
{
    {
        bool require_compress = false;
        std::string payload = "123456789";
        vnetwork::xmessage_t unpack_msg = vnetwork::xmessage_t(top::to_bytes(payload), static_cast<common::xmessage_id_t>(100));
        big_pack_serialize_unserialize_test(unpack_msg, require_compress);
    }

    {
        bool require_compress = true;
        std::string payload = "123456789";
        vnetwork::xmessage_t unpack_msg = vnetwork::xmessage_t(top::to_bytes(payload), static_cast<common::xmessage_id_t>(100));
        big_pack_serialize_unserialize_test(unpack_msg, require_compress);
    }

    {
        bool require_compress = false;
        xbyte_buffer_t msg_payload;
        for (uint32_t i = 0; i < 100000; i++) {
            msg_payload.push_back((uint8_t)i);
        }
        vnetwork::xmessage_t unpack_msg = vnetwork::xmessage_t(msg_payload, static_cast<common::xmessage_id_t>(100));
        big_pack_serialize_unserialize_test(unpack_msg, require_compress);
    }

    {
        bool require_compress = true;
        xbyte_buffer_t msg_payload;
        for (uint32_t i = 0; i < 100000; i++) {
            msg_payload.push_back((uint8_t)i);
        }
        vnetwork::xmessage_t unpack_msg = vnetwork::xmessage_t(msg_payload, static_cast<common::xmessage_id_t>(100));
        big_pack_serialize_unserialize_test(unpack_msg, require_compress);
    }
}

TEST(test_xmessage_pack_t, xmessage_pack_big_pack_message_test)
{
    std::vector<xchain_state_info_t> rsp_info_list;
    for (int i = 0; i < 50; i++) {
        xchain_state_info_t chain_info;
        chain_info.address = "T800001753d40631a3ad31568c3141272cac45692888d1";
        chain_info.start_height = i;
        chain_info.end_height = i * i;
        rsp_info_list.push_back(chain_info);
    }

    for (int i = 0; i < 2; i++) {
        bool require_compress = false;
        if (i == 1) {
            require_compress = true;
        }

        auto body = make_object_ptr<xsync_message_chain_state_info_t>(rsp_info_list);
        vnetwork::xmessage_t msg;
        send_message_mock(body, xmessage_id_sync_broadcast_chain_state, msg, require_compress);
        xbyte_buffer_t msg_unpack;
        vnetwork::xmessage_t::message_type msg_type = msg.id();
        xmessage_pack_t::unpack_message(msg.payload(), msg_type, msg_unpack);

        base::xstream_t stream_unpack(base::xcontext_t::instance(), (uint8_t*)msg_unpack.data(), msg_unpack.size());
        xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
        header->serialize_from(stream_unpack);

        auto ptr = make_object_ptr<xsync_message_chain_state_info_t>();
        ptr->serialize_from(stream_unpack);
        std::vector<xchain_state_info_t>& info_list = ptr->info_list;
        ASSERT_EQ(ptr->info_list.size(), rsp_info_list.size());
        ASSERT_EQ(ptr->info_list.size(), 50);
        for (int i = 0; i < 50; i++) {
            ASSERT_EQ(ptr->info_list[i].address, rsp_info_list[i].address);
            ASSERT_EQ(ptr->info_list[i].start_height, rsp_info_list[i].start_height);
            ASSERT_EQ(ptr->info_list[i].end_height, rsp_info_list[i].end_height);
        }
    }
}