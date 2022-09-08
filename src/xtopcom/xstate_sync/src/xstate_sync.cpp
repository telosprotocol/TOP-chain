// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xstate_sync.h"

#include "xevm_common/xerror/xerror.h"
#include "xstate_mpt/xstate_sync.h"
#include "xstate_sync/xerror.h"
#include "xutility/xhash.h"

namespace top {
namespace state_sync {

constexpr uint32_t ideal_batch_size = 100 * 1024;
constexpr uint32_t fetch_num = 128;

xtop_state_sync::xtop_state_sync(const std::string & table, const xhash256_t & root, base::xvdbstore_t * db) : m_root(root) {
    m_sched = state_mpt::new_state_sync(table, root, db);
    m_db = std::make_shared<state_mpt::xstate_mpt_db_t>(db, table);
}

void xtop_state_sync::run_state_sync() {
    //  TODO: timeout
    std::thread run_th(&xtop_state_sync::run, this);

    while (!m_done) {
        if (m_res_list.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        auto res = m_res_list.front();
        if (!m_active_req.count(res.id)) {
            xwarn("xtop_state_sync::run_state_sync unrequested id: %lu", res.id);
            continue;
        }
        auto req = m_active_req[res.id];
        req.response = res.states;
        req.delivered = base::xtime_utl::gettimeofday();

        m_deliver_list.push_back(req);
        m_active_req.erase(res.id);
        m_res_list.pop_front();
    }

    cancel();
    run_th.join();
    return;
}

void xtop_state_sync::run() {
    loop(m_ec);
    m_done = true;
}

void xtop_state_sync::wait() {
    while (!m_done) {}
    return;
}

void xtop_state_sync::cancel() {
    m_cancel = true;
    wait();
}

void xtop_state_sync::loop(std::error_code & ec) {
    m_started = true;

    while (m_sched->Pending() > 0) {
        commit(false, ec);
        if (ec) {
            xwarn("xtop_state_sync::loop commit error: %s %s", ec.category().name(), ec.message().c_str());
            return;
        }

        assign_tasks();

        if (m_cancel) {
            ec = error::xerrc_t::state_sync_cancel;
            return;
        }

        if (m_deliver_list.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        auto req = m_deliver_list.front();
        process(req, ec);
        if (ec) {
            xwarn("xtop_state_sync::loop process error: %s %s", ec.category().name(), ec.message().c_str());
            return;
        }
        m_deliver_list.pop_front();
    }

    commit(true, ec);
    if (ec) {
        xwarn("xtop_state_sync::loop commit error: %s %s", ec.category().name(), ec.message().c_str());
        return;
    }
    return;
}

void xtop_state_sync::commit(bool force, std::error_code & ec) {
    if (!force && m_bytes_uncommitted < ideal_batch_size) {
        return;
    }
    m_sched->Commit(m_db);
	m_num_uncommitted = 0;
	m_bytes_uncommitted = 0;
    return;
}

void xtop_state_sync::assign_tasks() {
    state_req req;
    std::vector<xhash256_t> nodes;
    std::vector<xhash256_t> units;
    fill_tasks(fetch_num, req, nodes, units);
    if (nodes.size() + units.size() == 0) {
        return;
    }
    if (m_cancel) {
        return;
    }
    m_active_req[m_req_sequence_id++] = req;
    // TODO: fetch data
}

void xtop_state_sync::fill_tasks(uint32_t n, state_req & req, std::vector<xhash256_t> & nodes_out, std::vector<xhash256_t> & units_out) {
    if (n > m_trie_tasks.size() + m_unit_tasks.size()) {
        auto fill = n - m_trie_tasks.size() - m_unit_tasks.size();
        auto res = m_sched->Missing(fill);
        auto nodes = std::get<0>(res);
        auto paths = std::get<1>(res);
        auto units = std::get<2>(res);
        for (size_t i = 0; i < nodes.size(); i++) {
            m_trie_tasks.insert({nodes[i], {paths[i]}});
        }
        for (size_t i = 0; i < units.size(); i++) {
            m_unit_tasks.insert({units[i], {}});
        }
    }
    for (auto it = m_trie_tasks.begin(); it != m_trie_tasks.end(); it++) {
        if (nodes_out.size() + units_out.size() >= n) {
            break;
        }
        nodes_out.push_back(it->first);
        req.trie_tasks.insert(*it);
        m_trie_tasks.erase(it++);
    }
    for (auto it = m_unit_tasks.begin(); it != m_unit_tasks.end(); it++) {
        if (nodes_out.size() + units_out.size() >= n) {
            break;
        }
        units_out.push_back(it->first);
        req.unit_tasks.insert(*it);
        m_unit_tasks.erase(it++);
    }
    req.n_items = nodes_out.size() + units_out.size();
    return;
}

void xtop_state_sync::process(state_req & req, std::error_code & ec) {
    for (auto blob : req.response) {
        auto hash = process_node_data(blob, ec);
        if (ec) {
            if (ec != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_not_requested) && 
                ec != evm_common::error::make_error_code(evm_common::error::xerrc_t::trie_sync_already_processed)) {
                xwarn("xtop_state_sync::process invalid state node: %s, %s %s", hash.as_hex_str().c_str(), ec.category().name(), ec.message().c_str());
                return;
            }
            xwarn("xtop_state_sync::process process_node_data abnormal: %s, %s %s", hash.as_hex_str().c_str(), ec.category().name(), ec.message().c_str());
        }
        req.trie_tasks.erase(hash);
        req.unit_tasks.erase(hash);
    }
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
    return res.Hash;
}

}  // namespace state_sync
}  // namespace top