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
constexpr uint32_t total_fetch_num = 64;
constexpr uint32_t unit_fetch_num = 4;
constexpr uint32_t max_req_num = 8;

xtop_state_sync::xtop_state_sync() {
    XMETRICS_COUNTER_INCREMENT("statesync_syncers", 1);
}

xtop_state_sync::~xtop_state_sync() {
    XMETRICS_COUNTER_DECREMENT("statesync_syncers", 1);
}

std::shared_ptr<xtop_state_sync> xtop_state_sync::new_state_sync(const common::xaccount_address_t & table,
                                                                 const uint64_t height,
                                                                 const evm_common::xh256_t & block_hash,
                                                                 const evm_common::xh256_t & state_hash,
                                                                 const evm_common::xh256_t & root_hash,
                                                                 std::function<sync_peers(const common::xtable_id_t & id)> peers,
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
    sync->m_symbol = "table: " + table.to_string() + ", height: " + std::to_string(height) + ", root: " + root_hash.hex();
    sync->m_peers_func = peers;
    sync->m_track_func = track_req;
    sync->m_db = db;
    sync->m_kv_db = std::make_shared<evm_common::trie::xkv_db_t>(db, table);
    sync->m_items_per_task = total_fetch_num;
    xinfo("xtop_state_sync::new_state_sync {%s}", sync->symbol().c_str());
    return sync;
}

void xtop_state_sync::run() {
#if !defined(NDEBUG)
    if (running_thead_id_ == std::thread::id{}) {
        running_thead_id_ = std::this_thread::get_id();
    }
    assert(running_thead_id_ == std::this_thread::get_id());
#endif

    do {
        sync_table(m_ec);
        if (m_ec) {
            xwarn("xtop_state_sync::run sync_table error: %s %s, {%s}", m_ec.category().name(), m_ec.message().c_str(), symbol().c_str());
            break;
        }
        sync_trie(m_ec);
        if (m_ec) {
            xwarn("xtop_state_sync::run sync_trie error: %s %s, {%s}", m_ec.category().name(), m_ec.message().c_str(), symbol().c_str());
            break;
        }
    } while(false);

    m_sched->clear();
    m_done = true;
    return;
}

void xtop_state_sync::wait() const {
    while (!m_done) {}
    return;
}

void xtop_state_sync::cancel() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cancel = true;
    }
    m_condition.notify_one();
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

void xtop_state_sync::deliver_req(const state_req & req) {
#if !defined(NDEBUG)
    assert(running_thead_id_ != std::this_thread::get_id());
#endif
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_deliver_list.push(req);
    }
    m_condition.notify_one();
}

void xtop_state_sync::sync_table(std::error_code & ec) {
#if !defined(NDEBUG)
    assert(running_thead_id_ == std::this_thread::get_id());
#endif
    // check exist
    auto const key = base::xvdbkey_t::create_prunable_state_key(m_table.to_string(), m_height, {m_table_block_hash.begin(), m_table_block_hash.end()});
    auto const value = m_db->get_value(key);
    if (!value.empty()) {
        xinfo("xtop_state_sync::sync_table state already exist, %s, block_hash: %s", symbol().c_str(), to_hex(m_table_block_hash).c_str());
        m_sync_table_finish = true;
        return;
    }
    xinfo("xtop_state_sync::sync_table {%s}", symbol().c_str());
    auto condition = [this]() -> bool { return !m_sync_table_finish; };
    auto add_task = [this](sync_peers const & peers) { return assign_table_tasks(peers); };
    auto process_task = [this](state_req & req, std::error_code & ec) { return process_table(req, ec); };
    loop(condition, add_task, process_task, ec);
    if (ec) {
        xwarn("xtop_state_sync::sync_table loop error: %s %s, exit, {%s}", ec.category().name(), ec.message().c_str(), symbol().c_str());
        return;
    }
    return;
}

void xtop_state_sync::sync_trie(std::error_code & ec) {
#if !defined(NDEBUG)
    assert(running_thead_id_ == std::this_thread::get_id());
#endif
    if (m_root.empty()) {
        xinfo("xtop_state_sync::sync_trie root empty, skip sync_trie, {%s}", symbol().c_str());
        return;
    }

    xinfo("xtop_state_sync::sync_trie {%s}", symbol().c_str());
    auto condition = [this]() -> bool { return (m_sched->Pending() > 0); };
    auto add_task = [this](sync_peers const & peers) { return assign_trie_tasks(peers); };
    auto process_task = [this](state_req & req, std::error_code & ec) { return process_trie(req, ec); };
    loop(condition, add_task, process_task, ec);
    if (ec) {
        xwarn("xtop_state_sync::sync_trie loop error: %s %s, exit, {%s}", ec.category().name(), ec.message().c_str(), symbol().c_str());
        return;
    }

    m_sched->Commit(m_kv_db);
    return;
}

