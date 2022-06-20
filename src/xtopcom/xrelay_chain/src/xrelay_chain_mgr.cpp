// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xrelay_chain/xrelay_chain_mgr.h"

#include "xdata/xblockbuild.h"
#include "xdata/xblockextract.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xnative_contract_address.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xmbus/xevent_store.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_manager.h"

NS_BEG2(top, xrelay_chain)

xrelay_chain_resources::xrelay_chain_resources(const observer_ptr<store::xstore_face_t> & store,
                                               const observer_ptr<base::xvblockstore_t> & blockstore,
                                               const observer_ptr<mbus::xmessage_bus_face_t> & bus)
  : m_store(store), m_blockstore(blockstore), m_bus(bus) {
}

store::xstore_face_t * xrelay_chain_resources::get_store() const {
    return m_store.get();
}

base::xvblockstore_t * xrelay_chain_resources::get_vblockstore() const {
    return m_blockstore.get();
}

mbus::xmessage_bus_face_t * xrelay_chain_resources::get_bus() const {
    return m_bus.get();
}

xcross_tx_cache_t::xcross_tx_cache_t(const std::shared_ptr<xrelay_chain_resources> & para) : m_para(para) {
}

void xcross_tx_cache_t::process_block(data::xblock_t * block) {
    xdbg("xcross_tx_cache_t::process_block block=%s,last proc h:%llu,cache low h:%llu,cache up h:%llu,tx num:%u",
         block->dump().c_str(),
         m_last_proc_evm_height,
         m_cached_evm_lower_height,
         m_cached_evm_upper_height,
         m_tx_num);

    auto cross_addr = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_chain_contract_addr_for_eth);
    if (!cross_addr.empty()) {
        base::xvaccount_t _vaccount(block->get_account());
        if (false == m_para->get_vblockstore()->load_block_input(_vaccount, block, metrics::blockstore_access_from_txpool_refresh_table)) {
            xerror("xcross_tx_cache_t::process_block fail-load block input output, block=%s", block->dump().c_str());
            return;
        }
        auto input_actions = data::xblockextract_t::unpack_eth_txactions(block);
        xcross_txs_t cross_txs;
        for (auto & action : input_actions) {
            data::xlightunit_action_t txaction(action);

            if (txaction.get_tx_subtype() != base::enum_transaction_subtype_send) {
                continue;
            }

            data::xtransaction_ptr_t _rawtx = block->query_raw_transaction(txaction.get_tx_hash());
            xdbg("xcross_tx_cache_t::process_block process tx. block:%s,tx:%s", block->dump().c_str(), _rawtx->dump().c_str());

            // todo(nathan): save real cross transaction only.
            // if (_rawtx->get_target_addr() != cross_addr) {
            //     continue;
            // }

            // todo(nathan): just for test here, tobe deleted.
            if (_rawtx->get_tx_version() != data::xtransaction_version_3 ||
                (_rawtx->get_tx_type() != data::xtransaction_type_deploy_evm_contract && _rawtx->get_tx_type() != data::xtransaction_type_run_contract)) {
                continue;
            }
            data::xeth_store_receipt_t evm_result;
            auto ret = txaction.get_evm_transaction_receipt(evm_result);
            if (!ret) {
                xwarn("xcross_tx_cache_t::process_block get evm tx result fail. block:%s,tx:%s", block->dump().c_str(), _rawtx->dump().c_str());
                continue;
            }

            if (evm_result.get_tx_status() == data::enum_ethreceipt_status::ethreceipt_status_successful) {
                xdbg("xcross_tx_cache_t::process_block add cross tx to cache block:%s,tx:%s,last proc h:%llu,cache low h:%llu,cache up h:%llu,tx num:%u",
                     block->dump().c_str(),
                     _rawtx->dump().c_str(),
                     m_last_proc_evm_height,
                     m_cached_evm_lower_height,
                     m_cached_evm_upper_height,
                     m_tx_num);
                cross_txs.m_txs.push_back(_rawtx);
                cross_txs.m_tx_results.push_back(evm_result);
            }
        }
        if (!cross_txs.m_txs.empty()) {
            m_cross_tx_map[block->get_height()] = cross_txs;
            m_tx_num += cross_txs.m_txs.size();
        }
    }

    if (block->get_height() == m_cached_evm_upper_height + 1) {
        m_cached_evm_upper_height++;
    }

    if (block->get_height() + 1 == m_cached_evm_lower_height) {
        m_cached_evm_lower_height--;
    }
}

