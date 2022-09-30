// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xstate_downloader.h"

#include "xmbus/xevent_state_sync.h"
#include "xstate_sync/xerror.h"
#include "xstate_sync/xstate_sync.h"
#include "xstate_sync/xunit_state_sync.h"

namespace top {
namespace state_sync {

#define TIMEOUT_MSEC 5000U

xtop_state_downloader::xtop_state_downloader(base::xvdbstore_t * db, statestore::xstatestore_face_t * store, const observer_ptr<mbus::xmessage_bus_face_t> & msg_bus)
  : m_db(db), m_store(store), m_bus(msg_bus) {
}

bool xtop_state_downloader::is_syncing(const common::xaccount_address_t & table) {
    std::lock_guard<std::mutex> lock(m_dispatch_mutex);
    if (m_running.count(table)) {
        return true;
    }
    return false;
}

void xtop_state_downloader::sync_state(const common::xaccount_address_t & table,
                                       const uint64_t height,
                                       const xhash256_t & block_hash,
                                       const xhash256_t & state_hash,
                                       const xhash256_t & root_hash,
                                       bool sync_unit,
                                       std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_table_dispatch);
    if (m_running_tables.count(table)) {
        ec = error::xerrc_t::downloader_is_running;
        return;
    }
    auto executer = std::make_shared<xtop_download_executer>();
    auto peers_func = std::bind(&xtop_state_downloader::get_peers, this);
    auto track_func = std::bind(&xtop_download_executer::push_track_req, executer, std::placeholders::_1);
    auto syncer = xstate_sync_t::new_state_sync(table, height, block_hash, state_hash, root_hash, peers_func, track_func, m_db, sync_unit);
    auto finish = std::bind(&xtop_state_downloader::process_finish, this, std::placeholders::_1);
    std::thread th(&xtop_download_executer::run_state_sync, executer, syncer, finish);
    th.detach();
    m_running_tables.emplace(table, executer);
    xinfo("xtop_state_downloader::sync_state table: %s, height: %lu, root: %s add to running task", table.c_str(), height, root_hash.as_hex_str().c_str());
}

void xtop_state_downloader::process_finish(const sync_result & res) {
    std::lock_guard<std::mutex> lock(m_table_dispatch);
    if (m_running_tables.count(res.account)) {
        m_running_tables.erase(res.account);
        if (res.ec) {
            xwarn("xtop_state_downloader::sync_finish table: %s, height: %lu, root: %s, error: %s, %s, out from running task",
                  res.account.c_str(),
                  res.height,
                  res.root_hash.as_hex_str().c_str(),
                  res.ec.category().name(),
                  res.ec.message().c_str());
        } else {
            xinfo("xtop_state_downloader::sync_finish table: %s, height: %lu, root: %s out from running task", res.account.c_str(), res.height, res.root_hash.as_hex_str().c_str());
        }
        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_state_sync_t>(res.account, res.height, res.block_hash, res.state_hash, res.root_hash, res.ec);
        m_bus->push_event(ev);
    } else {
        xwarn("xtop_state_downloader::sync_finish table: %s, height: %lu, root: %s not found in running task", res.account.c_str(), res.height, res.root_hash.as_hex_str().c_str());
    }
}

void xtop_state_downloader::sync_cancel(const common::xaccount_address_t & table) {
    std::lock_guard<std::mutex> lock(m_table_dispatch);
    if (m_running_tables.count(table)) {
        m_running_tables[table]->cancel();
    }
    xinfo("xtop_state_downloader::sync_cancel cancel table: %s sync task", table.c_str());
}

void xtop_state_downloader::sync_unit_state(const common::xaccount_address_t & account, const base::xaccount_index_t & index, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_unit_dispatch);
    if (m_running_units.count(account)) {
        ec = error::xerrc_t::downloader_is_running;
        return;
    }
    auto executer = std::make_shared<xtop_download_executer>();
    auto peers_func = std::bind(&xtop_state_downloader::get_peers, this);
    auto track_func = std::bind(&xtop_download_executer::push_track_req, executer, std::placeholders::_1);
    auto syncer = xunit_state_sync_t::new_state_sync(account, index, peers_func, m_db, m_store);
    auto finish = std::bind(&xtop_state_downloader::process_unit_finish, this, std::placeholders::_1);
    std::thread th(&xtop_download_executer::run_state_sync, executer, syncer, finish);
    th.detach();
    m_running_units.emplace(account, executer);
    xinfo("xtop_state_downloader::sync_unit_state account: %s, height: %lu, index: %s generate task", account.c_str(), index.dump().c_str());
}

