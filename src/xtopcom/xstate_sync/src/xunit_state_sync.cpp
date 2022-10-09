// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xunit_state_sync.h"

#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xevm_common/xerror/xerror.h"
#include "xpbase/base/top_utils.h"
#include "xstate_mpt/xstate_sync.h"
#include "xstate_sync/xerror.h"
#include "xutility/xhash.h"
#include "xvledger/xvdbkey.h"
#include "xvnetwork/xvnetwork_message.h"

namespace top {
namespace state_sync {

std::shared_ptr<xtop_unit_state_sync> xtop_unit_state_sync::new_state_sync(const common::xaccount_address_t & account,
                                                                           const base::xaccount_index_t & index,
                                                                           std::function<state_sync_peers_t()> peers,
                                                                           base::xvdbstore_t * db,
                                                                           statestore::xstatestore_face_t * store) {
    auto sync = std::make_shared<xtop_unit_state_sync>();
    sync->m_account = account;
    sync->m_index = index;
    auto height = index.get_latest_unit_height();
    auto block_hash = index.get_latest_unit_hash();
    auto state_hash = index.get_latest_state_hash();
    if (block_hash.empty() || state_hash.empty()) {
        xwarn("xtop_unit_state_sync::xtop_unit_state_sync sync_table hash empty: %s, %s", to_hex(block_hash).c_str(), to_hex(state_hash).c_str());
        return nullptr;
    }
    sync->m_peers_func = peers;
    sync->m_db = db;
    sync->m_store = store;
    xinfo("xtop_unit_state_sync::new_state_sync unit: %s, height: %lu, root: %s", account.c_str(), height, to_hex(block_hash).c_str());
    return sync;
}

void xtop_unit_state_sync::run() {
    sync_unit(m_ec);
    if (m_ec) {
        xwarn("xtop_unit_state_sync::run sync_unit error, unit: %s, index: %s, error: %s, %s",
              m_account.c_str(),
              m_index.dump().c_str(),
              m_ec.category().name(),
              m_ec.message().c_str());
    }

    m_done = true;
    return;
}

void xtop_unit_state_sync::wait() const {
    while (!m_done) {}
    return;
}

void xtop_unit_state_sync::cancel() {
    m_cancel = true;
    wait();
}

std::error_code xtop_unit_state_sync::error() const {
    return m_ec;
}

std::string xtop_unit_state_sync::symbol() const {
    return m_index.dump();
}

sync_result xtop_unit_state_sync::result() const {
    return {m_account, m_index.get_latest_unit_height(), xhash256_t(to_bytes(m_index.get_latest_unit_hash())), xhash256_t(to_bytes(m_index.get_latest_state_hash())), {}, m_ec};
}

bool xtop_unit_state_sync::is_done() const {
    return m_done;
}

void xtop_unit_state_sync::push_deliver_state(const single_state_detail & detail) {
    if (m_done) {
        return;
    }
    auto bstate = base::xvblock_t::create_state_object({detail.value.begin(), detail.value.end()});
    auto table_state = std::make_shared<data::xtable_bstate_t>(bstate);
    auto snapshot = table_state->take_snapshot();
    auto hash = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
    if (hash != m_index.get_latest_state_hash()) {
        xwarn("xtop_unit_state_sync::push_deliver_state state hash mismatch: %s, %s", to_hex(hash).c_str(), to_hex(m_index.get_latest_state_hash()).c_str());
        return;
    }
    xinfo("xtop_unit_state_sync::push_deliver_state account: %s, index: %s, value: %s", m_account.c_str(), m_index.dump().c_str(), to_hex(detail.value).c_str());
    auto key = base::xvdbkey_t::create_prunable_unit_state_key(m_account.value(), detail.height, {detail.hash.begin(), detail.hash.end()});
    std::string value{detail.value.begin(), detail.value.end()};
    m_db->set_value(key, value);
    m_done = true;
}

void xtop_unit_state_sync::push_deliver_req(const state_req & req) {
    // not use
    xassert(false);
    return;
}

void xtop_unit_state_sync::sync_unit(std::error_code & ec) {
    // check exist
    auto unitstate = m_store->get_unit_state_by_accountindex(m_account, m_index);
    if (unitstate != nullptr) {
        xinfo("xtop_unit_state_sync::sync_unit index: %s, state already exist in db", m_index.dump().c_str());
        m_done = true;
        return;
    }

    // send request
    auto network = available_network();
    if (network == nullptr) {
        xwarn("xtop_unit_state_sync::sync_unit index: %s, no network availble, exit", m_index.dump().c_str());
        ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    auto peers = available_peers(network);
    if (peers.empty()) {
        xwarn("xtop_unit_state_sync::sync_unit index: %s, peers empty, exit", m_index.dump().c_str());
        ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    base::xstream_t stream(base::xcontext_t::instance());
    state_mpt::xaccount_info_t info;
    info.m_account = m_account;
    info.m_index = m_index;
    stream << info.encode();
    stream << rand();  // defend from filter
    xbytes_t data{stream.data(), stream.data() + stream.size()};
    xinfo("xtop_unit_state_sync::sync_unit, account: %s, index: %s", m_account.c_str(), m_index.dump().c_str());

    uint32_t cnt{0};
    while (!m_done) {
        // resend over 5s
        if (cnt % 50 == 0) {
            send_message(network, peers, data, xmessage_id_sync_unit_request);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cnt++;
        if (cnt > 10 * 60) {
            // 1 min overtime
            xwarn("xtop_unit_state_sync::sync_unit account: %s, index: %s, overtime, cnt: %u", m_account.c_str(), m_index.dump().c_str(), cnt);
            ec = error::xerrc_t::state_sync_overtime;
            m_cancel = true;
            return;
        }
    }
}

void xtop_unit_state_sync::send_message(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                   const std::vector<common::xnode_address_t> & peers,
                                   const xbytes_t & msg,
                                   common::xmessage_id_t id) {
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t(msg, id);
    xassert(!peers.empty());
    auto random_fullnode = peers.at(RandomUint32() % peers.size());
    std::error_code ec;
    network->send_to(random_fullnode, _msg, ec);
    if (ec) {
        xwarn("xtop_unit_state_sync::send_message send net error, %s, %s", random_fullnode.account_address().c_str(), network->address().account_address().c_str());
    }
}

std::shared_ptr<vnetwork::xvnetwork_driver_face_t> xtop_unit_state_sync::available_network() const {
    auto peers = m_peers_func();
    for (auto it = peers.rbegin(); it != peers.rend(); it++) {
        std::error_code ec;
        auto fullnode_list = (*it)->fullnode_addresses(ec);
        if (ec || fullnode_list.empty() || (fullnode_list.size() == 1 && fullnode_list[0] == (*it)->address())) {
            continue;
        }
        return (*it);
    }
    return nullptr;
}

std::vector<common::xnode_address_t> xtop_unit_state_sync::available_peers(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network) const {
    std::error_code ec;
    auto fullnode_list = network->fullnode_addresses(ec);
    if (ec) {
        return {};
    }
    auto it = std::find(fullnode_list.begin(), fullnode_list.end(), network->address());
    if (it != fullnode_list.end()) {
        fullnode_list.erase(it);
    }
    xassert(!fullnode_list.empty());
    return fullnode_list;
}

}  // namespace state_sync
}  // namespace top