void xcross_tx_cache_t::on_evm_db_event(data::xblock_t * block) {
    base::xvaccount_t vaccount(sys_contract_eth_table_block_addr_with_suffix);
    if (block->get_height() > m_cached_evm_upper_height) {
        for (uint32_t h = m_cached_evm_upper_height + 1; h < block->get_height(); h++) {
            auto latest_committed_block =
                m_para->get_vblockstore()->load_block_object(vaccount, h, base::enum_xvblock_flag_committed, true, metrics::blockstore_access_from_txpool_refresh_table);
            process_block(dynamic_cast<data::xblock_t *>(latest_committed_block.get()));
        }
    }
}

bool xcross_tx_cache_t::get_tx_cache_leader(uint64_t & upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) const {
    if (m_last_proc_evm_height + 1 != m_cached_evm_lower_height && m_last_proc_evm_height != 0) {
        xinfo("xcross_tx_cache_t::get_tx_cache_leader cache not full.last_proc_evm_height:%llu,cached_evm_lower_height:%llu", m_last_proc_evm_height, m_cached_evm_lower_height);
        return false;
    }
    upper_height = m_cached_evm_upper_height;
    cross_tx_map = m_cross_tx_map;
    return true;
}

bool xcross_tx_cache_t::get_tx_cache_backup(uint64_t upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) const {
    if ((m_last_proc_evm_height + 1 != m_cached_evm_lower_height && m_last_proc_evm_height != 0) || m_cached_evm_upper_height < upper_height) {
        xinfo("xcross_tx_cache_t::get_tx_cache_backup cache not full.last_proc_evm_height:%llu,cached_evm_lower_height:%llu,cached_evm_upper_heigh:%llu,upper_height:%llu",
              m_last_proc_evm_height,
              m_cached_evm_lower_height,
              m_cached_evm_upper_height,
              upper_height);
        return false;
    }

    for (auto & cross_tx_map_pair : m_cross_tx_map) {
        if (cross_tx_map_pair.first <= upper_height) {
            cross_tx_map[cross_tx_map_pair.first] = cross_tx_map_pair.second;
        }
    }
    return true;
}

void xcross_tx_cache_t::recover_cache() {
    base::xvaccount_t vaccount(sys_contract_eth_table_block_addr_with_suffix);
    auto latest_committed_block = m_para->get_vblockstore()->get_latest_committed_block(vaccount, metrics::blockstore_access_from_txpool_refresh_table);
    if (m_cached_evm_upper_height < latest_committed_block->get_height()) {
        bool block_lack = false;
        for (uint64_t h = m_cached_evm_upper_height + 1; h < latest_committed_block->get_height(); h++) {
            auto block = m_para->get_vblockstore()->load_block_object(vaccount, h, base::enum_xvblock_flag_committed, true, metrics::blockstore_access_from_txpool_refresh_table);
            if (block == nullptr) {
                // sync on demand.
                xinfo("xcross_tx_cache_t::recover_cache load block fail. height=%llu", h);
                block_lack = true;
                break;
            }
            process_block(dynamic_cast<data::xblock_t *>(block.get()));
        }
        if (!block_lack) {
            process_block(dynamic_cast<data::xblock_t *>(latest_committed_block.get()));
        }
    }

    if (m_last_proc_evm_height + 1 != m_cached_evm_lower_height && m_cached_evm_lower_height > 1) {
        for (uint64_t h = m_cached_evm_lower_height - 1; h > m_last_proc_evm_height; h--) {
            auto block = m_para->get_vblockstore()->load_block_object(vaccount, h, base::enum_xvblock_flag_committed, true, metrics::blockstore_access_from_txpool_refresh_table);
            if (block == nullptr) {
                // sync on demand.
                xinfo("xcross_tx_cache_t::recover_cache load block fail. height=%llu", h);
                break;
            }
            process_block(dynamic_cast<data::xblock_t *>(block.get()));
        }
    }
}

