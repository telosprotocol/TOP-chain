// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <time.h>
#include <string>
#include <vector>
#include "xdata/xdata_common.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction.h"
#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xdata.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xversion.h"
#include "xbasic/xserialize_face.h"
#include "xdata/xentire_block.hpp"
#include "xsync/xgossip_message.h"
#include "xpbase/base/top_utils.h"

NS_BEG2(top, sync)

#define xsync_event_queue_size_max (8000)

enum class xsync_msg_err_code_t : uint8_t {
    succ = 0,
    limit = 1,
    decode = 2,
};

struct xsync_message_header_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_header_t() {}
public:

    xsync_message_header_t() {}

    xsync_message_header_t(uint64_t _random, uint8_t _code = 0):
    random(_random),
    code(_code) {
    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();
        SERIALIZE_FIELD_BT(random);
        SERIALIZE_FIELD_BT(code);
        // add padding
        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        DESERIALIZE_FIELD_BT(random);
        DESERIALIZE_FIELD_BT(code);
        // restore padding
        return CALC_LEN();
    }
public:
    uint64_t random{0};
    uint8_t code{0};
    uint8_t padding[16];
};

using xsync_message_header_ptr_t = xobject_ptr_t<xsync_message_header_t>;


struct xsync_message_gossip_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_gossip_t() {}
public:
    xsync_message_gossip_t() {}

    xsync_message_gossip_t(
            const std::vector<xgossip_chain_info_ptr_t> &_info_list,
            const xbyte_buffer_t &_bloom_data) :
    info_list(_info_list),
    bloom_data(_bloom_data) {
    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();

        xgossip_message_t msg;
        xbyte_buffer_t buffer = msg.create_payload(info_list, bloom_data);
        stream << buffer;

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {

        try {

            KEEP_SIZE();

            xgossip_message_t msg;
            xbyte_buffer_t _msg;
            stream >> _msg;

            msg.parse_payload(_msg, info_list, bloom_data);

            return CALC_LEN();
        } catch (...) {
            info_list.clear();
        }

        return 0;
    }

public:
    std::vector<xgossip_chain_info_ptr_t> info_list;
    xbyte_buffer_t bloom_data;
};


class xchain_state_info_t : public top::basic::xserialize_face_t {
public:
    std::string address;
    uint64_t start_height{0};
    uint64_t end_height{0};

    xchain_state_info_t() {

    }

    xchain_state_info_t(const xchain_state_info_t &other) {
        address = other.address;
        start_height = other.start_height;
        end_height = other.end_height;
    }

    virtual ~xchain_state_info_t() {

    }

    virtual int32_t do_write(base::xstream_t & stream) {
        KEEP_SIZE();

        SERIALIZE_FIELD_BT(address);
        SERIALIZE_FIELD_BT(start_height);
        SERIALIZE_FIELD_BT(end_height);

        return CALC_LEN();
    }

    virtual int32_t do_read(base::xstream_t & stream) {
        KEEP_SIZE();

        DESERIALIZE_FIELD_BT(address);
        DESERIALIZE_FIELD_BT(start_height);
        DESERIALIZE_FIELD_BT(end_height);

        return CALC_LEN();
    }
};

struct xsync_message_chain_state_info_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_chain_state_info_t() {}
public:
    xsync_message_chain_state_info_t() {}

    xsync_message_chain_state_info_t(
            const std::vector<xchain_state_info_t> &_info_list):
    info_list(_info_list) {
    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();

        SERIALIZE_CONTAINER(info_list) {
            item.serialize_to(stream);
        }

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {

        try {

            KEEP_SIZE();

            DESERIALIZE_CONTAINER(info_list) {

                xchain_state_info_t info;
                info.serialize_from(stream);
                info_list.push_back(info);
            }

            return CALC_LEN();
        } catch (...) {
            info_list.clear();
        }

        return 0;
    }

public:
    std::vector<xchain_state_info_t> info_list;
};

class xblock_hash_t : public top::basic::xserialize_face_t {
public:
    std::string address;
    uint64_t height{0};
    std::string hash;

    xblock_hash_t() {

    }

    xblock_hash_t(const xblock_hash_t &other) {
        address = other.address;
        height = other.height;
        hash = other.hash;
    }