void xtop_state_sync::loop(std::function<bool()> condition,
                           std::function<void(sync_peers const &)> add_task,
                           std::function<void(state_req &, std::error_code &)> process_task,
                           std::error_code & ec) {
#if !defined(NDEBUG)
    assert(running_thead_id_ == std::this_thread::get_id());
#endif
    auto net = m_peers_func(m_table.table_id());
    if (net.network == nullptr) {
        xwarn("xtop_state_sync::loop no network availble, exit, {%s}", symbol().c_str());
        ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    while (condition()) {
        xdbg("xtop_state_sync::loop table finish: %d, pending size: %lu, {%s}", m_sync_table_finish.load(), m_sched->Pending(), symbol().c_str());
        // step1: commit
        if (m_unit_bytes_uncommitted >= ideal_batch_size) {
            m_sched->CommitUnit(m_kv_db);
            m_unit_bytes_uncommitted = 0;
        }
        // step2: check peers
        if (net.peers.empty()) {
            xwarn("xtop_state_sync::loop peers empty, exit, {%s}", symbol().c_str());
            ec = error::xerrc_t::state_network_invalid;
            m_cancel = true;
            return;
        }
        // step3: add tasks
        add_task(net);
        // step4: wait reqs
        std::vector<state_req> reqs;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] { return m_cancel || !m_deliver_list.empty(); });

            if (m_cancel) {
                ec = m_ec ? m_ec : error::xerrc_t::state_sync_cancel;
                return;
            }
            while (!m_deliver_list.empty()) {
                reqs.emplace_back(m_deliver_list.front());
                m_deliver_list.pop();
            }
        }
        // step5: process reqs
        for (auto & req : reqs) {
            if (req.nodes_response.empty() && req.units_response.empty()) {
                auto it = std::find(net.peers.begin(), net.peers.end(), req.peer);
                if (it != net.peers.end()) {
                    net.peers.erase(it);
                    xwarn("xtop_state_sync::loop del zero data peer %s, left: %zu, {%s}", it->to_string().c_str(), net.peers.size(), symbol().c_str());
                }
            }
            process_task(req, ec);
            if (ec) {
                xwarn("xtop_state_sync::loop process error: %s %s, {%s}", ec.category().name(), ec.message().c_str(), symbol().c_str());
                return;
            }
        }
    }
}

void xtop_state_sync::assign_table_tasks(const sync_peers & peers) {
    base::xstream_t stream(base::xcontext_t::instance());
    stream << m_table.to_string();
    stream << m_height;
    stream << m_table_block_hash.to_bytes();
    stream << m_req_sequence_id;
    stream << rand();  // defend from filter
    xbytes_t data{stream.data(), stream.data() + stream.size()};
    xinfo("xtop_state_sync::assign_table_tasks %s, block_hash: %s, id: %u, start sync", symbol().c_str(), to_hex(m_table_block_hash).c_str(), m_req_sequence_id);
    state_req req;
    req.peer = send_message(peers, {stream.data(), stream.data() + stream.size()}, xmessage_id_sync_table_request);
    req.start = base::xtime_utl::time_now_ms();
    req.id = m_req_sequence_id++;
    req.type = state_req_type::enum_state_req_table;
    m_track_func(req);
}

