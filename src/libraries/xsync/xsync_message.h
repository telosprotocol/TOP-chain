// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <time.h>
#include <string>
#include <vector>
#include "xbasic/xns_macro.h"
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

NS_BEG2(top, sync)

using namespace top::base;

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

struct xsync_message_get_blocks_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_get_blocks_t() {}
public:
    xsync_message_get_blocks_t() {
    }

    xsync_message_get_blocks_t(
            const std::string& _owner,
            uint64_t _start_height,
            uint32_t _count) :
    owner(_owner),
    start_height(_start_height),
    count(_count) {
    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();
        SERIALIZE_FIELD_BT(owner);
        SERIALIZE_FIELD_BT(start_height);
        SERIALIZE_FIELD_BT(count);

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {

        try {

            KEEP_SIZE();
            DESERIALIZE_FIELD_BT(owner);
            DESERIALIZE_FIELD_BT(start_height);
            DESERIALIZE_FIELD_BT(count);
        
            return CALC_LEN();
        } catch (...) {
            owner = "";
            start_height = 0;
            count = 0;
        }

        return 0;
    }

public:
    std::string owner;
    uint64_t start_height;
    uint32_t count;
};

struct xsync_message_blocks_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_blocks_t() {}
public:
    xsync_message_blocks_t() {
    }

    xsync_message_blocks_t(
            const std::string& _owner,
            const std::vector<data::xblock_ptr_t> &_blocks) :
    owner(_owner),
    blocks(_blocks) {
    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();
        SERIALIZE_FIELD_BT(owner);

        std::vector<data::xentire_block_ptr_t> vector_entire_block;
        for (auto &it: blocks) {
            data::xentire_block_ptr_t entire_block = make_object_ptr<data::xentire_block_t>();
            entire_block->block_ptr = it;
            vector_entire_block.push_back(entire_block);
        }

        SERIALIZE_CONTAINER(vector_entire_block) {
            item->serialize_to(stream);
        }

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {

        try {
            KEEP_SIZE();
            DESERIALIZE_FIELD_BT(owner);

            std::vector<data::xentire_block_ptr_t> vector_entire_block;

            DESERIALIZE_CONTAINER(vector_entire_block) {

#if 0
                data::xentire_block_ptr_t entire_block;
                DEFAULT_DESERIALIZE_PTR(entire_block, data::xentire_block_t);
#endif
                data::xentire_block_ptr_t entire_block = make_object_ptr<data::xentire_block_t>();
                entire_block->serialize_from(stream);

                if (entire_block != nullptr)
                    vector_entire_block.push_back(entire_block);
                    
            }

            for (auto &it: vector_entire_block) {
                if (it->block_ptr != nullptr)
                    blocks.push_back(it->block_ptr);
            }

            return CALC_LEN();
        } catch (...) {
            owner = "";
            blocks.clear();
        }

        return 0;
    }

public:
    std::string owner;
    std::vector<data::xblock_ptr_t> blocks;
};

struct xsync_message_newblock_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_newblock_t() {}
public:
    xsync_message_newblock_t() {}

    xsync_message_newblock_t(
            const data::xblock_ptr_t &_block) :
    block(_block) {

    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();

        data::xentire_block_ptr_t entire_block = make_object_ptr<data::xentire_block_t>();
        entire_block->block_ptr = block;

        if (entire_block != nullptr) {
            entire_block->serialize_to(stream);
        }

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {

        try {

            KEEP_SIZE();

            data::xentire_block_ptr_t entire_block = make_object_ptr<data::xentire_block_t>();
            entire_block->serialize_from(stream);

            block = entire_block->block_ptr;

            return CALC_LEN();
        } catch (...) {
            block = nullptr;
        }

        return 0;
    }

public:
    data::xblock_ptr_t block{};
};

struct xsync_message_newblockhash_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_newblockhash_t() {}
public:
    xsync_message_newblockhash_t() {
    }

    xsync_message_newblockhash_t(
            const std::string &_address,
            uint64_t _height,
            uint64_t _view_id) :
    address(_address),
    height(_height),
    view_id(_view_id) {
    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();
        SERIALIZE_FIELD_BT(address);
        SERIALIZE_FIELD_BT(height);
        SERIALIZE_FIELD_BT(view_id);
        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {
        try {
            KEEP_SIZE();
            DESERIALIZE_FIELD_BT(address);
            DESERIALIZE_FIELD_BT(height);
            DESERIALIZE_FIELD_BT(view_id);
            return CALC_LEN();

        } catch (...) {
            address = "";
            height = 0;
            view_id = 0;
        }

        return 0;
    }

public:
    std::string address;
    uint64_t height;
    uint64_t view_id;
};

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

class xlatest_block_info_t : public top::basic::xserialize_face_t {
public:
    std::string address;
    uint64_t height{0};
    uint64_t view_id{0};
    std::string hash;

    xlatest_block_info_t() {

    }

    xlatest_block_info_t(const xlatest_block_info_t &other) {
        address = other.address;
        height = other.height;
        view_id = other.view_id;
        hash = other.hash;
    }

    virtual ~xlatest_block_info_t() {

    }