    virtual ~xblock_hash_t() {

    }

    virtual int32_t do_write(base::xstream_t & stream) {
        KEEP_SIZE();

        SERIALIZE_FIELD_BT(address);
        SERIALIZE_FIELD_BT(height);
        SERIALIZE_FIELD_BT(hash);

        return CALC_LEN();
    }

    virtual int32_t do_read(base::xstream_t & stream) {
        KEEP_SIZE();

        DESERIALIZE_FIELD_BT(address);
        DESERIALIZE_FIELD_BT(height);
        DESERIALIZE_FIELD_BT(hash);

        return CALC_LEN();
    }
};


class xsync_msg_t : public top::basic::xserialize_face_t {
public:
    xsync_msg_t();
    xsync_msg_t(uint64_t _sessionID);

    virtual ~xsync_msg_t() {};

private:
    xsync_msg_t(const xsync_msg_t&);
    xsync_msg_t& operator=(const xsync_msg_t&);

protected:
    int32_t do_write(base::xstream_t& stream) override;
    int32_t do_read(base::xstream_t& stream) override;

public:
    uint64_t get_sessionID() const { return sessionID; }

private:
    uint64_t sessionID;
};

// 8bit
enum enum_sync_data_type {
    enum_sync_data_header = 0x01, //  block
    enum_sync_data_input = 0x02, //
    enum_sync_data_output = 0x04,
    enum_sync_data_offdata = 0x08,
    // enum_sync_data_state = 0x10,
    enum_sync_data_all = 0xFF, //
};

// 8bit
enum enum_sync_block_request_type {
    enum_sync_block_request_push = 0x01,
    enum_sync_block_request_demand = 0x02,
    enum_sync_block_request_ontime = 0x03,
    //enum_sync_block_request_tx = 0x04,
    enum_sync_block_request_max,
};

// 4bit
enum enum_sync_block_block_object_type {
    enum_sync_block_object_xvblock = 0x00,
    enum_sync_block_object_max = 0x0f,
};

// 8bit
enum enum_sync_block_request_param {
    enum_sync_block_by_none = 0x0,
    enum_sync_block_by_height = 0x1,
    enum_sync_block_by_hash = 0x2,
    enum_sync_block_by_viewid = 0x3,
    enum_sync_block_by_txhash = 0x4,
    enum_sync_block_by_height_lists = 0x5,
    enum_sync_block_by_max,
};

// 4bit
enum enum_sync_block_request_extend {
    enum_sync_block_height_adjust = 0x1,
};

// 0—7  ：enum_sync_block_request_type
// 8—15  :  enum_sync_block_request_param
// 16—23 bit：enum_sync_data_type
// 24—27 bit：enum_sync_block_object_type
// 28-31 bit： enum_sync_block_request_extend
#define SYNC_MSG_OPTION_SET(request_type, param, date_type, object_type, extend) \
    (((extend) << 28) | ((object_type) << 24) | ((date_type) << 16) | ((param) << 8) | request_type)

class xsync_msg_block_t : public xsync_msg_t {

public:
    xsync_msg_block_t() {};
    xsync_msg_block_t(std::string const& _address, uint32_t _request_option)
        : address(_address)
        , request_option(_request_option)
    {
    }

    xsync_msg_block_t(uint64_t _sessionID, std::string const& _address, uint32_t _request_option)
        : xsync_msg_t(_sessionID)
        , address(_address)
        , request_option(_request_option)
    {
    }

    virtual ~xsync_msg_block_t() {};

private:
    xsync_msg_block_t(const xsync_msg_t&);
    xsync_msg_block_t& operator=(const xsync_msg_block_t&);

protected:
    int32_t do_write(base::xstream_t& stream) override;
    int32_t do_read(base::xstream_t& stream) override;

public:
    const std::string& get_address() const;
    uint32_t get_request_type() const;
    uint32_t get_requeset_param_type() const;
    uint32_t get_data_type() const;
    uint32_t get_block_object_type() const;
    uint32_t get_extend_bits() const;
    uint32_t get_option() const { return request_option; }

private:
    std::string address{};

