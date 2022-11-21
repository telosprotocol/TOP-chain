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
                                                                           std::function<sync_peers(const common::xtable_id_t & id)> peers,
                                                                           std::function<void(const state_req &)> track_req,
                                                                           base::xvdbstore_t * db,
                                                                           statestore::xstatestore_face_t * store) {
    auto sync = std::make_shared<xtop_unit_state_sync>();
    sync->m_account = account;
    sync->m_index = index;
    auto height = index.get_latest_unit_height();
    auto block_hash = index.get_latest_unit_hash();
    auto state_hash = index.get_latest_state_hash();
    sync->m_peers_func = peers;
    sync->m_track_func = track_req;
    sync->m_db = db;
    sync->m_store = store;
    sync->m_symbol = "unit: " + account.to_string() + ", height: " + std::to_string(height) + ", block_hash: " + index.get_latest_unit_hash();
    xinfo("xtop_unit_state_sync::new_state_sync unit: %s, height: %lu, root: %s", account.to_string().c_str(), height, to_hex(block_hash).c_str());
    return sync;
}

void xtop_unit_state_sync::run() {
    sync_unit(m_ec);
    if (m_ec) {
        xwarn("xtop_unit_state_sync::run sync_unit error, unit: %s, index: %s, error: %s, %s",
              m_account.to_string().c_str(),
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
    return m_symbol;
}

sync_result xtop_unit_state_sync::result() const {
    return {m_account, m_index.get_latest_unit_height(), evm_common::xh256_t(to_bytes(m_index.get_latest_unit_hash())), evm_common::xh256_t(to_bytes(m_index.get_latest_state_hash())), {}, m_ec};
}

bool xtop_unit_state_sync::is_done() const {
    return m_done;
}

void xtop_unit_state_sync::deliver_req(const state_req & req) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_deliver_list.push_back(req);
    }
    m_condition.notify_one();
}

void xtop_unit_state_sync::sync_unit(std::error_code & ec) {
    // check exist
    auto unitstate = m_store->get_unit_state_by_accountindex(m_account, m_index);
    if (unitstate != nullptr) {
        xinfo("xtop_unit_state_sync::sync_unit state already exist in db, {%s}", symbol().c_str());
        m_done = true;
        return;
    }
    xinfo("xtop_state_sync::sync_table {%s}", symbol().c_str());
    auto condition = [this]() -> bool { return !m_sync_unit_finish; };
    auto add_task = [this](sync_peers const & peers) { return assign_unit_tasks(peers); };
    auto process_task = [this](state_req & req, std::error_code & ec) { return process_unit(req, ec); };
    loop(condition, add_task, process_task, ec);
    if (ec) {
        xwarn("xtop_state_sync::sync_table loop error: %s %s, exit, {%s}", ec.category().name(), ec.message().c_str(), symbol().c_str());
        return;
    }
    return;
}

void xtop_unit_state_sync::assign_unit_tasks(const sync_peers & peers) {
    base::xstream_t stream(base::xcontext_t::instance());
    state_mpt::xaccount_info_t info;
    info.m_account = m_account;
    info.m_index = m_index;
    stream << info.encode();
    stream << rand();  // defend from filter
    xbytes_t data{stream.data(), stream.data() + stream.size()};
    xinfo("xtop_unit_state_sync::assign_unit_tasks %s, id: %u, start sync", symbol().c_str(), m_req_sequence_id);
    state_req req;
    req.peer = send_message(peers, {stream.data(), stream.data() + stream.size()}, xmessage_id_sync_unit_request);
    req.start = base::xtime_utl::time_now_ms();
    req.id = m_req_sequence_id++;
    req.type = state_req_type::enum_state_req_unit;
    m_track_func(req);
}

void xtop_unit_state_sync::process_unit(state_req & req, std::error_code & ec) {
    if (req.type != state_req_type::enum_state_req_unit) {
        return;
    }
    if (m_sync_unit_finish) {
        return;
    }
    if (req.units_response.empty()) {
        xwarn("xtop_state_sync::process_unit empty, %s", symbol().c_str());
        return;
    }
    auto & data = req.units_response.at(0);
    xinfo("xtop_state_sync::process_unit value size: %zu, {%s}", data.size(), symbol().c_str());
    {
        // check state
        auto bstate = base::xvblock_t::create_state_object({data.begin(), data.end()});
        auto const table_state = std::make_shared<data::xtable_bstate_t>(bstate);
        auto const snapshot = table_state->take_snapshot();
        auto const hash = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
        if (hash != m_index.get_latest_state_hash()) {
            xwarn("xtop_unit_state_sync::process_unit state hash mismatch: %s, %s", to_hex(hash).c_str(), to_hex(m_index.get_latest_state_hash()).c_str());
            return;
        }
    }
    auto const key = base::xvdbkey_t::create_prunable_unit_state_key(m_account.to_string(), m_index.get_latest_unit_height(), {data.begin(), data.end()});
    m_db->set_value(key, {data.begin(), data.end()});
    m_sync_unit_finish = true;
}

void xtop_unit_state_sync::loop(std::function<bool()> condition,
                                std::function<void(sync_peers const &)> add_task,
                                std::function<void(state_req &, std::error_code &)> process_task,
                                std::error_code & ec) {
    auto net = m_peers_func(m_account.table_id());
    if (net.network == nullptr) {
        xwarn("xtop_unit_state_sync::loop no network availble, exit, {%s}", symbol().c_str());
        ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    while (condition()) {
        xdbg("xtop_unit_state_sync::loop unit finish: %d, {%s}", m_sync_unit_finish.load(), symbol().c_str());
        // step2: check peers
        if (net.peers.empty()) {
            xwarn("xtop_unit_state_sync::loop peers empty, exit, {%s}", symbol().c_str());
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
                m_deliver_list.pop_front();
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

common::xnode_address_t xtop_unit_state_sync::send_message(const sync_peers & peers, const xbytes_t & msg, common::xmessage_id_t id) {
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

}  // namespace state_sync
}  // namespace top