// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xstate_downloader.h"

#include "xmbus/xevent_state_sync.h"
#include "xstate_sync/xerror.h"
#include "xstate_sync/xstate_sync.h"

namespace top {
namespace state_sync {

#define TIMEOUT_MSEC 1000U

xtop_state_downloader::xtop_state_downloader(base::xvdbstore_t * db, const observer_ptr<mbus::xmessage_bus_face_t> & msg_bus) : m_db(db), m_bus(msg_bus) {
}

void xtop_state_downloader::sync_state(const std::string & table, const xhash256_t & root, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, bool sync_unit, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(table)) {
        ec = error::xerrc_t::downloader_is_running;
        return;
    }
    auto executer = std::make_shared<xtop_download_executer>(m_db);
    auto finish = std::bind(&xtop_state_downloader::process_finish, this, std::placeholders::_1);
    std::thread th(&xtop_download_executer::run_state_sync, executer, table, root, network, finish, sync_unit);
    th.detach();
    m_running.emplace(table, executer);
    xinfo("xtop_state_downloader::sync_state table: %s, root: %s add to running task", table.c_str(), root.as_hex_str().c_str());
}

void xtop_state_downloader::process_finish(const sync_result & res) {
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(res.table)) {
        m_running.erase(res.table);
        xinfo("xtop_state_downloader::sync_finish table: %s, root: %s out from running task", res.table.c_str(), res.root.as_hex_str().c_str());
        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_state_sync_t>(common::xaccount_address_t{res.table}, res.root, xhash256_t{}, res.ec);
        m_bus->push_event(ev);
    } else {
        xwarn("xtop_state_downloader::sync_finish table: %s, root: %s not found in running task", res.table.c_str(), res.root.as_hex_str().c_str());
    }
}

void xtop_state_downloader::sync_cancel(const std::string & table) {
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(table)) {
        m_running[table]->cancel();
    }
    xinfo("xtop_state_downloader::sync_cancel cancel table: %s sync task", table.c_str());
}

void xtop_state_downloader::handle_message(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message) {
    if (message.id() == xmessage_id_sync_request) {
        process_request(sender, network, message);
    } else if (message.id() == xmessage_id_sync_response) {
        process_response(sender, network, message);
    } else {
        xerror("xtop_state_downloader::handle_message unknown msg id: %lu", static_cast<uint32_t>(message.id()));
    }
}

void xtop_state_downloader::process_request(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
    std::string table;
    uint32_t id;
    std::vector<xbytes_t> nodes_hashes;
    std::vector<xbytes_t> nodes_values;
    std::vector<xbytes_t> units_hashes;
    std::vector<xbytes_t> units_values;
    stream >> table;
    stream >> id;
    stream >> nodes_hashes;
    stream >> units_hashes;
    // state_mpt::xstate_mpt_db_t state_db{m_db, table};
    auto mpt_db = std::make_shared<state_mpt::xstate_mpt_db_t>(m_db, table);
    auto trie_db = evm_common::trie::xtrie_db_t::NewDatabase(mpt_db);
    for (auto hash : nodes_hashes) {
        std::error_code ec;
        auto v = trie_db->Node(xhash256_t(hash), ec);
        if (ec || v.empty()) {
            xwarn("xtop_state_downloader::process_request find node %s error: %s %s", to_hex(hash).c_str(), ec.category().name(), ec.message().c_str());
            continue;
        }
        xinfo("xtop_state_downloader::process_request node request hash: %s, data: %s", to_hex(hash).c_str(), to_hex(v).c_str());
        nodes_values.push_back(v);
    }
    for (auto hash : units_hashes) {
        auto v = evm_common::trie::ReadUnitWithPrefix(mpt_db, xhash256_t(hash));
        if (v.empty()) {
            xwarn("xtop_state_downloader::process_request not found", to_hex(hash).c_str());
            continue;
        }
        xinfo("xtop_state_downloader::process_request unit request hash: %s, data: %s", to_hex(hash).c_str(), to_hex(v).c_str());
        units_values.push_back(v);
    }
    base::xstream_t stream_back{top::base::xcontext_t::instance()};
    stream_back << table;
    stream_back << id;
    stream_back << nodes_values;
    stream_back << units_values;
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_response);
    std::error_code ec;
    network->send_to(sender, _msg, ec);
    if (ec) {
        xwarn("xtop_state_downloader::deliver_node_data net error, %s", network->address().to_string().c_str());
    }
}