void xtop_state_downloader::process_unit_finish(const sync_result & res) {
    std::lock_guard<std::mutex> lock(m_unit_dispatch);
    if (m_running_units.count(res.account)) {
        m_running_units.erase(res.account);
        if (res.ec) {
            xwarn("xtop_state_downloader::process_unit_finish unit: %s, height: %lu, unit_hash: %s, error: %s, %s, out from running task",
                  res.account.c_str(),
                  res.height,
                  res.block_hash.as_hex_str().c_str(),
                  res.ec.category().name(),
                  res.ec.message().c_str());
        } else {
            xinfo("xtop_state_downloader::process_unit_finish unit: %s, height: %lu, unit_hash: %s out from running task",
                  res.account.c_str(),
                  res.height,
                  res.block_hash.as_hex_str().c_str());
        }
        // mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_state_sync_t>(res.account, res.height, res.block_hash, res.state_hash, res.root_hash, res.ec);
        // m_bus->push_event(ev);
    } else {
        xwarn("xtop_state_downloader::process_unit_finish unit: %s, height: %lu, unit_hash: %s not found in running task",
              res.account.c_str(),
              res.height,
              res.block_hash.as_hex_str().c_str());
    }
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
    } else if (message.id() == xmessage_id_sync_unit_request) {
        process_unit_request(sender, network, message);
    } else if (message.id() == xmessage_id_sync_unit_response) {
        process_unit_response(message);
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
            xwarn("xtop_state_downloader::process_request node request error: %s %s, table: %s, id: %u, node %s",
                  ec.category().name(),
                  ec.message().c_str(),
                  table.c_str(),
                  id,
                  to_hex(hash).c_str());
            continue;
        }
        xinfo("xtop_state_downloader::process_request node request, table: %s, id: %u, hash: %s, data: %s", table.c_str(), id, to_hex(hash).c_str(), to_hex(v).c_str());
        nodes_values.push_back(v);
    }
    for (auto hash : units_hashes) {
        state_mpt::xaccount_info_t info;
        info.decode({hash.begin(), hash.end()});
        // auto v = evm_common::trie::ReadUnitWithPrefix(kv_db, xhash256_t(hash));
        auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_accountindex(info.m_account, info.m_index);
        if (unitstate == nullptr) {
            xwarn("xtop_state_downloader::process_request unit request not found, table: %s, id: %u, hash: %s", table.c_str(), id, to_hex(hash).c_str());
            continue;
        }
        std::string unit_state_str;
        unitstate->get_bstate()->serialize_to_string(unit_state_str);
        if (unit_state_str.empty()) {
            xwarn("xtop_state_downloader::process_request unit request not found, table: %s, id: %u, hash: %s", table.c_str(), id, to_hex(hash).c_str());
            continue;
        }
        xinfo("xtop_state_downloader::process_request unit request, table: %s, id: %u, hash: %s, data: %s", table.c_str(), id, to_hex(hash).c_str(), to_hex(unit_state_str).c_str());
        units_values.push_back({unit_state_str.begin(), unit_state_str.end()});
    }
    base::xstream_t stream_back{top::base::xcontext_t::instance()};
    stream_back << table;
    stream_back << id;
    stream_back << nodes_values;
    stream_back << units_values;
    xinfo("xtop_state_downloader::process_request success table: %s, id: %u, size: %zu, %zu", table.c_str(), id, nodes_values.size(), units_values.size());
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_node_response);
    std::error_code ec;
    network->send_to(sender, _msg, ec);
    if (ec) {
        xwarn("xtop_state_downloader::process_request network error %s %s, table: %s, id: %u, %s->%s",
              ec.category().name(),
              ec.message().c_str(),
              table.c_str(),
              id,
              network->address().to_string().c_str(),
              sender.to_string().c_str());
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
    std::lock_guard<std::mutex> lock(m_table_dispatch);
    if (m_running_tables.count(res.table)) {
        auto executer = m_running_tables[res.table];
        executer->push_state_pack(res);
    }
    xinfo("xtop_state_downloader::process_response table: %s, id: %u, size: %zu, %zu", res.table.c_str(), res.id, res.nodes.size(), res.units.size());
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
    auto v = m_db->get_value(state_key);
    if (v.empty()) {
        xwarn("xtop_state_downloader::process_table_request empty, table %s, height: %lu", table.c_str(), height, to_hex(hash).c_str());
    }
    base::xstream_t stream_back{top::base::xcontext_t::instance()};
    stream_back << table;
    stream_back << height;
    stream_back << hash;
    stream_back << xbytes_t{v.begin(), v.end()};
    xinfo("xtop_state_downloader::process_table_request table: %s, height: %lu, hash: %s, key: %s, value: %s",
          table.c_str(),
          height,
          to_hex(hash).c_str(),
          to_hex(state_key).c_str(),
          to_hex(v).c_str());
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_table_response);
    std::error_code ec;
    network->send_to(sender, _msg, ec);
    if (ec) {
        xwarn("xtop_state_downloader::process_table_request network error %s %s, table: %s, height: %u, hash: %s, %s->%s",
              ec.category().name(),
              ec.message().c_str(),
              table.c_str(),
              height,
              to_hex(hash).c_str(),
              network->address().to_string().c_str(),
              sender.to_string().c_str());
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
    xinfo("xtop_state_downloader::process_table_response table: %s, height: %lu, hash: %s, value: %s", table.c_str(), height, to_hex(hash).c_str(), to_hex(value).c_str());
    auto state_key = base::xvdbkey_t::create_prunable_state_key(base::xvaccount_t{table}, height, {hash.begin(), hash.end()});
    if (value.empty()) {
        xwarn("xtop_state_downloader::process_table_response empty, table %s, height: %lu, hash: %s", table.c_str(), height, to_hex(hash).c_str());
        return;
    }
    auto table_account = common::xaccount_address_t{table};
    std::lock_guard<std::mutex> lock(m_table_dispatch);
    if (m_running_tables.count(table_account)) {
        auto executer = m_running_tables[table_account];
        executer->push_single_state({table_account, height, xhash256_t(hash), value});
    }
}

