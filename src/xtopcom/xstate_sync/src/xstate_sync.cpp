// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xstate_sync.h"

#include "xcodec/xmsgpack_codec.hpp"
#include "xevm_common/xerror/xerror.h"
#include "xpbase/base/top_utils.h"
#include "xstate_mpt/xstate_sync.h"
#include "xstate_sync/xerror.h"
#include "xutility/xhash.h"
#include "xvnetwork/xvnetwork_message.h"

namespace top {
namespace state_sync {

constexpr uint32_t ideal_batch_size = 100 * 1024;
constexpr uint32_t fetch_num = 128;

void xtop_state_sync::run_state_sync(const std::string & table, const xhash256_t & root, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, base::xvdbstore_t * db) {
    if (m_running) {
        m_ec = error::xerrc_t::downloader_is_running;
        return;
    }
    m_running = true;

    clear();

    m_table = table;
    m_root = root;
    m_sched = state_mpt::new_state_sync(table, root, db);
    m_db = std::make_shared<state_mpt::xstate_mpt_db_t>(db, table);
    xinfo("xtop_state_sync::run_state_sync running state sync, table: %s, root: %s, network: %s", table.c_str(), root.as_hex_str().c_str(), network->address().to_string().c_str());
    std::thread run_th(&xtop_state_sync::run, this, network);

    while (!m_done) {
        if (!m_res_list.empty()) {
            auto res = m_res_list.front();
            xinfo("xtop_state_sync::run_state_sync get res, id: %u", res.id);
            if (!m_active_req.count(res.id)) {
                xwarn("xtop_state_sync::run_state_sync unrequested id: %u", res.id);
                continue;
            }
            auto req = m_active_req[res.id];
            req.response = res.states;
            req.delivered = base::xtime_utl::gettimeofday();

            m_deliver_list.push_back(req);
            m_active_req.erase(res.id);
            m_res_list.pop_front();
        } else if (!m_active_req.empty()) {
            uint64_t time = base::xtime_utl::time_now_ms();
            for (auto it = m_active_req.begin(); it != m_active_req.end();) {
                if (it->second.start + 5000 > time) {
                    break;
                }
                xwarn("xtop_state_sync::run_state_sync timeout %lu, %lu", it->second.start, time);
                it->second.delivered = base::xtime_utl::gettimeofday();
                m_deliver_list.push_back(it->second);
                m_active_req.erase(it++);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
    }

    cancel();
    run_th.join();
    xinfo("xtop_state_sync::run_state_sync sync thread finish: %s", network->address().to_string().c_str());
    m_running = false;
    return;
}

void xtop_state_sync::clear() {
    m_started = false;
    m_done = false;
    m_cancel = false;
    m_trie_tasks.clear();
    m_unit_tasks.clear();
    m_num_uncommitted = 0;
    m_bytes_uncommitted = 0;
    m_active_req.clear();
    m_req_sequence_id = 0;
    m_res_list.clear();
    m_deliver_list.clear();
    m_ec.clear();
}

void xtop_state_sync::run(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network) {
    loop(network, m_ec);
    m_done = true;
}

void xtop_state_sync::run2() {
    loop(m_network_ptr, m_ec);
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

xhash256_t xtop_state_sync::root() const {
    return m_root;
}

bool xtop_state_sync::is_done() const {
    return m_done;
}

void xtop_state_sync::push_deliver_req(const state_req & req) {
    m_deliver_list.push_back(req);
}

void xtop_state_sync::loop(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, std::error_code & ec) {
    xinfo("xtop_state_sync::loop main loop");
    m_started = true;

    while (m_sched->Pending() > 0) {
        commit(false, ec);
        if (ec) {
            xwarn("xtop_state_sync::loop commit error: %s %s", ec.category().name(), ec.message().c_str());
            return;
        }

        assign_tasks(network);

        if (m_cancel) {
            if (m_ec) {
                ec = m_ec;
            } else {
                ec = error::xerrc_t::state_sync_cancel;
            }
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

void xtop_state_sync::assign_tasks(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network) {
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
    base::xstream_t stream(base::xcontext_t::instance());
    stream << m_table;
    stream << m_req_sequence_id;
    std::vector<xbytes_t> nodes_bytes;
    for (auto hash : nodes) {
        nodes_bytes.emplace_back(hash.to_bytes());
        xdbg("xtop_state_sync::assign_tasks %s", hash.as_hex_str().c_str());
    }
    stream << nodes_bytes;
    
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_request);
    auto archive_list = network->archive_addresses(common::xnode_type_t::storage_archive);
    if (archive_list.empty()) {
        m_ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    if (archive_list.size() == 1 && archive_list[0] == network->address()) {
        m_ec = error::xerrc_t::state_network_invalid;
        m_cancel = true;
        return;
    }
    common::xnode_address_t random_archive;
    for (;;) {
        random_archive = archive_list.at(RandomUint32() % archive_list.size());
        if (random_archive.account_address() != network->address().account_address()) {
            break;
        }
    }

    std::error_code ec;
    network->send_to(random_archive, _msg, ec);
    if (ec) {
        xwarn("xtop_state_sync::assign_tasks net error, %s, %s", random_archive.account_address().c_str(), network->address().account_address().c_str());
    }
    req.start = base::xtime_utl::time_now_ms();
    m_active_req[m_req_sequence_id++] = req;
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
    for (auto it = m_trie_tasks.begin(); it != m_trie_tasks.end(); ) {
        if (nodes_out.size() + units_out.size() >= n) {
            break;
        }
        nodes_out.push_back(it->first);
        req.trie_tasks.insert(*it);
        m_trie_tasks.erase(it++);
    }
    for (auto it = m_unit_tasks.begin(); it != m_unit_tasks.end(); ) {
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
        xinfo("xtop_state_sync::process blob: %s", to_hex(blob).c_str());
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
    // retry queue
    for (auto pair : req.trie_tasks) {
        xinfo("m_trie_tasks retry queue");
        m_trie_tasks.insert(pair);
    }
    for (auto pair : req.unit_tasks) {
        xinfo("m_unit_tasks retry queue");
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

void xtop_state_sync::deliver_node_data(top::vnetwork::xvnode_address_t const & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, top::vnetwork::xmessage_t const & message, base::xvdbstore_t * db) {
    base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
    if (message.id() == xmessage_id_sync_request) {
        std::string table;
        uint32_t id;
        std::vector<xbytes_t> hashes;
        std::vector<xbytes_t> values;
        stream >> table;
        stream >> id;
        stream >> hashes;

        state_mpt::xstate_mpt_db_t state_db{db, table};
        for (auto hash : hashes) {
            std::error_code ec;
            auto v = state_db.Get(hash, ec);
            if (v.empty() || ec) {
                xwarn("xtop_state_sync::deliver_node_data not found %s", to_hex(hash).c_str());
                continue;
            }
            xinfo("xtop_state_sync::deliver_node_data request hash: %s, data: %s", to_hex(hash).c_str(), to_hex(v).c_str());
            values.push_back(v);
        }
        base::xstream_t stream_back{top::base::xcontext_t::instance()};
        stream_back << table;
        stream_back << id;
        stream_back << values;
        vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream_back.data(), stream_back.data() + stream_back.size()}, xmessage_id_sync_response);
        std::error_code ec;
        network->send_to(sender, _msg, ec);
        if (ec) {
            xwarn("xtop_state_sync::deliver_node_data net error, %s", network->address().to_string().c_str());
        }
    } else if (message.id() == xmessage_id_sync_response) {
        state_res res;
        stream >> res.table;
        stream >> res.id;
        stream >> res.states;
        m_res_list.push_back(res);
        xinfo("xtop_state_sync::deliver_node_data response size: %zu", res.states.size());
    } else {
        xerror("unknown id: %lu", static_cast<uint32_t>(message.id()));
    }
}

std::shared_ptr<xtop_state_sync> xtop_state_sync::new_state_sync(const std::string & table,
                                                                 const xhash256_t & root,
                                                                 std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                                                 std::shared_ptr<std::list<state_req>> track_req,
                                                                 base::xvdbstore_t * db) {

    auto sync = std::make_shared<xtop_state_sync>();
    sync->m_sched = state_mpt::new_state_sync(table, root, db);
    sync->m_table = table;
    sync->m_root = root;
    sync->m_network_ptr = network;
    sync->m_track_list_ptr = track_req;
    sync->m_db = std::make_shared<state_mpt::xstate_mpt_db_t>(db, table);
    xinfo("xtop_state_sync::new_state_sync table: %s, root: %s, network: %s", table.c_str(), root.as_hex_str().c_str(), network->address().to_string().c_str());
    return sync;
}

}  // namespace state_sync
}  // namespace top