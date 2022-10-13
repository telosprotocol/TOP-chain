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

xtop_state_downloader::xtop_state_downloader(base::xvdbstore_t * db, statestore::xstatestore_face_t * store, const observer_ptr<mbus::xmessage_bus_face_t> & msg_bus)
  : m_db(db)
  , m_store(store), m_executor_thread{make_object_ptr<base::xiothread_t>()}
  , m_syncer_thread{make_object_ptr<base::xiothread_t>()}
  , m_bus(msg_bus) {
}

bool xtop_state_downloader::is_syncing(const common::xaccount_address_t & table) {
    std::lock_guard<std::mutex> lock(m_table_dispatch);
    return m_running_tables.count(table);
}

void xtop_state_downloader::sync_state(const common::xaccount_address_t & table,
                                       const uint64_t height,
                                       const xhash256_t & block_hash,
                                       const xhash256_t & state_hash,
                                       const xhash256_t & root_hash,
                                       bool sync_unit,
                                       std::error_code & ec) {
    if (block_hash.empty() || state_hash.empty() || root_hash.empty()) {
        xwarn("xtop_state_downloader::sync_state table %s param invalid: %s, %s, %s",
              table.c_str(),
              block_hash.as_hex_str().c_str(),
              state_hash.as_hex_str().c_str(),
              root_hash.as_hex_str().c_str());
        ec = error::xerrc_t::downloader_param_invalid;
        return;
    }
    std::lock_guard<std::mutex> lock(m_table_dispatch);
    if (m_running_tables.count(table)) {
        xwarn("xtop_state_downloader::sync_state table %s is running sync", table.c_str());
        ec = error::xerrc_t::downloader_is_running;
        return;
    }
    auto executer = std::make_shared<xtop_download_executer>(make_observer(m_syncer_thread));

    auto peers_func = [this] { return get_peers(); };
    auto track_func = [executer](state_req const & sr) { executer->push_track_req(sr); };
    auto finish = [this](sync_result const & result) { process_finish(result); };
    auto syncer = xstate_sync_t::new_state_sync(table, height, block_hash, state_hash, root_hash, std::move(peers_func), std::move(track_func), m_db, sync_unit);

    auto f = [executer, syncer = std::move(syncer), finish = std::move(finish)](base::xcall_t &, const int32_t, const uint64_t) -> bool {
        executer->run_state_sync(syncer, finish);
        return true;
    };

    base::xcall_t call(f);
    m_executor_thread->send_call(call);
    m_running_tables.emplace(table, executer);

    int64_t total_in{0};
    int64_t total_out{0};
    m_executor_thread->count_calls(total_in, total_out);
    xinfo("xtop_state_downloader::sync_state table: %s, height: %lu, root: %s, add to running task, table queue cnts: %zu, thread in: %ld, thread out: %ld",
          table.c_str(),
          height,
          root_hash.as_hex_str().c_str(),
          m_running_tables.size(),
          total_in,
          total_out);
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

    auto executer = std::make_shared<xtop_download_executer>(make_observer(m_syncer_thread));
    auto peers_func = [this] { return get_peers(); };
    // auto track_func = [executer](state_req const & sr) { executer->push_track_req(sr); };
    auto finish = [this](sync_result const & sr) { process_unit_finish(sr); };
    auto syncer = xunit_state_sync_t::new_state_sync(account, index, peers_func, m_db, m_store);
    auto f = [executer, syncer, finish](base::xcall_t &, const int32_t, const uint64_t) -> bool {
        executer->run_state_sync(syncer, finish);
        return true;
    };
    base::xcall_t call(f);
    m_executor_thread->send_call(call);
    m_running_units.emplace(account, executer);

    int64_t total_in{0};
    int64_t total_out{0};
    m_executor_thread->count_calls(total_in, total_out);
    xinfo("xtop_state_downloader::sync_unit_state %s add to running task, unit queue cnts: %zu, thread in: %ld, thread out: %ld",
          syncer->symbol().c_str(),
          m_running_units.size(),
          total_in,
          total_out);
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
    for (auto const & hash : nodes_hashes) {
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
        xinfo("xtop_state_downloader::process_request unit request, table: %s, id: %u, hash: %s, data size: %zu", table.c_str(), id, to_hex(hash).c_str(), unit_state_str.size());
        units_values.emplace_back(unit_state_str.begin(), unit_state_str.end());
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
    base::xstream_t stream(base::xcontext_t::instance(), const_cast<uint8_t *>(message.payload().data()), (uint32_t)message.payload().size());
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
    base::xstream_t stream(base::xcontext_t::instance(), const_cast<uint8_t *>(message.payload().data()), (uint32_t)message.payload().size());
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
    vnetwork::xmessage_t const _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_table_response);
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
    base::xstream_t stream(base::xcontext_t::instance(), const_cast<uint8_t *>(message.payload().data()), (uint32_t)message.payload().size());
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
        auto & executer = m_running_tables[table_account];
        executer->push_single_state({table_account, height, xhash256_t(hash), value});
    }
}

void xtop_state_downloader::process_unit_request(const vnetwork::xvnode_address_t & sender,
                                                 std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                                 const vnetwork::xmessage_t & message) {
    base::xstream_t stream(base::xcontext_t::instance(), const_cast<uint8_t *>(message.payload().data()), (uint32_t)message.payload().size());
    std::string info_str;
    state_mpt::xaccount_info_t info;
    info.decode({info_str.begin(), info_str.end()});

    std::string state_str;
    auto const unitstate = m_store->get_unit_state_by_accountindex(info.m_account, info.m_index);
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

    vnetwork::xmessage_t const _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_unit_response);
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
    base::xstream_t stream(base::xcontext_t::instance(), const_cast<uint8_t *>(message.payload().data()), (uint32_t)message.payload().size());
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
        auto & executer = m_running_units[unit_account];
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
            ++it;
        }
    }
}

}  // namespace state_sync
}  // namespace top