void xtop_state_downloader::process_unit_request(const vnetwork::xvnode_address_t & sender,
                                                 std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                                 const vnetwork::xmessage_t & message) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
    std::string info_str;
    state_mpt::xaccount_info_t info;
    info.decode({info_str.begin(), info_str.end()});

    std::string state_str;
    auto unitstate = m_store->get_unit_state_by_accountindex(info.m_account, info.m_index);
    if (unitstate == nullptr) {
        xwarn("xtop_state_downloader::process_unit_request unit request not found, account: %s, index: %s", info.m_account.c_str(), info.m_index.dump().c_str());
    } else {
        unitstate->get_bstate()->serialize_to_string(state_str);
        if (state_str.empty()) {
            xwarn("xtop_state_downloader::process_request unit empty, account: %s, index: %s", info.m_account.c_str(), info.m_index.dump().c_str());
        }
    }

    base::xstream_t stream_back{top::base::xcontext_t::instance()};
    stream_back << info.m_account.value();
    stream_back << state_str;

    xinfo("xtop_state_downloader::process_request unit request, account: %s, index: %s, state_str: %s",
          info.m_account.c_str(),
          info.m_index.dump().c_str(),
          to_hex(state_str).c_str());

    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_unit_response);
    std::error_code ec;
    network->send_to(sender, _msg, ec);
    if (ec) {
        xwarn("xtop_state_downloader::process_request network error %s %s, account: %s, index: %s, %s->%s",
              ec.category().name(),
              ec.message().c_str(),
              info.m_account.c_str(),
              info.m_index.dump().c_str(),
              network->address().to_string().c_str(),
              sender.to_string().c_str());
    }
}

