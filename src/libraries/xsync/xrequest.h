// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <vector>
#include <string>
#include "xdata/xdata_common.h"
#include "xvnetwork/xaddress.h"
#include "xdata/xdatautil.h"
#include "xbasic/xbyte_buffer.h"

NS_BEG2(top, sync)

// for priority
enum enum_xrequest_type_t {
    xrequest_type_query_entire_block,
    xrequest_type_max
};

// any account has multi-type request
// but same request only allow one, has same key

class xrequest_t {
public:

    xrequest_t(enum_xrequest_type_t req_type) :
    type(req_type) {
    }

    virtual ~xrequest_t() {
    }

    enum_xrequest_type_t type;
    vnetwork::xvnode_address_t self_addr{};
    vnetwork::xvnode_address_t target_addr{};
};

using xrequest_ptr_t = std::shared_ptr<xrequest_t>;

class xentire_block_request_t : public xrequest_t {
public:

    xentire_block_request_t(
            const std::string& _owner,
            uint64_t _start_height,
            uint32_t _count) :
    xrequest_t(xrequest_type_query_entire_block),
    owner(_owner),
    start_height(_start_height),
    count(_count) {
    }

    virtual ~xentire_block_request_t() {
    }

    std::string owner{};
    uint64_t start_height;
    uint32_t count;
    int64_t create_time;
    int64_t try_time;
    int64_t send_time;
};

using xentire_block_request_ptr_t = std::shared_ptr<xentire_block_request_t>;

NS_END2
