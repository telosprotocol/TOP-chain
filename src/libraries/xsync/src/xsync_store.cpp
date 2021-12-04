// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_store.h"
#include "xdata/xgenesis_data.h"
#include "xsync/xsync_log.h"
#include "xmbus/xmessage_bus.h"
#include "xsync/xsync_store_shadow.h"
#include "xchain_fork/xchain_upgrade_center.h"
NS_BEG2(top, sync)

xsync_store_t::xsync_store_t(std::string vnode_id, const observer_ptr<base::xvblockstore_t> &blockstore, xsync_store_shadow_t *shadow):
m_vnode_id(vnode_id),
m_blockstore(blockstore),
m_shadow(shadow) {
    m_shadow->set_store(this);
}

bool xsync_store_t::store_block(base::xvblock_t* block) {
    base::xvaccount_t _vaddress(block->get_account());
    if (block->get_block_level() == base::enum_xvblock_level_unit) {
        XMETRICS_GAUGE(metrics::xsync_store_block_units, 1);
        return m_blockstore->store_committed_unit_block(_vaddress, block);
    } else if (block->get_block_level() == base::enum_xvblock_level_table) {
        XMETRICS_GAUGE(metrics::xsync_store_block_tables, 1);
    }
    return m_blockstore->store_block(_vaddress, block, metrics::blockstore_access_from_sync_store_blk);
}

bool xsync_store_t::store_blocks(std::vector<base::xvblock_t*> &blocks) {
    if (blocks.empty()) {
        return true;
    }
    if (blocks[0]->get_block_level() == base::enum_xvblock_level_unit) {
        XMETRICS_GAUGE(metrics::xsync_store_block_units, blocks.size());
    } else if (blocks[0]->get_block_level() == base::enum_xvblock_level_table) {
        XMETRICS_GAUGE(metrics::xsync_store_block_tables, blocks.size());
    }
    base::xvaccount_t _vaddress(blocks[0]->get_account());
    return m_blockstore->store_blocks(_vaddress, blocks, true);
}

uint32_t xsync_store_t::add_listener(int major_type, mbus::xevent_queue_cb_t cb) {
    mbus::xmessage_bus_t *mbus = (mbus::xmessage_bus_t *)base::xvchain_t::instance().get_xevmbus();
    return mbus->add_listener(major_type, cb);
}