void xtop_state_downloader::process_unit_response(const vnetwork::xmessage_t & message) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
    std::string account;
    xbytes_t value;
    stream >> account;
    stream >> value;
    xinfo("xtop_state_downloader::process_unit_response unit: %s, value: %s", account.c_str(), to_hex(value).c_str());
    if (value.empty()) {
        xwarn("xtop_state_downloader::process_unit_response empty, account %s", account.c_str());
        return;
    }
    auto unit_account = common::xaccount_address_t{account};
    std::lock_guard<std::mutex> lock(m_unit_dispatch);
    if (m_running_units.count(unit_account)) {
        auto executer = m_running_units[unit_account];
        executer->push_single_state({unit_account, 0, {}, value});
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

void xtop_download_executer::run_state_sync(std::shared_ptr<xstate_sync_face_t> syncer, std::function<void(sync_result)> callback) {
    xinfo("xtop_download_executer::run_state_sync sync thread start, %s", syncer->symbol().c_str());

    std::thread run_th(&xstate_sync_face_t::run, syncer);
    run_th.detach();

    std::error_code loop_ec;
    loop(syncer, loop_ec);

    auto done = syncer->is_done();
    auto res = syncer->result();
    syncer->cancel();
    if (done) {
        if (res.ec) {
            xwarn("xtop_download_executer::run_state_sync sync thread finish but error, %s, error: %s, %s",
                  syncer->symbol().c_str(),
                  res.ec.category().name(),
                  res.ec.message().c_str());
        } else {
            xinfo("xtop_download_executer::run_state_sync sync thread finish, %s", syncer->symbol().c_str());
        }
    } else {
        if (!res.ec && !loop_ec) {
            xwarn("xtop_download_executer::run_state_sync not finish but no error code");
            xassert(false);
        }
        if (loop_ec) {
            xwarn("xtop_download_executer::run_state_sync origin error: %s %s, replace by loop error: %s %s",
                  res.ec.category().name(),
                  res.ec.message().c_str(),
                  loop_ec.category().name(),
                  loop_ec.message().c_str());
            res.ec = loop_ec;
        }
        xwarn("xtop_download_executer::run_state_sync thread overtime, maybe error, account: %s, height: %lu, root: %s, error: %s %s",
              res.account.c_str(),
              res.height,
              res.root_hash.as_hex_str().c_str(),
              res.ec.category().name(),
              res.ec.message().c_str());
    }
    if (callback) {
        callback(res);
    }
    return;
}

void xtop_download_executer::loop(std::shared_ptr<xstate_sync_face_t> syncer, std::error_code & ec) {
    std::map<uint32_t, state_req, std::less<uint32_t>> active;
    int cnt{0};

    while (!syncer->is_done()) {
        if (!m_single_states.empty()) {
            auto detail = m_single_states.front();
            xdbg("xtop_download_executer::loop push single state, account: %s, height: %lu, hash: %s, value: %s",
                 detail.address.c_str(),
                 detail.height,
                 to_hex(detail.hash).c_str(),
                 to_hex(detail.value).c_str());
            syncer->push_deliver_state(detail);
            pop_single_state();
        } else if (!m_track_req.empty()) {
            auto req = m_track_req.front();
            active[req.id] = req;
            xdbg("xtop_download_executer::loop add active, id: %u", req.id);
            pop_track_req();
        } else if (!m_state_packs.empty()) {
            // recv packs
            auto res = m_state_packs.front();
            xdbg("xtop_download_executer::loop get state pack, id: %u", res.id);
            if (!active.count(res.id)) {
                xwarn("xtop_download_executer::loop unrequested id: %u", res.id);
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
                xwarn("xtop_download_executer::loop req: %u timeout %lu, %lu", it->first, it->second.start, time);
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
                xwarn("xtop_download_executer::loop overtime: %s", syncer->symbol().c_str());
                ec = error::xerrc_t::state_sync_overtime;
                break;
            }
            continue;
        }
    }
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

void xtop_download_executer::push_single_state(const single_state_detail & detail) {
    std::lock_guard<std::mutex> lock(m_single_state_mutex);
    m_single_states.emplace_back(detail);
}

void xtop_download_executer::pop_single_state() {
    std::lock_guard<std::mutex> lock(m_single_state_mutex);
    m_single_states.pop_front();
}

}  // namespace state_sync
}  // namespace top