void xtop_state_sync::assign_trie_tasks(const sync_peers & peers) {
    for (auto i = 0; i < 2; ++i) {
        state_req req;
        std::vector<evm_common::xh256_t> nodes;
        std::vector<xbytes_t> units;
        fill_tasks(m_items_per_task, m_units_per_task, req, nodes, units);
        if (nodes.size() + units.size() == 0) {
            return;
        }
        if (m_cancel) {
            return;
        }
        base::xstream_t stream(base::xcontext_t::instance());
        stream << m_table.to_string();
        stream << m_req_sequence_id;
        std::vector<xbytes_t> nodes_bytes;
        std::vector<xbytes_t> units_bytes;
        for (auto const & hash : nodes) {
            nodes_bytes.emplace_back(hash.to_bytes());
            xdbg("xtop_state_sync::assign_trie_tasks nodes %s", hash.hex().c_str());
        }
        for (auto const & key : units) {
            units_bytes.emplace_back(key);
            xdbg("xtop_state_sync::assign_trie_tasks units %s", to_hex(key).c_str());
        }
        stream << nodes_bytes;
        stream << units_bytes;
        stream << rand();
        xinfo("xtop_state_sync::assign_trie_tasks total %zu, %zu, {%s}", nodes_bytes.size(), units_bytes.size(), symbol().c_str());
        req.peer = send_message(peers, {stream.data(), stream.data() + stream.size()}, xmessage_id_sync_trie_request);
        req.start = base::xtime_utl::time_now_ms();
        req.id = m_req_sequence_id++;
        req.type = state_req_type::enum_state_req_trie;
        m_track_func(req);
        m_req_nums++;
        if (m_req_nums > max_req_num) {
            return;
        }
    }
}

common::xnode_address_t xtop_state_sync::send_message(const sync_peers & peers, const xbytes_t & msg, common::xmessage_id_t id) {
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t(msg, id);
    auto random_fullnode = peers.peers.at(RandomUint32() % peers.peers.size());
    std::error_code ec;
    peers.network->send_to(random_fullnode, _msg, ec);
    if (ec) {
        xwarn("xtop_state_sync::send_message send net error, %s, %s",
              random_fullnode.account_address().to_string().c_str(),
              peers.network->address().account_address().to_string().c_str());
    }
    return random_fullnode;
}

void xtop_state_sync::fill_tasks(uint32_t total_n, uint32_t unit_n, state_req & req, std::vector<xhash256_t> & nodes_out, std::vector<xbytes_t> & units_out) {
    if (total_n > m_trie_tasks.size() + m_unit_tasks.size()) {
        auto fill = total_n - m_trie_tasks.size() - m_unit_tasks.size();
        auto const res = m_sched->Missing(fill);
        auto const & nodes = std::get<0>(res);
        auto const & unit_hashes = std::get<1>(res);
        auto const & unit_keys = std::get<2>(res);
        assert(unit_hashes.size() == unit_keys.size());
        for (size_t i = 0; i < nodes.size(); i++) {
            xdbg("xtop_state_sync::fill_tasks push missing node: %s", nodes[i].hex().c_str());
            m_trie_tasks.insert(nodes[i]);
        }
        for (size_t i = 0; i < unit_keys.size(); i++) {
            xdbg("xtop_state_sync::fill_tasks push missing unit: %s", to_hex(unit_keys[i]).c_str());
            m_unit_tasks.insert({unit_hashes[i], unit_keys[i]});
        }
    }
    for (auto it = m_trie_tasks.begin(); it != m_trie_tasks.end(); ) {
        if ((nodes_out.size() + units_out.size() >= total_n) || (units_out.size() >= unit_n)) {
            break;
        }
        xdbg("xtop_state_sync::fill_tasks push left node: %s", (*it).hex().c_str());
        nodes_out.push_back(*it);
        req.trie_tasks.insert(*it);
        m_trie_tasks.erase(it++);
    }
    for (auto it = m_unit_tasks.begin(); it != m_unit_tasks.end(); ) {
        if ((nodes_out.size() + units_out.size() >= total_n) || (units_out.size() >= unit_n)) {
            break;
        }
        xdbg("xtop_state_sync::fill_tasks push left unit: %s", to_hex(it->first).c_str());
        units_out.push_back(it->second);
        req.unit_tasks.insert(*it);
        m_unit_tasks.erase(it++);
    }
    req.n_items = nodes_out.size() + units_out.size();
    return;
}

void xtop_state_sync::process_table(state_req & req, std::error_code & ec) {
    if (req.type != state_req_type::enum_state_req_table) {
        return;
    }
    if (m_sync_table_finish) {
        return;
    }
    if (req.nodes_response.empty()) {
        xwarn("xtop_state_sync::process_table empty, table %s, height: %lu, block_hash: %s", m_table.to_string().c_str(), m_height, to_hex(m_table_block_hash).c_str());
        return;
    }
    auto & data = req.nodes_response.at(0);
    xinfo("xtop_state_sync::process_table hash: %s, value size: %zu, {%s}", m_table_block_hash.hex().c_str(), data.size(), symbol().c_str());
    {
        // check state
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvblock_t::create_state_object({data.begin(), data.end()});
        if (nullptr == bstate) {
            xwarn("xtop_state_sync::process_table state null");
            return;
        }
        auto table_state = std::make_shared<data::xtable_bstate_t>(bstate.get());
        auto snapshot = table_state->take_snapshot();
        auto hash = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
        if (evm_common::xh256_t{to_bytes(hash)} != m_table_state_hash) {
            xwarn("xtop_state_sync::process_table state hash mismatch: %s, %s, {%s}", to_hex(hash).c_str(), to_hex(m_table_state_hash).c_str(), symbol().c_str());
            return;
        }
    }
    auto key = base::xvdbkey_t::create_prunable_state_key(m_table.to_string(), m_height, {m_table_block_hash.begin(), m_table_block_hash.end()});
    std::string value{data.begin(), data.end()};
    m_db->set_value(key, value);
    m_sync_table_finish = true;
}

