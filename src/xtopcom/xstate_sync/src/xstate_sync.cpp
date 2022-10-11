// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xstate_sync.h"

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

constexpr uint32_t ideal_batch_size = 100 * 1024;
constexpr uint32_t fetch_num = 64;

std::shared_ptr<xtop_state_sync> xtop_state_sync::new_state_sync(const common::xaccount_address_t & table,
                                                                 const uint64_t height,
                                                                 const xhash256_t & block_hash,
                                                                 const xhash256_t & state_hash,
                                                                 const xhash256_t & root_hash,
                                                                 std::function<state_sync_peers_t()> peers,
                                                                 std::function<void(const state_req &)> track_req,
                                                                 base::xvdbstore_t * db,
                                                                 bool sync_unit) {
    auto sync = std::make_shared<xtop_state_sync>();
    sync->m_sched = state_mpt::new_state_sync(table, root_hash, db, sync_unit);
    sync->m_table = table;
    sync->m_height = height;
    sync->m_table_block_hash = block_hash;
    sync->m_table_state_hash = state_hash;
    sync->m_root = root_hash;
    sync->m_symbol = "table: " + table.value() + ", height: " + std::to_string(height) + ", root: " + root_hash.as_hex_str();
    sync->m_peers_func = peers;
    sync->m_track_func = track_req;
    sync->m_db = db;
    sync->m_kv_db = std::make_shared<evm_common::trie::xkv_db_t>(db, table);
    xinfo("xtop_state_sync::new_state_sync {%s}", sync->symbol().c_str());
    return sync;
}

void xtop_state_sync::run() {
    do {
        sync_table(m_ec);
        if (m_ec) {
            xwarn("xtop_state_sync::run sync_table error: %s %s, {%s}", m_ec.category().name(), m_ec.message().c_str(), symbol().c_str());
            break;
        }
        loop(m_ec);
        if (m_ec) {
            xwarn("xtop_state_sync::run loop error: %s %s, {%s}", m_ec.category().name(), m_ec.message().c_str(), symbol().c_str());
            break;
        }
    } while (0);

    m_done = true;
    return;
}

void xtop_state_sync::wait() const {
    while (!m_done) {}
    return;
}

void xtop_state_sync::cancel() {
    m_cancel = true;
    wait();
}

std::error_code xtop_state_sync::error() const {
    return m_ec;
}

std::string xtop_state_sync::symbol() const {
    return m_symbol;
}

sync_result xtop_state_sync::result() const {
    return {m_table, m_height, m_table_block_hash, m_table_state_hash, m_root, m_ec};
}

bool xtop_state_sync::is_done() const {
    return m_done;
}

void xtop_state_sync::push_deliver_req(const state_req & req) {
    std::lock_guard<std::mutex> lock(m_deliver_mutex);
    m_deliver_list.push_back(req);
}

void xtop_state_sync::pop_deliver_req() {
    std::lock_guard<std::mutex> lock(m_deliver_mutex);
    m_deliver_list.pop_front();
}

void xtop_state_sync::push_deliver_state(const single_state_detail & detail) {
    if (m_sync_table_finish) {
        return;
    }

    if (detail.address != m_table) {
        xwarn("xtop_state_sync::push_deliver_state address mismatch: %s, %s, {%s}", detail.address.c_str(), m_table.c_str(), symbol().c_str());
        return;
    }
    if (detail.height != m_height) {
        xwarn("xtop_state_sync::push_deliver_state height mismatch: %lu, %lu, {%s}", detail.height, m_height, symbol().c_str());
        return;
    }
    if (detail.hash != m_table_block_hash) {
        xwarn("xtop_state_sync::push_deliver_state block hash mismatch: %s, %s, {%s}", detail.hash.as_hex_str().c_str(), m_table_block_hash.as_hex_str().c_str(), symbol().c_str());
        return;
    }

    xinfo("xtop_state_sync::push_deliver_state hash: %s, value size: %zu, {%s}", detail.hash.as_hex_str().c_str(), detail.value.size(), symbol().c_str());
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvblock_t::create_state_object({detail.value.begin(), detail.value.end()});
    if (nullptr == bstate) {
        xerror("xtop_state_sync::push_deliver_state state null");
        return;
    }
    auto table_state = std::make_shared<data::xtable_bstate_t>(bstate.get());
    auto snapshot = table_state->take_snapshot();
    auto hash = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
    if (xhash256_t{to_bytes(hash)} != m_table_state_hash) {
        xwarn("xtop_state_sync::process_table state hash mismatch: %s, %s, {%s}", to_hex(hash).c_str(), to_hex(m_table_state_hash).c_str(), symbol().c_str());
        return;
    }
    auto key = base::xvdbkey_t::create_prunable_state_key(m_table.value(), m_height, {m_table_block_hash.begin(), m_table_block_hash.end()});
    std::string value{detail.value.begin(), detail.value.end()};
    m_db->set_value(key, value);
    m_sync_table_finish = true;
}

