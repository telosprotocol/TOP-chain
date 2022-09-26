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

xtop_state_downloader::xtop_state_downloader(base::xvdbstore_t * db, statestore::xstatestore_face_t * store, const observer_ptr<mbus::xmessage_bus_face_t> & msg_bus)
  : m_db(db), m_store(store), m_bus(msg_bus) {
}

void xtop_state_downloader::sync_state(const common::xaccount_address_t & table,
                                       const uint64_t height,
                                       const xhash256_t & block_hash,
                                       const xhash256_t & state_hash,
                                       const xhash256_t & root_hash,
                                       bool sync_unit,
                                       std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(table)) {
        ec = error::xerrc_t::downloader_is_running;
        return;
    }
    auto executer = std::make_shared<xtop_download_executer>();
    auto peers_func = std::bind(&xtop_state_downloader::get_peers, this);
    auto track_func = std::bind(&xtop_download_executer::push_track_req, executer, std::placeholders::_1);
    auto syncer = xstate_sync_t::new_state_sync(table, height, block_hash, state_hash, root_hash, peers_func, track_func, m_db, sync_unit);
    auto finish = std::bind(&xtop_state_downloader::process_finish, this, std::placeholders::_1);
    std::thread th(&xtop_download_executer::run_state_sync, executer, syncer, finish, sync_unit);
    th.detach();
    m_running.emplace(table, executer);
    xinfo("xtop_state_downloader::sync_state table: %s, root: %s add to running task", table.c_str(), root_hash.as_hex_str().c_str());
}

void xtop_state_downloader::process_finish(const sync_result & res) {
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(res.table)) {
        m_running.erase(res.table);
        xinfo("xtop_state_downloader::sync_finish table: %s, root: %s out from running task", res.table.c_str(), res.root_hash.as_hex_str().c_str());
        if (res.ec) {
            xwarn("xtop_state_downloader::sync_finish error: %s, %s", res.ec.category().name(), res.ec.message().c_str());
        }
        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_state_sync_t>(res.table, res.height, res.block_hash, res.state_hash, res.root_hash, res.ec);
        m_bus->push_event(ev);
    } else {
        xwarn("xtop_state_downloader::sync_finish table: %s, root: %s not found in running task", res.table.c_str(), res.root_hash.as_hex_str().c_str());
    }
}

void xtop_state_downloader::sync_cancel(const common::xaccount_address_t & table) {
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(table)) {
        m_running[table]->cancel();
    }
    xinfo("xtop_state_downloader::sync_cancel cancel table: %s sync task", table.c_str());
}

void xtop_state_downloader::handle_message(const vnetwork::xvnode_address_t & sender,
                                           std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                           const vnetwork::xmessage_t & message) {
    if (message.id() == xmessage_id_sync_node_request) {
        process_request(sender, network, message);
    } else if (message.id() == xmessage_id_sync_node_response) {
        process_response(message);
    } else if (message.id() == xmessage_id_sync_table_request) {
        process_table_request(sender, network, message);
    } else if (message.id() == xmessage_id_sync_table_response) {
        process_table_response(message);
    } else {
        xerror("xtop_state_downloader::handle_message unknown msg id: %lu", static_cast<uint32_t>(message.id()));
    }
}

void xtop_state_downloader::process_request(const vnetwork::xvnode_address_t & sender,
                                            std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                            const vnetwork::xmessage_t & message) {
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
    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, common::xaccount_address_t{table});
    auto trie_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
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
        auto v = evm_common::trie::ReadUnitWithPrefix(kv_db, xhash256_t(hash));
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
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_node_response);
    std::error_code ec;
    network->send_to(sender, _msg, ec);
    if (ec) {
        xwarn("xtop_state_downloader::deliver_node_data net error, %s", network->address().to_string().c_str());
    }
}

void xtop_state_downloader::process_response(const vnetwork::xmessage_t & message) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
    std::string addr;
    state_res res;
    stream >> addr;
    res.table = common::xaccount_address_t{addr};
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

void xtop_state_downloader::process_table_request(const vnetwork::xvnode_address_t & sender,
                                                  std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                                  const vnetwork::xmessage_t & message) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
    std::string table;
    uint64_t height{0};
    xbytes_t hash;
    stream >> table;
    stream >> height;
    stream >> hash;
    auto state_key = base::xvdbkey_t::create_prunable_state_key(base::xvaccount_t{table}, height, {hash.begin(), hash.end()});
    xinfo("xtop_state_downloader::process_table_request %s, %lu, %s, key: %s", table.c_str(), height, to_hex(hash).c_str(), to_hex(state_key).c_str());
    auto v = m_db->get_value(state_key);
    if (v.empty()) {
        xwarn("xtop_state_downloader::process_table_request find table state %s %s empty", table.c_str(), to_hex(hash).c_str());
    }
    base::xstream_t stream_back{top::base::xcontext_t::instance()};
    stream_back << table;
    stream_back << height;
    stream_back << hash;
    stream_back << xbytes_t{v.begin(), v.end()};
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_table_response);
    std::error_code ec;
    network->send_to(sender, _msg, ec);
    if (ec) {
        xwarn("xtop_state_downloader::process_table_request net error, %s", network->address().to_string().c_str());
    }
}