void xcross_tx_cache_t::update_last_proc_evm_height(uint64_t last_proc_evm_height) {
    xinfo("xcross_tx_cache_t::update_last_proc_evm_height old:%llu,new:%llu", m_last_proc_evm_height, last_proc_evm_height);
    if (last_proc_evm_height <= m_last_proc_evm_height) {
        return;
    }
    for (auto iter = m_cross_tx_map.begin(); iter != m_cross_tx_map.end();) {
        if (iter->first <= last_proc_evm_height) {
            m_tx_num -= iter->second.m_txs.size();
            iter = m_cross_tx_map.erase(iter);
        } else {
            break;
        }
    }
    m_last_proc_evm_height = last_proc_evm_height;
    if (m_cached_evm_lower_height <= m_last_proc_evm_height) {
        m_cached_evm_lower_height = m_last_proc_evm_height + 1;
    }
}

xrelay_elect_cache_t::xrelay_elect_cache_t(const std::shared_ptr<xrelay_chain_resources> & para) : m_para(para) {
}

void xrelay_elect_cache_t::on_elect_db_event(data::xblock_t * block) {
    uint64_t from_height = 0;
    if (m_elect_info_map.empty()) {
        from_height = m_last_proc_height;
    } else {
        auto it = m_elect_info_map.rbegin();
        from_height = it->first;
    }

    for (uint64_t h = from_height + 1; h <= block->get_height(); h++) {
        auto ret = process_election_by_height(h);
        if (!ret) {
            break;
        }
    }
}

bool xrelay_elect_cache_t::get_elect_cache(uint64_t elect_height, std::vector<data::xrelay_election_node_t> & reley_election) {
    if (elect_height == 0) {
        return get_genesis_elect_cache(reley_election);
    }
    auto it = m_elect_info_map.find(elect_height);
    if (it == m_elect_info_map.end()) {
        return false;
    }
    reley_election = it->second;
    return true;
}

bool xrelay_elect_cache_t::get_genesis_elect_cache(std::vector<data::xrelay_election_node_t> & reley_election) {
    if (!m_genesis_elect_info.empty()) {
        reley_election = m_genesis_elect_info;
        return true;
    }

    auto ret = process_election_by_height(0);
    if (!ret) {
        xassert(0);
        return false;
    }
    reley_election = m_genesis_elect_info;
    return true;
}

void xrelay_elect_cache_t::recover_cache() {
    base::xvaccount_t vaccount(sys_contract_zec_elect_relay_addr);

    uint64_t latest_committed_height = m_para->get_vblockstore()->get_latest_committed_block_height(vaccount);
    uint64_t from_height = 0;
    if (m_elect_info_map.empty()) {
        from_height = m_last_proc_height;
    } else {
        auto it = m_elect_info_map.rbegin();
        from_height = it->first;
    }

    for (uint64_t h = from_height + 1; h <= latest_committed_height; h++) {
        auto ret = process_election_by_height(h);
        if (!ret) {
            break;
        }
    }

    if (!m_elect_info_map.empty()) {
        auto it = m_elect_info_map.begin();
        uint64_t to_height = it->first;
        for (uint64_t h = m_last_proc_height + 1; h < to_height; h++) {
            auto ret = process_election_by_height(h);
            if (!ret) {
                break;
            }
        }
    }
}

void xrelay_elect_cache_t::update_last_proc_elect_height(uint64_t last_proc_height) {
    for (auto it = m_elect_info_map.begin(); it != m_elect_info_map.end();) {
        if (it->first < last_proc_height) {
            it = m_elect_info_map.erase(it);
        } else {
            break;
        }
    }
    m_last_proc_height = last_proc_height;
}

bool xrelay_elect_cache_t::get_relay_elections_by_height(const base::xvaccount_t & vaccount, uint64_t height, std::vector<data::xrelay_election_node_t> & relay_elections) {
    xobject_ptr_t<base::xvbstate_t> address_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_committed_block_state(vaccount, height);
    if (address_bstate == nullptr) {
        return false;
    }
    data::xaccount_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(address_bstate.get());

    xinfo("xrelay_elect_cache_t::get_relay_elections_by_height height=%llu", height);

    auto property_name = top::data::election::get_property_by_group_id(common::xdefault_group_id);
    std::vector<std::pair<xpublic_key_t, uint64_t>> election_data;
    top::contract::xcontract_manager_t::instance().get_election_data(top::common::xaccount_address_t{vaccount.get_address()}, unitstate, property_name, election_data);

    for (auto & election : election_data) {
        auto pubkey_str = base::xstring_utl::base64_decode(election.first.to_string());
        xbytes_t bytes_x(pubkey_str.begin() + 1, pubkey_str.begin() + 33);
        xbytes_t bytes_y(pubkey_str.begin() + 33, pubkey_str.end());
        relay_elections.push_back(data::xrelay_election_node_t(evm_common::h256(bytes_x), evm_common::h256(bytes_y), election.second));
    }
    return true;
}