void xtop_state_downloader::process_response(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
    state_res res;
    stream >> res.table;
    stream >> res.id;
    stream >> res.nodes;
    stream >> res.units;
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(res.table)) {
        auto executer = m_running[res.table];
        executer->push_state_pack(res);
    }
    xinfo("xtop_state_downloader::deliver_data table: %s, id: %u, size: %zu, %zu", res.table.c_str(), res.id, res.nodes.size(), res.units.size());
}

xtop_download_executer::xtop_download_executer(base::xvdbstore_t * db) : m_db(db) {
}

void xtop_download_executer::run_state_sync(const std::string & table, const xhash256_t & root, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, std::function<void(sync_result)> callback, bool sync_unit) {
    auto track_func = std::bind(&xtop_download_executer::push_track_req, this, std::placeholders::_1);
    auto s = xstate_sync_t::new_state_sync(table, root, network, track_func, m_db, sync_unit);
    xinfo("xtop_download_executer::run_state_sync sync thread start: %s", s->root().as_hex_str().c_str());

    std::map<uint32_t, state_req, std::less<uint32_t>> active;
    std::thread run_th(&xtop_state_sync::run, s);

    while (!s->is_done()) {
        if (!m_track_req.empty()) {
            auto req = m_track_req.front();
            active[req.id] = req;
            xdbg("xtop_download_executer::run_state_sync add active, id: %u", req.id);
            pop_track_req();
        } else if (!m_state_packs.empty()) {
            // recv packs
            auto res = m_state_packs.front();
            xdbg("xtop_download_executer::run_state_sync get state pack, id: %u", res.id);
            if (!active.count(res.id)) {
                xwarn("xtop_download_executer::run_state_sync unrequested id: %u", res.id);
                pop_state_pack();
                continue;
            }
            auto req = active[res.id];
            req.nodes_response = res.nodes;
            req.units_response = res.units;
            req.delivered = base::xtime_utl::gettimeofday();
            s->push_deliver_req(req);
            active.erase(res.id);
            pop_state_pack();
        } else if (!active.empty()) {
            // timeout check
            uint64_t time = base::xtime_utl::time_now_ms();
            for (auto it = active.begin(); it != active.end();) {
                if (it->second.start + TIMEOUT_MSEC > time) {
                    break;
                }
                xwarn("xtop_download_executer::run_state_sync req: %u timeout %lu, %lu", it->first, it->second.start, time);
                it->second.delivered = base::xtime_utl::gettimeofday();
                s->push_deliver_req(it->second);
                active.erase(it++);
            }
        }  else if (m_cancel) {
            s->cancel();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
    }

    s->cancel();
    run_th.join();
    xinfo("xtop_download_executer::run_state_sync sync thread finish: %s", s->root().as_hex_str().c_str());

    callback({table, root, s->error()});
    return;
}

void xtop_download_executer::cancel() {
    m_cancel = true;
}

void xtop_download_executer::push_track_req(const state_req & req) {
    std::lock_guard<std::mutex> lock(m_track_mutex);
    m_track_req.emplace_back(req);
}

void xtop_download_executer::pop_track_req() {
    std::lock_guard<std::mutex> lock(m_track_mutex);
    m_track_req.pop_front();
}

void xtop_download_executer::push_state_pack(const state_res & res) {
    std::lock_guard<std::mutex> lock(m_state_pack_mutex);
    m_state_packs.emplace_back(res);
}

void xtop_download_executer::pop_state_pack() {
    std::lock_guard<std::mutex> lock(m_state_pack_mutex);
    m_state_packs.pop_front();
}

}  // namespace state_sync
}  // namespace top