void xtop_state_sync::process_trie(state_req & req, std::error_code & ec) {
    if (req.type != state_req_type::enum_state_req_trie) {
        return;
    }
    m_req_nums--;
    std::error_code ec_internal;
    for (auto const & blob : req.nodes_response) {
        xinfo("xtop_state_sync::process_trie node id: %u, blob: %s, {%s}", req.id, to_hex(blob).c_str(), symbol().c_str());
        auto hash = process_node_data(blob, ec_internal);
        if (ec_internal) {
            if (ec_internal != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_not_requested) &&
                ec_internal != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_already_processed)) {
                xwarn("xtop_state_sync::process_trie invalid state node: %s, %s %s", hash.hex().c_str(), ec.category().name(), ec.message().c_str());
                ec = ec_internal;
                return;
            } else {
                xwarn("xtop_state_sync::process_trie process_node_data abnormal: %s, %s %s", hash.hex().c_str(), ec_internal.category().name(), ec_internal.message().c_str());
            }
        }
        req.trie_tasks.erase(hash);
    }
    for (auto const & blob : req.units_response) {
        xinfo("xtop_state_sync::process_trie unit id: %u, blob size: %zu, {%s}", req.id, blob.size(), symbol().c_str());
        auto hash = process_unit_data(blob, ec_internal);
        if (ec_internal) {
            if (ec_internal != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_not_requested) &&
                ec_internal != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_already_processed)) {
                xwarn("xtop_state_sync::process_trie invalid state node: %s, %s %s", to_hex(hash).c_str(), ec.category().name(), ec.message().c_str());
                ec = ec_internal;
                return;
            } else {
                xwarn("xtop_state_sync::process_trie process_unit_data abnormal: %s, %s %s", to_hex(hash).c_str(), ec_internal.category().name(), ec_internal.message().c_str());
            }
        }
        m_unit_bytes_uncommitted += blob.size();
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

evm_common::xh256_t xtop_state_sync::process_node_data(const xbytes_t & blob, std::error_code & ec) {
    evm_common::trie::SyncResult res;
    auto hash_bytes = to_bytes(utl::xkeccak256_t::digest({blob.begin(), blob.end()}));
    res.Hash = evm_common::xh256_t{hash_bytes};
    res.Data = blob;
    m_sched->Process(res, ec);
    xinfo("xtop_state_sync::process_node_data hash: %s, data: %s", res.Hash.hex().c_str(), to_hex(res.Data).c_str());
    return res.Hash;
}

evm_common::xh256_t xtop_state_sync::process_unit_data(const xbytes_t & blob, std::error_code & ec) {
    evm_common::trie::SyncResult res;
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvblock_t::create_state_object({blob.begin(), blob.end()});
    if (nullptr == bstate) {
        ec = error::xerrc_t::state_data_invalid;
        xerror("xtop_state_sync::process_unit_data hash: %s, data size: %zu, error %s", res.Hash.hex().c_str(), res.Data.size(), ec.message().c_str());
        return {};
    }
    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    auto snapshot = unit_state->take_snapshot();
    auto state_hash = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
    res.Hash = evm_common::xh256_t{xbytes_t{state_hash.begin(), state_hash.end()}};
    res.Data = blob;
    m_sched->ProcessUnit(res, ec);
    if (ec) {
        xwarn("xtop_state_sync::process_unit_data hash: %s, data size: %zu, error %s", res.Hash.hex().c_str(), res.Data.size(), ec.message().c_str());
    } else {
        xinfo("xtop_state_sync::process_unit_data hash: %s, data size: %zu", res.Hash.hex().c_str(), res.Data.size());
    }
    return res.Hash;
}

}  // namespace state_sync
}  // namespace top