bool xrelay_elect_cache_t::process_election_by_height(uint64_t height) {
    base::xvaccount_t vaccount(sys_contract_zec_elect_relay_addr);
    std::vector<data::xrelay_election_node_t> relay_elections;
    auto ret = get_relay_elections_by_height(vaccount, height, relay_elections);
    if (!ret) {
        return ret;
    }

    if (height == 0) {
        m_genesis_elect_info = relay_elections;
        xdbg("xrelay_elect_cache_t::process_elect_state m_genesis_elect_info size:%u", m_genesis_elect_info.size());
    } else {
        m_elect_info_map[height] = relay_elections;
    }
    xinfo("xrelay_elect_cache_t::process_election_by_height height=%llu, size:%u", height, relay_elections.size());
    return true;
}

bool xwrap_block_convertor::convert_to_relay_block(std::vector<data::xblock_ptr_t> wrap_blocks, std::shared_ptr<data::xrelay_block> & relay_block) {
    // todo(nathan): convert to relay block and save to db.
    return true;
}

xrelay_chain_mgr_t::xrelay_chain_mgr_t(const observer_ptr<store::xstore_face_t> & store,
                                       const observer_ptr<base::xvblockstore_t> & blockstore,
                                       const observer_ptr<mbus::xmessage_bus_face_t> & bus)
  : m_para(std::make_shared<xrelay_chain_resources>(store, blockstore, bus)), m_cross_tx_cache(m_para), m_relay_elect_cache(m_para) {
}

bool xrelay_chain_mgr_t::get_tx_cache_leader(uint64_t lower_height, uint64_t & upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_cross_tx_cache.update_last_proc_evm_height(lower_height);
    return m_cross_tx_cache.get_tx_cache_leader(upper_height, cross_tx_map);
}

bool xrelay_chain_mgr_t::get_tx_cache_backup(uint64_t lower_height, uint64_t upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_cross_tx_cache.update_last_proc_evm_height(lower_height);
    return m_cross_tx_cache.get_tx_cache_backup(upper_height, cross_tx_map);
}

bool xrelay_chain_mgr_t::get_elect_cache(uint64_t elect_height, std::vector<data::xrelay_election_node_t> & reley_election) {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_relay_elect_cache.get_elect_cache(elect_height, reley_election);
}

void xrelay_chain_mgr_t::start(int32_t thread_id) {
    m_thread_id = thread_id;
    m_dispatcher = new xrelay_chain_mgr_dispatcher_t(top::base::xcontext_t::instance(), m_thread_id, this);
    m_dispatcher->create_mailbox(256, 512, 512);
    m_bus_listen_id = m_para->get_bus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&xrelay_chain_mgr_t::on_block_to_db_event, this, std::placeholders::_1));
}

void xrelay_chain_mgr_t::stop() {
    m_para->get_bus()->remove_listener(top::mbus::xevent_major_type_store, m_bus_listen_id);
    m_dispatcher->release_ref();
    m_dispatcher = nullptr;
}

void xrelay_chain_mgr_t::on_timer() {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_on_timer_count++;
    if (m_on_timer_count % 60 != 0) {
        return;
    }
    m_cross_tx_cache.recover_cache();
    m_relay_elect_cache.recover_cache();
}