void xsync_store_t::remove_listener(int major_type, uint32_t id) {
    mbus::xmessage_bus_t *mbus = (mbus::xmessage_bus_t *)base::xvchain_t::instance().get_xevmbus();
    mbus->remove_listener(major_type, id);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_cert_block(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    
    auto _block = m_blockstore->get_latest_cert_block(_vaddress, metrics::blockstore_access_from_sync_get_latest_cert_block);
    if (false == m_blockstore->load_block_output(_vaddress, _block.get())
        || false == m_blockstore->load_block_input(_vaddress, _block.get()) ) {
        xerror("xsync_store_t::get_latest_cert_block fail-load block input or output. block=%s", _block->dump().c_str());
        return nullptr;
    }
    return _block;
}

uint64_t xsync_store_t::get_genesis_block_height(const std::string & account) {
    return 0;
}

uint64_t xsync_store_t::get_latest_committed_block_height(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_committed_block_height(_vaddress);
}

uint64_t xsync_store_t::get_latest_connected_block_height(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_connected_block_height(_vaddress);
}

uint64_t xsync_store_t::get_latest_genesis_connected_block_height(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_genesis_connected_block_height(_vaddress);    
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::load_block_object(const std::string & account, const uint64_t height, bool ask_full_load, uint64_t viewid) {
    XMETRICS_TIME_RECORD("xsync_store_load_block_object");
    base::xvaccount_t _vaddress(account);
    // TODO(jimmy) need changed
    return m_blockstore->load_block_object(_vaddress, height, viewid, ask_full_load, metrics::blockstore_access_from_sync_load_block_object);
}

bool xsync_store_t::existed(const std::string & account, const uint64_t height, uint64_t viewid) {
    XMETRICS_TIME_RECORD("xsync_existed");
    base::xvaccount_t _vaddress(account);
    auto index = m_blockstore->load_block_index(_vaddress, height, viewid, metrics::blockstore_access_from_sync_existed_blk);
    return index != nullptr;
}

// force update _highest_connect_block_height
void xsync_store_t::update_latest_genesis_connected_block(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    m_blockstore->get_latest_genesis_connected_index(_vaddress, true, metrics::blockstore_access_from_sync_update_latest_genesis_connected_block);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_full_block(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    auto _block = m_blockstore->get_latest_committed_full_block(_vaddress, metrics::blockstore_access_from_sync_get_latest_committed_full_block);
    if (false == m_blockstore->load_block_output(_vaddress, _block.get())
        || false == m_blockstore->load_block_input(_vaddress, _block.get()) ) {
        xerror("xsync_store_t::get_latest_full_block fail-load block input or output. block=%s", _block->dump().c_str());
        return nullptr;
    }
    return _block;
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::query_block(const base::xvaccount_t &account, uint64_t height, const std::string &hash) {
    return m_blockstore->load_block_object(account, height, hash, true, metrics::blockstore_access_from_sync_query_blk);
}

uint64_t xsync_store_t::get_latest_start_block_height(const std::string & account, enum_chain_sync_policy sync_policy) {
    base::xvaccount_t _vaddress(account);
    if (sync_policy == enum_chain_sync_policy_fast) {
        base::xauto_ptr<base::xvblock_t> _full_block = m_blockstore->get_latest_committed_full_block(account, metrics::blockstore_access_from_sync_get_latest_committed_full_block);
        if (_full_block != nullptr && _full_block->get_block_level() == base::enum_xvblock_level_table) {
            if (!_full_block->is_full_state_block()) {
                auto _executed_block_height = m_blockstore->get_latest_executed_block_height(account, metrics::blockstore_access_from_sync_get_latest_committed_full_block);
                if (_full_block->get_height() <= _executed_block_height) {
                    if (false == base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_full_block_offsnapshot(_full_block.get(),metrics::statestore_access_from_sync_query_offchain)) {
                        xwarn("xsync_store_t::get_latest_start_block_height fail-get off snapshot.block=%s", _full_block->dump().c_str());
                    }
                } else {
                    xwarn("xsync_store_t::get_latest_start_block_height fail-full height less than execute height.block=%s", _full_block->dump().c_str());
                }
            }
        }
        return _full_block->get_height();
    } else if (sync_policy == enum_chain_sync_policy_full) {
        return get_genesis_block_height(account);
    }

    return 0;
}

uint64_t xsync_store_t::get_latest_end_block_height(const std::string & account, enum_chain_sync_policy sync_policy) {
    base::xvaccount_t _vaddress(account);
    uint64_t connect_height = 0;
    if (sync_policy == enum_chain_sync_policy_fast) {
        connect_height = m_blockstore->get_latest_connected_block_height(account);
    } else if (sync_policy == enum_chain_sync_policy_full) {
        connect_height = m_shadow->genesis_connect_height(account);
        //connect_height = m_blockstore->get_latest_genesis_connected_block_height(account);
    }

    if (connect_height == 0) {
        if (load_block_object(account, connect_height + 1, false) == nullptr) {
            return 0;
        }
        if (load_block_object(account, connect_height + 2, false) == nullptr) {
            return 1;
        }
    }
    return connect_height + 2;
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_start_block(const std::string & account, enum_chain_sync_policy sync_policy) {
    base::xvaccount_t _vaddress(account);
    if (sync_policy == enum_chain_sync_policy_fast) {
        base::xauto_ptr<base::xvblock_t> _full_block = m_blockstore->get_latest_committed_full_block(account, metrics::blockstore_access_from_sync_get_latest_committed_full_block);
        if (_full_block != nullptr && _full_block->get_block_level() == base::enum_xvblock_level_table) {
            if (false == m_blockstore->load_block_output(_vaddress, _full_block.get())
                || false == m_blockstore->load_block_input(_vaddress, _full_block.get()) ) {
                xerror("xsync_store_t::load_block_objects fail-load block input or output. block=%s", _full_block->dump().c_str());
                return nullptr;
            }
            if (!_full_block->is_full_state_block()) {
                const uint64_t _executed_block_height = m_blockstore->get_latest_executed_block_height(account, metrics::blockstore_access_from_sync_get_latest_executed_block);
                if (_full_block->get_height() <= _executed_block_height) {
                    if (false == base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_full_block_offsnapshot(_full_block.get(), metrics::statestore_access_from_sync_query_offchain)) {
                        xwarn("xsync_store_t::get_latest_start_block fail-get off snapshot.block=%s", _full_block->dump().c_str());
                    }
                } else {
                    xwarn("xsync_store_t::get_latest_start_block fail-full height less than execute height.block=%s", _full_block->dump().c_str());
                }
            }
        }
        return _full_block;
    } else if (sync_policy == enum_chain_sync_policy_full) {
        auto _genesis_block = m_blockstore->get_genesis_block(account, metrics::blockstore_access_from_sync_get_genesis_block);
        if (false == m_blockstore->load_block_output(_vaddress, _genesis_block.get())
            || false == m_blockstore->load_block_input(_vaddress, _genesis_block.get()) ) {
            xerror("xsync_store_t::load_block_objects fail-load block input or output. block=%s", _genesis_block->dump().c_str());
            return nullptr;
        }
        return _genesis_block;
    }

    return nullptr;
}

std::vector<data::xvblock_ptr_t> xsync_store_t::load_block_objects(const std::string & account, const uint64_t height) {
    base::xvaccount_t _vaddress(account);
    auto blks_v = m_blockstore->load_block_object(_vaddress, height, metrics::blockstore_access_from_sync_load_block_objects);
    std::vector<base::xvblock_t*> blks_ptr = blks_v.get_vector();
    std::vector<data::xvblock_ptr_t> blocks;
    for (uint32_t j = 0; j < blks_ptr.size(); j++) {
        if (false == m_blockstore->load_block_output(_vaddress, blks_ptr[j], metrics::blockstore_access_from_sync_load_block_objects_output)
            || false == m_blockstore->load_block_input(_vaddress, blks_ptr[j], metrics::blockstore_access_from_sync_load_block_objects_input) ) {
            xerror("xsync_store_t::load_block_objects fail-load block input or output. block=%s", blks_ptr[j]->dump().c_str());
            return {};
        }
        blocks.push_back(xblock_t::raw_vblock_to_object_ptr(blks_ptr[j]));
    }
    return blocks;
}

std::vector<data::xvblock_ptr_t> xsync_store_t::load_block_objects(const std::string & tx_hash, const base::enum_transaction_subtype type) {
    auto blocks = m_blockstore->load_block_object(tx_hash, type, metrics::blockstore_access_from_sync_load_tx);
    for (auto & block : blocks) {
        if (false == m_blockstore->load_block_output(base::xvaccount_t(block->get_account()), block.get(), metrics::blockstore_access_from_sync_load_tx_output)
            || false == m_blockstore->load_block_input(base::xvaccount_t(block->get_account()), block.get(), metrics::blockstore_access_from_sync_load_tx_input)) {
            xerror("xsync_store_t::load_block_objects for txhash fail-load block input or output. block=%s", block->dump().c_str());
            return {};
        }
    }
    return blocks;
}
base::xauto_ptr<base::xvblock_t>  xsync_store_t::load_block_object(const base::xvaccount_t & account,const uint64_t height) {
    return m_blockstore->load_block_object(account, height, base::enum_xvblock_flag_committed, false);
}
bool xsync_store_t::set_genesis_height(const base::xvaccount_t &account, const std::string &height) {
    return m_blockstore->set_genesis_height(account, height);
}

const std::string xsync_store_t::get_genesis_height(const base::xvaccount_t &account) {
    return m_blockstore->get_genesis_height(account);
}

bool xsync_store_t::set_block_span(const base::xvaccount_t &account, const uint64_t height,  const std::string &span) {
    return m_blockstore->set_block_span(account, height, span);
}

bool xsync_store_t::delete_block_span(const base::xvaccount_t &account, const uint64_t height) {
    return m_blockstore->delete_block_span(account, height);
}

const std::string xsync_store_t::get_block_span(const base::xvaccount_t &account, const uint64_t height) {
    return m_blockstore->get_block_span(account, height);
}

xsync_store_shadow_t* xsync_store_t::get_shadow() {
    return m_shadow;
};

bool xsync_store_t::set_unit_proof(const base::xvaccount_t & account, const std::string & unit_proof, uint64_t height) {
    if (!m_blockstore->set_unit_proof(account, unit_proof, height)) {
        xerror("xsync_store_t::store_unit_proof account %s,fail to writed into db,unit_proof=%s",account.get_address().c_str(), unit_proof.c_str());
        return false;
    }
    return true;
}

const std::string xsync_store_t::get_unit_proof(const base::xvaccount_t & account, uint64_t height) {
    return m_blockstore->get_unit_proof(account, height);
}

bool xsync_store_t::remove_empty_unit_forked() {
    if (m_remove_empty_unit_forked) {
        return true;
    }

    auto vb = m_blockstore->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr));
    if (vb == nullptr) {
        return false;
    }

    xdbg("xsync_store_t::remove_empty_unit_forked clock:%llu", vb->get_height());
    auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, vb->get_height());
    if (forked) {
        xinfo("xsync_store_t::remove_empty_unit_forked already forked clock:%llu", vb->get_height());
        m_remove_empty_unit_forked = true;
    }
    return m_remove_empty_unit_forked;
}

NS_END2
