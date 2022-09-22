// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash.hpp"
#include "xcommon/xnode_id.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvnetwork_driver_face.h"

namespace top {
namespace state_sync {

XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_node_request, 0x01);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_node_response, 0x02);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_table_request, 0x03);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_table_response, 0x04);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_unit_request, 0x05);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_unit_response, 0x06);

struct single_state_detail {
    common::xaccount_address_t address;
    uint64_t height{0};
    xhash256_t hash;
    xbytes_t value;

    single_state_detail(common::xaccount_address_t addr, uint64_t h, xhash256_t _hash, const xbytes_t & _value) : address(addr), height(h), hash(_hash), value(_value) {
    }
};

struct sync_result {
    common::xaccount_address_t account;
    uint64_t height{0};
    xhash256_t block_hash;
    xhash256_t state_hash;
    xhash256_t root_hash;
    std::error_code ec;

    sync_result(common::xaccount_address_t _account, uint64_t h, xhash256_t _block_hash, xhash256_t _state_hash, xhash256_t _root, std::error_code _ec)
      : account(_account), height(h), block_hash(_block_hash), state_hash(_state_hash), root_hash(_root), ec(_ec) {
    }
};

struct state_req {
    uint32_t id;
    uint32_t n_items{0};
    std::map<xhash256_t, std::set<std::string>> trie_tasks;
    std::map<xhash256_t, std::pair<xbytes_t, std::set<std::string>>> unit_tasks;
    uint64_t start{0};
    uint64_t delivered{0};
    std::vector<xbytes_t> nodes_response;
    std::vector<xbytes_t> units_response;
};

struct state_res {
    uint32_t id;
    common::xaccount_address_t table;
    std::vector<xbytes_t> nodes;
    std::vector<xbytes_t> units;
};

using state_sync_peers_t = std::vector<std::shared_ptr<vnetwork::xvnetwork_driver_face_t>>;

class xtop_state_sync_face {
public:
    xtop_state_sync_face() = default;
    virtual ~xtop_state_sync_face() = default;

    virtual void run() = 0;
    virtual void cancel() = 0;
    virtual bool is_done() const = 0;
    virtual std::error_code error() const = 0;
    virtual std::string symbol() const = 0;
    virtual sync_result result() const = 0;
    virtual void push_deliver_state(const single_state_detail & detail) = 0;
    virtual void push_deliver_req(const state_req & req) = 0;
};
using xstate_sync_face_t = xtop_state_sync_face;

}  // namespace state_sync
}  // namespace top