void xrelay_chain_mgr_t::on_block_to_db_event(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_committed) {
        return;
    }

    mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);

    xwarn("xrelay_chain_mgr_t::on_block_to_db_event in.block=%s,height:%llu,owner:%s,level:%d",
          block_event->blk_hash.c_str(),
          block_event->blk_height,
          block_event->owner.c_str(),
          block_event->blk_level);

    if (block_event->owner != sys_contract_eth_table_block_addr_with_suffix && block_event->owner != sys_contract_relay_table_block_addr &&
        block_event->owner != sys_contract_zec_elect_relay_addr) {
        return;
    }

    // use slow thread to deal with block event
    if (m_dispatcher->is_mailbox_over_limit()) {
        xwarn("xrelay_chain_mgr_t::on_block_to_db_event reatch mailbox limit,drop block=%s,height:%llu", block_event->blk_hash.c_str(), block_event->blk_height);
        return;
    }

    auto event_handler = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        mbus::xevent_object_t * _event_obj = dynamic_cast<mbus::xevent_object_t *>(call.get_param1().get_object());
        xassert(_event_obj != nullptr);
        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(_event_obj->event);
        const data::xblock_ptr_t & block = mbus::extract_block_from(block_event, metrics::blockstore_access_from_mbus_txpool_db_event_on_block);
        base::xvaccount_t _vaccount(block->get_account());
        if (false == m_para->get_vblockstore()->load_block_input(_vaccount, block.get(), metrics::blockstore_access_from_txpool_on_block_event)) {
            xerror("xrelay_chain_mgr_t::on_block_to_db_event fail-load block input output, block=%s", block->dump().c_str());
            return true;
        }
        xassert(block->check_block_flag(base::enum_xvblock_flag_committed));
        if (block->get_account() == sys_contract_eth_table_block_addr_with_suffix) {
            on_evm_db_event(block);
        } else if (block->get_account() == sys_contract_relay_table_block_addr) {
            on_wrap_db_event(block);
        } else {
            on_relay_elect_db_event(block);
        }

        return true;
    };

    base::xauto_ptr<mbus::xevent_object_t> event_obj = new mbus::xevent_object_t(e, 0);
    base::xcall_t asyn_call(event_handler, event_obj.get());
    m_dispatcher->dispatch(asyn_call);
}

void xrelay_chain_mgr_t::on_evm_db_event(data::xblock_ptr_t evm_block) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_cross_tx_cache.on_evm_db_event(evm_block.get());
}

void xrelay_chain_mgr_t::on_wrap_db_event(data::xblock_ptr_t wrap_block) {
    xinfo("xrelay_chain_mgr_t::on_wrap_db_event block:%s", wrap_block->dump().c_str());
    if (wrap_block->is_genesis_block()) {
        return;
    }
    std::lock_guard<std::mutex> lck(m_mutex);

    data::xtableheader_extra_t header_extra;
    auto extra_str = wrap_block->get_header()->get_extra_data();
    header_extra.deserialize_from_string(extra_str);

    auto wrap_data = header_extra.get_relay_wrap_info();
    if (wrap_data.empty()) {
        xerror("xrelay_chain_mgr_t::on_wrap_db_event wrap data should not empty.");
        return;
    }

    data::xrelay_wrap_info_t wrap_info;
    wrap_info.serialize_from_string(wrap_data);
    uint8_t wrap_phase = wrap_info.get_wrap_phase();
    uint64_t evm_height = wrap_info.get_evm_height();
    uint64_t elect_height = wrap_info.get_elect_height();

    if (wrap_phase == 2) {
        // std::shared_ptr<data::xrelay_block> relay_block = nullptr;
        // auto relay_block_data = wrap_block->get_header()->get_extra_data()();
        // todo(nathan): store relay block.
        xinfo("xrelay_chain_mgr_t::on_wrap_db_event created a new relay block.wrap_phase:%d,evm_height:%llu,rec_height:%llu,wrap_block:%s",
              wrap_phase,
              evm_height,
              elect_height,
              wrap_block->dump().c_str());
    }
    m_cross_tx_cache.update_last_proc_evm_height(evm_height);
}

void xrelay_chain_mgr_t::on_relay_elect_db_event(data::xblock_ptr_t relay_elect_block) {
    xinfo("xrelay_chain_mgr_t::on_relay_elect_db_event block:%s", relay_elect_block->dump().c_str());
    std::lock_guard<std::mutex> lck(m_mutex);
    m_relay_elect_cache.on_elect_db_event(relay_elect_block.get());
}

void xrelay_chain_mgr_dispatcher_t::dispatch(base::xcall_t & call) {
    send_call(call);
}

bool xrelay_chain_mgr_dispatcher_t::is_mailbox_over_limit() {
    int64_t in, out;
    int32_t queue_size = count_calls(in, out);
    bool discard = queue_size >= 512;
    if (discard) {
        xwarn("xrelay_chain_mgr_dispatcher_t::is_mailbox_over_limit in=%ld,out=%ld", in, out);
        return true;
    }
    return false;
}

NS_END2