    // 0—7  ：enum_sync_block_request_type
    // 8—15  :  enum_sync_block_request_param
    // 16—23 bit：enum_sync_data_type
    // 24—27 bit：enum_sync_block_object_type
    // 28-31 bit： enum_sync_block_request_extend
    uint32_t request_option{0};
};

class xsync_msg_block_request_t : public xsync_msg_block_t {
public:
    xsync_msg_block_request_t() {};
    xsync_msg_block_request_t(std::string const& _address, uint32_t _request_option, uint64_t _height, uint32_t _count, std::string const& _param);

    virtual ~xsync_msg_block_request_t() {};

protected:
    int32_t do_write(base::xstream_t& stream) override;
    int32_t do_read(base::xstream_t& stream) override;

public:
    const std::string& get_requeset_param_str() const { return param; }
    uint64_t get_request_start_height() const { return height_start; }
    int64_t get_time() const { return tm; }
    uint32_t get_count() const { return count; }
    std::string dump() const;

private:
    xsync_msg_block_request_t(const xsync_msg_block_request_t&);
    xsync_msg_block_request_t& operator=(const xsync_msg_t&);

private:
    uint64_t height_start{0};
    uint32_t count{0};
    std::string param{};
    int64_t tm{0}; // no serialize
};

enum enum_sync_block_response_option {
    enum_sync_block_extend_none = 0x0,
    enum_sync_block_extend_proof = 0x1,
};

class xsync_msg_block_response_t : public xsync_msg_block_t {
public:
    xsync_msg_block_response_t() {};
    xsync_msg_block_response_t(uint64_t _sessionID, std::string const& _address, uint32_t _request_option,
        std::vector<std::string> &_blocks_data, uint32_t _extend_option = 0,
        std::string const& _extend_data = "");

    virtual ~xsync_msg_block_response_t() {};

private:
    xsync_msg_block_response_t(const xsync_msg_block_response_t&);
    xsync_msg_block_response_t& operator=(const xsync_msg_block_response_t&);

protected:
    int32_t do_write(base::xstream_t& stream) override;
    int32_t do_read(base::xstream_t& stream) override;

public:
    uint8_t get_block_version() const { return (response_option&0x3); }
    uint32_t get_response_option() const { return response_option; }
    const std::vector<std::string>& get_blocks_data() const { return blocks_data; }
    const std::string& get_extend_data() const { return extend_data; }
    std::vector<data::xblock_ptr_t> get_all_xblock_ptr() const;
    int32_t do_read_from(base::xstream_t& stream) {
        return do_read(stream);
    }

private:

    //0-1 bite,blocker version
    //others no used
    uint32_t response_option{0};
    std::string extend_data{};
    std::vector<std::string> blocks_data{};
};

class xsync_msg_block_push_t : public xsync_msg_block_t {
public:
    xsync_msg_block_push_t() {};
    xsync_msg_block_push_t(std::string const& _address, uint32_t _request_option,
        std::vector<std::string> &_blocks_data, uint32_t _push_option = 0, std::string const& _extend_data = "");

    virtual ~xsync_msg_block_push_t() {};

private:
    xsync_msg_block_push_t(const xsync_msg_block_push_t&);
    xsync_msg_block_push_t& operator=(const xsync_msg_block_push_t&);

protected:
    int32_t do_write(base::xstream_t& stream) override;
    int32_t do_read(base::xstream_t& stream) override;

public:
    uint8_t get_block_version() const { return (push_option&0x3); }
    uint32_t get_push_option() const { return push_option; }
    const std::vector<std::string>& get_blocks_data() const { return blocks_data; }
    const std::string& get_extend_data() const { return extend_data; }
    std::vector<data::xblock_ptr_t> get_all_xblock_ptr() const;

private:

    //0-1 bite,blocker version
    //others no used
    uint32_t push_option{0};
    std::string extend_data{};
    std::vector<std::string> blocks_data{};
};

using xsync_msg_block_request_ptr_t = xobject_ptr_t<xsync_msg_block_request_t>;
using xsync_msg_block_response_ptr_t = xobject_ptr_t<xsync_msg_block_response_t>;
using xsync_msg_block_push_ptr_t = xobject_ptr_t<xsync_msg_block_push_t>;

NS_END2