    virtual int32_t do_write(base::xstream_t & stream) {
        KEEP_SIZE();

        SERIALIZE_FIELD_BT(address);
        SERIALIZE_FIELD_BT(height);
        SERIALIZE_FIELD_BT(view_id);
        SERIALIZE_FIELD_BT(hash);

        return CALC_LEN();
    }

    virtual int32_t do_read(base::xstream_t & stream) {
        KEEP_SIZE();

        DESERIALIZE_FIELD_BT(address);
        DESERIALIZE_FIELD_BT(height);
        DESERIALIZE_FIELD_BT(view_id);
        DESERIALIZE_FIELD_BT(hash);

        return CALC_LEN();
    }
};

struct xsync_message_latest_block_info_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_latest_block_info_t() {}
public:
    xsync_message_latest_block_info_t() {}

    xsync_message_latest_block_info_t(
            const std::vector<xlatest_block_info_t> &_info_list):
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

                xlatest_block_info_t info;
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
    std::vector<xlatest_block_info_t> info_list;
};

class xlatest_block_item_t : public top::basic::xserialize_face_t {
public:
    uint64_t height{0};
    std::string hash;

    xlatest_block_item_t() {

    }

    xlatest_block_item_t(const xlatest_block_item_t &other) {
        height = other.height;
        hash = other.hash;
    }

    virtual ~xlatest_block_item_t() {

    }

    virtual int32_t do_write(base::xstream_t & stream) {
        KEEP_SIZE();

        SERIALIZE_FIELD_BT(height);
        SERIALIZE_FIELD_BT(hash);

        return CALC_LEN();
    }

    virtual int32_t do_read(base::xstream_t & stream) {
        KEEP_SIZE();

        DESERIALIZE_FIELD_BT(height);
        DESERIALIZE_FIELD_BT(hash);

        return CALC_LEN();
    }
};

struct xsync_message_get_latest_blocks_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_get_latest_blocks_t() {}
public:
    xsync_message_get_latest_blocks_t() {
    }

    xsync_message_get_latest_blocks_t(
            const std::string& _address,
            const std::vector<xlatest_block_item_t> &_list) :
    address(_address),
    list(_list) {
    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();

        SERIALIZE_FIELD_BT(address);
        SERIALIZE_CONTAINER(list) {
            item.serialize_to(stream);
        }

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {

        try {

            KEEP_SIZE();

            DESERIALIZE_FIELD_BT(address);
            DESERIALIZE_CONTAINER(list) {

                xlatest_block_item_t item;
                item.serialize_from(stream);
                list.push_back(item);
            }

            return CALC_LEN();
        } catch (...) {
            address = "";
            list.clear();
        }

        return 0;
    }

public:
    std::string address;
    std::vector<xlatest_block_item_t> list;
};

struct xsync_message_latest_blocks_t : public top::basic::xserialize_face_t {
protected:
    virtual ~xsync_message_latest_blocks_t() {}
public:
    xsync_message_latest_blocks_t() {}

    xsync_message_latest_blocks_t(
            const std::vector<data::xblock_ptr_t> &_blocks) :
    blocks(_blocks) {

    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();

        std::vector<data::xentire_block_ptr_t> vector_entire_block;
        for (auto &it: blocks) {
            data::xentire_block_ptr_t entire_block = make_object_ptr<data::xentire_block_t>();
            entire_block->block_ptr = it;
            vector_entire_block.push_back(entire_block);
        }

        SERIALIZE_CONTAINER(vector_entire_block) {
            item->serialize_to(stream);
        }

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {

        try {

            KEEP_SIZE();

            std::vector<data::xentire_block_ptr_t> vector_entire_block;

            DESERIALIZE_CONTAINER(vector_entire_block) {

                data::xentire_block_ptr_t entire_block = make_object_ptr<data::xentire_block_t>();
                entire_block->serialize_from(stream);

                if (entire_block != nullptr)
                    vector_entire_block.push_back(entire_block);
            }

            for (auto &it: vector_entire_block) {
                if (it->block_ptr != nullptr)
                    blocks.push_back(it->block_ptr);
            }

            return CALC_LEN();
        } catch (...) {
            blocks.clear();
        }

        return 0;
    }

public:
    std::vector<data::xblock_ptr_t> blocks;
};


using xsync_message_get_blocks_ptr_t = xobject_ptr_t<xsync_message_get_blocks_t>;
using xsync_message_blocks_ptr_t = xobject_ptr_t<xsync_message_blocks_t>;

using xsync_message_newblock_ptr_t = xobject_ptr_t<xsync_message_newblock_t>;
using xsync_message_newblockhash_ptr_t = xobject_ptr_t<xsync_message_newblockhash_t>;

using xsync_message_gossip_ptr_t = xobject_ptr_t<xsync_message_gossip_t>;

using xsync_message_latest_block_info_ptr_t = xobject_ptr_t<xsync_message_latest_block_info_t>;
using xsync_message_get_latest_blocks_ptr_t = xobject_ptr_t<xsync_message_get_latest_blocks_t>;
using xsync_message_latest_blocks_ptr_t = xobject_ptr_t<xsync_message_latest_blocks_t>;

NS_END2