void xtop_state_downloader::process_table_response(const vnetwork::xmessage_t & message) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
    std::string table;
    uint64_t height{0};
    xbytes_t hash;
    xbytes_t value;
    stream >> table;
    stream >> height;
    stream >> hash;
    stream >> value;
    auto state_key = base::xvdbkey_t::create_prunable_state_key(base::xvaccount_t{table}, height, {hash.begin(), hash.end()});
    xinfo("xtop_state_downloader::process_table_response %s, %lu, %s, key: %s", table.c_str(), height, to_hex(hash).c_str(), to_hex(state_key).c_str());
    if (value.empty()) {
        xwarn("xtop_state_downloader::process_table_response empty %s, %lu, %s, key: %s", table.c_str(), height, to_hex(hash).c_str());
        return;
    }
    auto table_account = common::xaccount_address_t{table};
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(table_account)) {
        auto executer = m_running[table_account];
        executer->push_table_state({table_account, height, xhash256_t(hash), value});
    }
}

state_sync_peers_t xtop_state_downloader::get_peers() {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    return m_peers;
}

void xtop_state_downloader::add_peer(const vnetwork::xvnode_address_t & peer, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network) {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    // bind syncer
    network->register_message_ready_notify(xmessage_category_state_sync,
                                           std::bind(&state_sync::xstate_downloader_t::handle_message, this, std::placeholders::_1, network, std::placeholders::_2));
    m_peers.emplace_back(network);
}

void xtop_state_downloader::del_peer(const vnetwork::xvnode_address_t & peer) {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    for (auto it = m_peers.begin(); it != m_peers.end();) {
        if ((*it)->address() == peer) {
            it = m_peers.erase(it);
        } else {
            it++;
        }
    }
}

void xtop_download_executer::run_state_sync(std::shared_ptr<xstate_sync_t> syncer, std::function<void(sync_result)> callback, bool sync_unit) {
    xinfo("xtop_download_executer::run_state_sync sync thread start: %s %s", syncer->table().c_str(), syncer->root().as_hex_str().c_str());

    evm_common::trie::WriteTrieSyncFlag(syncer->db(), syncer->root());
    std::map<uint32_t, state_req, std::less<uint32_t>> active;
    std::thread run_th(&xtop_state_sync::run, syncer);
    run_th.detach();

    int cnt{0};
    std::error_code inner_ec;
    while (!syncer->is_done()) {
        if (!m_table_state.empty()) {
            auto detail = m_table_state.front();
            xdbg("xtop_download_executer::run_state_syn push table state: %s, %s", detail.address.c_str(), to_hex(detail.value).c_str());
            syncer->process_table(detail);
            pop_table_state();
        }
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
            syncer->push_deliver_req(req);
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
                syncer->push_deliver_req(it->second);
                active.erase(it++);
            }
        } else if (m_cancel) {
            syncer->cancel();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            cnt++;
            if (cnt > 10 * 60 * 10) {
                xwarn("xtop_download_executer::run_state_sync overtime: %s, %s", syncer->table().c_str(), syncer->root().as_hex_str().c_str());
                inner_ec = error::xerrc_t::state_sync_overtime;
                break;
            }
            continue;
        }
    }

    if (syncer->is_done()) {
        syncer->cancel();
        if (syncer->error()) {
            xwarn("xtop_download_executer::run_state_sync sync thread %s %s finish but error: %s, %s",
                  syncer->table().c_str(),
                  syncer->root().as_hex_str().c_str(),
                  syncer->error().category().name(),
                  syncer->error().message().c_str());
        } else {
            evm_common::trie::DeleteTrieSyncFlag(syncer->db(), syncer->root());
            xinfo("xtop_download_executer::run_state_sync sync thread finish: %s %s", syncer->table().c_str(), syncer->root().as_hex_str().c_str());
        }
        callback(syncer->result());
    } else {
        xwarn("xtop_download_executer::run_state_sync child thread overtime, maybe error, exist");
        syncer->cancel();
        callback(syncer->result());
    }
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

void xtop_download_executer::push_table_state(const table_state_detail & detail) {
    std::lock_guard<std::mutex> lock(m_table_state_mutex);
    m_table_state.emplace_back(detail);
}

void xtop_download_executer::pop_table_state() {
    std::lock_guard<std::mutex> lock(m_table_state_mutex);
    m_table_state.pop_front();
}

}  // namespace state_sync
}  // namespace top