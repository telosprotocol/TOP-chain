// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xrelay_chain/xrelay_chain_mgr.h"

#include "xdata/xnative_contract_address.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xmbus/xevent_store.h"

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

// todo(nathan): extract and save transaction and receipts.
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
        auto tx_actions = block->get_tx_actions();
        xcross_txs_t cross_txs;
        for (auto & action : tx_actions) {
            if (action.get_org_tx_hash().empty()) {  // not txaction
                continue;
            }
            data::xlightunit_action_t txaction(action);

            if (txaction.get_tx_subtype() != base::enum_transaction_subtype_send) {
                continue;
            }

            data::xtransaction_ptr_t _rawtx = block->query_raw_transaction(txaction.get_tx_hash());
            xdbg("xcross_tx_cache_t::process_block process tx. block:%s,tx:%s", block->dump().c_str(), _rawtx->dump().c_str());
            // if (_rawtx->get_target_addr() != cross_addr) {
            //     continue;
            // }
            if (_rawtx->get_tx_version() != data::xtransaction_version_3 ||
                (_rawtx->get_tx_type() != data::xtransaction_type_deploy_evm_contract && _rawtx->get_tx_type() != data::xtransaction_type_run_contract)) {
                continue;
            }
            evm_common::xevm_transaction_result_t evm_result;
            auto ret = txaction.get_evm_transaction_result(evm_result);
            if (!ret) {
                xwarn("xcross_tx_cache_t::process_block get evm tx result fail. block:%s,tx:%s", block->dump().c_str(), _rawtx->dump().c_str());
                continue;
            }

            if (evm_result.status == evm_common::Success) {
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
    base::xvaccount_t vaccount("Ta0004@0");
    if (block->get_height() > m_cached_evm_upper_height) {
        for (uint32_t h = m_cached_evm_upper_height + 1; h < block->get_height(); h++) {
            auto latest_committed_block =
                m_para->get_vblockstore()->load_block_object(vaccount, h, base::enum_xvblock_flag_committed, true, metrics::blockstore_access_from_txpool_refresh_table);
            process_block(dynamic_cast<data::xblock_t *>(latest_committed_block.get()));
        }
    }
}

bool xcross_tx_cache_t::get_tx_cache_leader(uint64_t & upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) const {
    if (m_last_proc_evm_height + 1 != m_cached_evm_lower_height) {
        xinfo("xcross_tx_cache_t::get_tx_cache_leader cache not full.");
        return false;
    }
    upper_height = m_cached_evm_upper_height;
    cross_tx_map = m_cross_tx_map;
    return true;
}

bool xcross_tx_cache_t::get_tx_cache_backup(uint64_t upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) const {
    if (m_last_proc_evm_height + 1 != m_cached_evm_lower_height) {
        xinfo("xcross_tx_cache_t::get_tx_cache_leader cache not full.");
        return false;
    }

    for (auto & cross_tx_map_pair : m_cross_tx_map) {
        if (cross_tx_map_pair.first < upper_height) {
            cross_tx_map[cross_tx_map_pair.first] = cross_tx_map_pair.second;
        }
    }
    return true;
}

void xcross_tx_cache_t::recover_cache() {
    base::xvaccount_t vaccount("Ta0004@0");
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

    // todo(nathan): recover cache
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

bool xwrap_block_convertor::convert_to_relay_block(std::vector<data::xblock_ptr_t> wrap_blocks, std::shared_ptr<data::xrelay_block> & relay_block) {
    // todo(nathan):
    return true;
}

xrelay_chain_mgr_t::xrelay_chain_mgr_t(const observer_ptr<store::xstore_face_t> & store,
                                       const observer_ptr<base::xvblockstore_t> & blockstore,
                                       const observer_ptr<mbus::xmessage_bus_face_t> & bus)
  : m_para(std::make_shared<xrelay_chain_resources>(store, blockstore, bus)), m_cross_tx_cache(m_para) {
    m_wrap_blocks.resize(3);
}

bool xrelay_chain_mgr_t::get_tx_cache_leader(uint64_t lower_height, uint64_t & upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) {
    std::lock_guard<std::mutex> lck(m_mutex);
    // if (!m_wrap_blocks.empty()) {
    //     if (m_wrap_blocks[0]->get_height() >= latest_wrap_block->get_height()) {
    //         xerror("xrelay_chain_mgr_t::get_tx_cache_leader wrap block cache height >= latest wrap block.cache:%s,latest:%s",
    //                m_wrap_blocks[0]->dump().c_str(),
    //                latest_wrap_block->dump().c_str());
    //     }
    //     m_wrap_blocks.clear();
    // }
    m_cross_tx_cache.update_last_proc_evm_height(lower_height);
    return m_cross_tx_cache.get_tx_cache_leader(upper_height, cross_tx_map);
}

bool xrelay_chain_mgr_t::get_tx_cache_backup(uint64_t lower_height, uint64_t upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_cross_tx_cache.update_last_proc_evm_height(lower_height);
    return m_cross_tx_cache.get_tx_cache_backup(upper_height, cross_tx_map);
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

    // todo(nathan):sys_contract_zec_elect_eth_addr change to relay nodes elect contract addr.
    if (block_event->owner != "Ta0004@0" && block_event->owner != relay_block_addr && block_event->owner != sys_contract_zec_elect_eth_addr) {
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
        if (block->get_account() == "Ta0004@0") {
            on_evm_db_event(block);
        } else if (block->get_account() == relay_block_addr) {
            on_wrap_db_event(block);
        } else {
            on_wrap_db_event(block);
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

    uint8_t wrap_phase;
    uint64_t evm_height;
    uint64_t rec_height;
    auto & wrap_data = wrap_block->get_header()->get_comments();
    if (wrap_data.empty()) {
        xerror("xrelay_chain_mgr_t::on_wrap_db_event wrap data should not empty. wrap_block:%s", wrap_block->dump().c_str());
        return;
    } else {
        base::xstream_t stream_wrap_data{base::xcontext_t::instance(), (uint8_t *)wrap_data.data(), static_cast<uint32_t>(wrap_data.size())};
        stream_wrap_data >> wrap_phase;
        stream_wrap_data >> evm_height;
        stream_wrap_data >> rec_height;
    }
    if (wrap_phase == 0) {
        // a new relay block.
        m_wrap_blocks.clear();
    }
    m_wrap_blocks.push_back(wrap_block);

    if (wrap_phase == 2) {
        if (m_wrap_blocks.size() == 3) {
            std::shared_ptr<data::xrelay_block> relay_block = nullptr;
            xwrap_block_convertor::convert_to_relay_block(m_wrap_blocks, relay_block);
            auto relay_block_data = m_wrap_blocks[2]->get_relay_block_data();
            // todo: store relay block.
            xinfo("xrelay_chain_mgr_t::on_wrap_db_event created a new relay block.wrap_phase:%d,evm_height:%llu,rec_height:%llu,relay_block_data:%s,wrap_block:%s",
                  wrap_phase,
                  evm_height,
                  rec_height,
                  relay_block_data.c_str(),
                  wrap_block->dump().c_str());
        } else {
            // todo: sync wrap block on demand.
        }
        m_wrap_blocks.clear();
    }
    m_cross_tx_cache.update_last_proc_evm_height(evm_height);
}

void xrelay_chain_mgr_t::on_relay_elect_db_event(data::xblock_ptr_t relay_elect_block) {
    xinfo("xrelay_chain_mgr_t::on_relay_elect_db_event block:%s", relay_elect_block->dump().c_str());
    if (relay_elect_block->is_genesis_block()) {
        return;
    }
    std::lock_guard<std::mutex> lck(m_mutex);
    // todo(nathan):decode relay elect data.
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