void xtop_state_sync::sync_table(std::error_code & ec) {
    // check exist
    auto key = base::xvdbkey_t::create_prunable_state_key(m_table.value(), m_height, {m_table_block_hash.begin(), m_table_block_hash.end()});
    auto value = m_db->get_value(key);
    if (!value.empty()) {
        xinfo("xtop_state_sync::sync_table state already exist, %s, block_hash: %s", symbol().c_str(), to_hex(m_table_block_hash).c_str());
        m_sync_table_finish = true;
        return;
    }
    // send request
    auto network = available_network();
    if (network == nullptr) {
        xwarn("xtop_state_sync::sync_table no network availble, exit, {%s}", symbol().c_str());
        ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    auto peers = available_peers(network);
    if (peers.empty()) {
        xwarn("xtop_state_sync::sync_table peers empty, exit, {%s}", symbol().c_str());
        ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    base::xstream_t stream(base::xcontext_t::instance());
    stream << m_table.value();
    stream << m_height;
    stream << m_table_block_hash.to_bytes();
    stream << rand();  // defend from filter
    xbytes_t data{stream.data(), stream.data() + stream.size()};
    xinfo("xtop_state_sync::sync_table %s, block_hash: %s, start sync", symbol().c_str(), to_hex(m_table_block_hash).c_str());

    uint32_t cnt{0};
    while (!m_sync_table_finish) {
        // resend over 5s
        if (cnt % 50 == 0) {
            send_message(network, peers, data, xmessage_id_sync_table_request);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cnt++;
        if (cnt > 10 * 60) {
            // 1 min overtime
            xwarn("xtop_state_sync::sync_table overtime, cnt: %u, {%s}", cnt, symbol().c_str());
            ec = error::xerrc_t::state_sync_overtime;
            m_cancel = true;
            return;
        }
    }
}

void xtop_state_sync::loop(std::error_code & ec) {
    xinfo("xtop_state_sync::loop {%s}", symbol().c_str());

    auto network = available_network();
    if (network == nullptr) {
        xwarn("xtop_state_sync::loop no network availble, exit, {%s}", symbol().c_str());
        ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    auto peers = available_peers(network);
    if (peers.empty()) {
        xwarn("xtop_state_sync::loop peers empty, exit, {%s}", symbol().c_str());
        ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    uint32_t cnt{0};
    while (m_sched->Pending() > 0) {
        xdbg("xtop_state_sync::loop pending size: %lu, {%s}", m_sched->Pending(), symbol().c_str());

        if (m_cancel) {
            if (m_ec) {
                ec = m_ec;
            } else {
                ec = error::xerrc_t::state_sync_cancel;
            }
            return;
        }

        assign_tasks(network, peers);

        if (m_deliver_list.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            cnt++;
            if (cnt > 10 * 60 * 10) {
                // 10 min overtime
                xwarn("xtop_state_sync::loop overtime, cnt: %u, {%s}", cnt, symbol().c_str());
                ec = error::xerrc_t::state_sync_overtime;
                m_cancel = true;
                return;
            }
            continue;
        } else {
            auto req = m_deliver_list.front();
            process(req, ec);
            if (ec) {
                xwarn("xtop_state_sync::loop process error: %s %s, {%s}", ec.category().name(), ec.message().c_str(), symbol().c_str());
                return;
            }
            pop_deliver_req();
        }
    }

    m_sched->Commit(m_kv_db);
    return;
}

void xtop_state_sync::assign_tasks(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const std::vector<common::xnode_address_t> & peers) {
    state_req req;
    std::vector<xhash256_t> nodes;
    std::vector<xbytes_t> units;
    fill_tasks(fetch_num, req, nodes, units);
    if (nodes.size() + units.size() == 0) {
        return;
    }
    if (m_cancel) {
        return;
    }
    base::xstream_t stream(base::xcontext_t::instance());
    stream << m_table.value();
    stream << m_req_sequence_id;
    std::vector<xbytes_t> nodes_bytes;
    std::vector<xbytes_t> units_bytes;
    for (auto hash : nodes) {
        nodes_bytes.emplace_back(hash.to_bytes());
        xdbg("xtop_state_sync::assign_tasks nodes %s", hash.as_hex_str().c_str());
    }
    for (auto key : units) {
        units_bytes.emplace_back(key);
        xdbg("xtop_state_sync::assign_tasks units %s", to_hex(key).c_str());
    }
    stream << nodes_bytes;
    stream << units_bytes;
    stream << rand();
    xinfo("xtop_state_sync::assign_tasks total %zu, %zu, {%s}", nodes_bytes.size(), units_bytes.size(), symbol().c_str());
    send_message(network, peers, {stream.data(), stream.data() + stream.size()}, xmessage_id_sync_node_request);
    req.start = base::xtime_utl::time_now_ms();
    req.id = m_req_sequence_id++;
    m_track_func(req);
}

void xtop_state_sync::send_message(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                   const std::vector<common::xnode_address_t> & peers,
                                   const xbytes_t & msg,
                                   common::xmessage_id_t id) {
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t(msg, id);
    xassert(!peers.empty());
    auto random_fullnode = peers.at(RandomUint32() % peers.size());
    std::error_code ec;
    network->send_to(random_fullnode, _msg, ec);
    if (ec) {
        xwarn("xtop_state_sync::send_message send net error, %s, %s", random_fullnode.account_address().c_str(), network->address().account_address().c_str());
    }
}

void xtop_state_sync::fill_tasks(uint32_t n, state_req & req, std::vector<xhash256_t> & nodes_out, std::vector<xbytes_t> & units_out) {
    if (n > m_trie_tasks.size() + m_unit_tasks.size()) {
        auto fill = n - m_trie_tasks.size() - m_unit_tasks.size();
        auto res = m_sched->Missing(fill);
        auto nodes = std::get<0>(res);
        auto unit_hashes = std::get<1>(res);
        auto unit_keys = std::get<2>(res);
        for (size_t i = 0; i < nodes.size(); i++) {
            xdbg("xtop_state_sync::fill_tasks push missing node: %s", nodes[i].as_hex_str().c_str());
            m_trie_tasks.insert({nodes[i], {}});
        }
        for (size_t i = 0; i < unit_keys.size(); i++) {
            xdbg("xtop_state_sync::fill_tasks push missing unit: %s", to_hex(unit_keys[i]).c_str());
            m_unit_tasks.insert({unit_hashes[i], {unit_keys[i], std::set<std::string>()}});
        }
    }
    for (auto it = m_trie_tasks.begin(); it != m_trie_tasks.end(); ) {
        if (nodes_out.size() + units_out.size() >= n) {
            break;
        }
        xdbg("xtop_state_sync::fill_tasks push left node: %s", it->first.as_hex_str().c_str());
        nodes_out.push_back(it->first);
        req.trie_tasks.insert(*it);
        m_trie_tasks.erase(it++);
    }
    for (auto it = m_unit_tasks.begin(); it != m_unit_tasks.end(); ) {
        if (nodes_out.size() + units_out.size() >= n) {
            break;
        }
        xdbg("xtop_state_sync::fill_tasks push left unit: %s", to_hex(it->first).c_str());
        units_out.push_back(it->second.first);
        req.unit_tasks.insert(*it);
        m_unit_tasks.erase(it++);
    }
    req.n_items = nodes_out.size() + units_out.size();
    return;
}

void xtop_state_sync::process(state_req & req, std::error_code & ec) {
    std::error_code ec_internal;
    for (auto blob : req.nodes_response) {
        xinfo("xtop_state_sync::process node id: %u, blob: %s, {%s}", req.id, to_hex(blob).c_str(), symbol().c_str());
        auto hash = process_node_data(blob, ec_internal);
        if (ec_internal) {
            if (ec_internal != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_not_requested) &&
                ec_internal != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_already_processed)) {
                xwarn("xtop_state_sync::process invalid state node: %s, %s %s", hash.as_hex_str().c_str(), ec.category().name(), ec.message().c_str());
                ec = ec_internal;
                return;
            } else {
                xwarn("xtop_state_sync::process process_node_data abnormal: %s, %s %s", hash.as_hex_str().c_str(), ec_internal.category().name(), ec_internal.message().c_str());      
            }
        }
        req.trie_tasks.erase(hash);
    }
    for (auto blob : req.units_response) {
        xinfo("xtop_state_sync::process unit id: %u, blob size: %zu, {%s}", req.id, blob.size(), symbol().c_str());
        auto hash = process_unit_data(blob, ec_internal);
        if (ec_internal) {
            if (ec_internal != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_not_requested) &&
                ec_internal != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_already_processed)) {
                xwarn("xtop_state_sync::process invalid state node: %s, %s %s", to_hex(hash).c_str(), ec.category().name(), ec.message().c_str());
                ec = ec_internal;
                return;
            } else {
                xwarn("xtop_state_sync::process process_unit_data abnormal: %s, %s %s", to_hex(hash).c_str(), ec_internal.category().name(), ec_internal.message().c_str());
            }
        }
        // DEBUG
        if (!req.unit_tasks.count(hash)) {
            xwarn("xtop_state_sync::process process_unit_data not find key: %s", to_hex(hash).c_str());
            for (auto t : req.unit_tasks) {
                xwarn("%s", to_hex(t.first).c_str());
            }
        }
        req.unit_tasks.erase(hash);
    }
    // retry queue
    for (auto pair : req.trie_tasks) {
        m_trie_tasks.insert(pair);
    }
    for (auto pair : req.unit_tasks) {
        m_unit_tasks.insert(pair);
    }
    return;
}

xhash256_t xtop_state_sync::process_node_data(xbytes_t & blob, std::error_code & ec) {
    evm_common::trie::SyncResult res;
    auto hash_bytes = to_bytes(utl::xkeccak256_t::digest({blob.begin(), blob.end()}));
    res.Hash = xhash256_t{hash_bytes};
    res.Data = blob;
    m_sched->Process(res, ec);
    xinfo("xtop_state_sync::process_node_data hash: %s, data: %s", res.Hash.as_hex_str().c_str(), to_hex(res.Data).c_str());
    return res.Hash;
}

xhash256_t xtop_state_sync::process_unit_data(xbytes_t & blob, std::error_code & ec) {
    evm_common::trie::SyncResult res;
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvblock_t::create_state_object({blob.begin(), blob.end()});
    if (nullptr == bstate) {
        ec = error::xerrc_t::state_data_invalid;
        xerror("xtop_state_sync::process_unit_data hash: %s, data size: %zu, error %s", res.Hash.as_hex_str().c_str(), res.Data.size(), ec.message().c_str());
        return {};
    }
    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    auto snapshot = unit_state->take_snapshot();
    auto state_hash = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
    res.Hash = xhash256_t{xbytes_t{state_hash.begin(), state_hash.end()}};
    res.Data = blob;
    m_sched->ProcessUnit(res, ec);
    if (ec) {
        xwarn("xtop_state_sync::process_unit_data hash: %s, data size: %zu, error %s", res.Hash.as_hex_str().c_str(), res.Data.size(), ec.message().c_str());
    } else {
        xinfo("xtop_state_sync::process_unit_data hash: %s, data size: %zu", res.Hash.as_hex_str().c_str(), res.Data.size());
    }
    return res.Hash;
}

std::shared_ptr<vnetwork::xvnetwork_driver_face_t> xtop_state_sync::available_network() const {
    auto peers = m_peers_func();
    for (auto it = peers.rbegin(); it != peers.rend(); it++) {
        // table check
        auto id = m_table.table_id().value();
        auto table_ids = (*it)->table_ids();
        std::set<uint16_t> table_ids_set{table_ids.begin(), table_ids.end()};
        if (!table_ids_set.count(id)) {
            continue;
        }
        // fullnode check
        std::error_code ec;
        auto fullnode_list = (*it)->fullnode_addresses(ec);
        if (ec || fullnode_list.empty() || (fullnode_list.size() == 1 && fullnode_list[0] == (*it)->address())) {
            continue;
        }
        return (*it);
    }
    return nullptr;
}

std::vector<common::xnode_address_t> xtop_state_sync::available_peers(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network) const {
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