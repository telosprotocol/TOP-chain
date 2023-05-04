// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_store.h"

#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xsync/xsync_log.h"
#include "xmbus/xmessage_bus.h"
#include "xsync/xsync_store_shadow.h"
#include "xchain_fork/xutility.h"
#include "xdata/xcheckpoint.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"

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
        xassert(false); // TODO(jimmy) not support unit sync now
        return false;
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
    if (false == check_block_full_data(_vaddress, _block.get())) {
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
    if (_vaddress.is_unit_address()) {
        xerror("xsync_store_t::get_latest_committed_block_height %s",account.c_str());
    }
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
    base::xvaccount_t _vaddress(account);
    // TODO(jimmy) need changed
    return m_blockstore->load_block_object(_vaddress, height, viewid, ask_full_load, metrics::blockstore_access_from_sync_load_block_object);
}

bool xsync_store_t::existed(const std::string & account, const uint64_t height, uint64_t viewid) {
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
    if (false == check_block_full_data(_vaddress, _block.get())) {
        xerror("xsync_store_t::get_latest_full_block fail-load block input or output or offdata. block=%s", _block->dump().c_str());
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
        // base::xauto_ptr<base::xvblock_t> _full_block = m_blockstore->get_latest_committed_full_block(account, metrics::blockstore_access_from_sync_get_latest_committed_full_block);
        // if (_full_block != nullptr && _full_block->get_block_level() == base::enum_xvblock_level_table) {
        //     if (!_full_block->is_full_state_block()) {
        //         auto _executed_block_height = m_blockstore->get_latest_executed_block_height(account, metrics::blockstore_access_from_sync_get_latest_committed_full_block);
        //         if (_full_block->get_height() <= _executed_block_height) {
        //             if (false == base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_full_block_offsnapshot(_full_block.get(),metrics::statestore_access_from_sync_query_offchain)) {
        //                 xwarn("xsync_store_t::get_latest_start_block_height fail-get off snapshot.block=%s", _full_block->dump().c_str());
        //             }
        //         } else {
        //             xwarn("xsync_store_t::get_latest_start_block_height fail-full height less than execute height.block=%s", _full_block->dump().c_str());
        //         }
        //     }
        // }
        // return _full_block->get_height();
        base::xvaccount_t _vaddress(account);
        return m_blockstore->get_latest_full_block_height(_vaddress);
    } else if (sync_policy == enum_chain_sync_policy_full) {
        return get_genesis_block_height(account);
    } else if (sync_policy == enum_chain_sync_policy_checkpoint) {
        // need to fix: checkpoint
        return get_latest_immutable_connected_checkpoint_height(account);
    }

    return 0;
}

uint64_t xsync_store_t::get_latest_end_block_height(const std::string & account, enum_chain_sync_policy sync_policy) {
    base::xvaccount_t _vaddress(account);
    uint64_t connect_height = 0;
    uint64_t connect_immutable_height = 0;
    if (sync_policy == enum_chain_sync_policy_fast) {
        connect_height = m_blockstore->get_latest_connected_block_height(account);
    } else if (sync_policy == enum_chain_sync_policy_full) {
        connect_height = m_shadow->genesis_connect_height(account);
    } else if (sync_policy == enum_chain_sync_policy_checkpoint) {
        // need to fix: checkpoint
        connect_height = get_latest_mutable_connected_checkpoint_height(account);
        connect_immutable_height = get_latest_immutable_connected_checkpoint_height(account);
    }

    if (connect_height == 0) {
        if (load_block_object(account, connect_height + 1, false) == nullptr) {
            return 0;
        }
        if (load_block_object(account, connect_height + 2, false) == nullptr) {
            return 1;
        }
    }
    //connect_height will be 0 first
    if (connect_height < connect_immutable_height) {
        return connect_immutable_height;
    } else {
        return connect_height + 2;
    }
}

uint64_t xsync_store_t::get_latest_immutable_connected_checkpoint_height(const std::string & account) {
    common::xaccount_address_t _vaddress(account);
    std::error_code err;
    auto checkpoint = data::xtop_chain_checkpoint::get_latest_checkpoint(_vaddress, err);
    if (err) {
        return 0;
    }
    return checkpoint.height;
}

uint64_t xsync_store_t::get_latest_mutable_connected_checkpoint_height(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->update_get_latest_cp_connected_block_height(_vaddress);
}

uint64_t xsync_store_t::get_latest_stable_connected_checkpoint_height(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->update_get_db_latest_cp_connected_block_height(_vaddress);
}

uint64_t xsync_store_t::get_latest_deleted_block_height(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_deleted_block_height(_vaddress);
}

uint64_t xsync_store_t::get_latest_block_with_state(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_lowest_executed_block_height(_vaddress, metrics::blockstore_access_from_sync_get_latest_committed_full_block);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_start_block(const std::string & account, enum_chain_sync_policy sync_policy) {
    base::xvaccount_t _vaddress(account);
    if (sync_policy == enum_chain_sync_policy_fast) {
        base::xauto_ptr<base::xvblock_t> _full_block = m_blockstore->get_latest_committed_full_block(account, metrics::blockstore_access_from_sync_get_latest_committed_full_block);
        if (_full_block != nullptr && _full_block->get_block_level() == base::enum_xvblock_level_table) {
            if (false == check_block_full_data(_vaddress, _full_block.get())) {
                xerror("xsync_store_t::load_block_objects fail-load block input or output or offdata block=%s", _full_block->dump().c_str());
                return nullptr;
            }
            // if (!_full_block->is_full_state_block()) {
            //     const uint64_t _executed_block_height = m_blockstore->get_latest_executed_block_height(account, metrics::blockstore_access_from_sync_get_latest_executed_block);
            //     if (_full_block->get_height() <= _executed_block_height) {
            //         if (false == base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_full_block_offsnapshot(_full_block.get(), metrics::statestore_access_from_sync_query_offchain)) {
            //             xwarn("xsync_store_t::get_latest_start_block fail-get off snapshot.block=%s", _full_block->dump().c_str());
            //         }
            //     } else {
            //         xwarn("xsync_store_t::get_latest_start_block fail-full height less than execute height.block=%s", _full_block->dump().c_str());
            //     }
            // }
        }
        return _full_block;
    } else if (sync_policy == enum_chain_sync_policy_full) {
        auto _genesis_block = m_blockstore->get_genesis_block(account, metrics::blockstore_access_from_sync_get_genesis_block);
        if (false == check_block_full_data(_vaddress, _genesis_block.get())) {
            xerror("xsync_store_t::load_block_objects fail-load block input or output or offdata. genesis_block=%s", _genesis_block->dump().c_str());
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
        if (false == check_block_full_data(_vaddress, blks_ptr[j])) {
            xerror("xsync_store_t::load_block_objects fail-load block input or output or offdata. block=%s", blks_ptr[j]->dump().c_str());
            return {};
        }
        blocks.push_back(data::xblock_t::raw_vblock_to_object_ptr(blks_ptr[j]));
    }
    return blocks;
}

base::xauto_ptr<base::xvblock_t>  xsync_store_t::load_block_object(const base::xvaccount_t & account,const uint64_t height) {
    return m_blockstore->load_block_object(account, height, base::enum_xvblock_flag_committed, false);
}
base::xauto_ptr<base::xvblock_t>  xsync_store_t::load_block_object(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag flag) {
    return m_blockstore->load_block_object(account, height, flag, false);
}

base::xauto_ptr<base::xvblock_t>  xsync_store_t::load_block_object(const base::xvaccount_t & account,const uint64_t height, const std::string & hash) {
    return m_blockstore->load_block_object(account, height, hash, true);
}

base::xauto_ptr<base::xvblock_t>  xsync_store_t::load_block_object(const base::xvaccount_t & account,const uint64_t height, const uint64_t viewid) {
    return m_blockstore->load_block_object(account, height, viewid, true);
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

bool xsync_store_t::is_support_big_pack_forked() {
    if (m_sync_big_pack) {
        return true;
    }

    set_fork_point();
    return m_sync_big_pack;
}

bool xsync_store_t::is_fullnode_elect_forked() {
    if (m_sync_fullnode_elect_forked) {
        return true;
    }

    set_fork_point();
    return m_sync_fullnode_elect_forked;
}

base::xauto_ptr<base::xvbindex_t> xsync_store_t::recover_and_load_commit_index(const base::xvaccount_t & account, uint64_t height) {
    return m_blockstore->recover_and_load_commit_index(account, height);
}

void xsync_store_t::set_fork_point() {

    if (m_sync_big_pack && m_sync_fullnode_elect_forked) {
        return;
    }
    
    auto vb = m_blockstore->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr));
    if (vb == nullptr) {
        return;
    }

    xdbg("xsync_store_t::forked clock:%llu", vb->get_height());
    // TODO(jimmy) remove fork points
    if(!m_sync_big_pack) {
        bool forked = chain_fork::xutility_t::is_forked(fork_points::v11200_sync_big_packet, vb->get_height());
        if (forked) {
            m_sync_big_pack = true;
            xinfo("xsync_store_t::block fork point:sync_big_packet already forked clock:%llu", vb->get_height());
        }
    }
    
    if(!m_sync_fullnode_elect_forked) {
        bool forked = chain_fork::xutility_t::is_forked(fork_points::v11200_fullnode_elect, vb->get_height());
        if (forked) {
            m_sync_fullnode_elect_forked = true;
            xinfo("xsync_store_t::block fork point:fullnode_elect already forked clock:%llu", vb->get_height());
        }
    }

    return;
}

bool xsync_store_t::check_block_full_data(const base::xvaccount_t & account, base::xvblock_t* block)
{
    if (false == m_blockstore->load_block_output(account, block, metrics::blockstore_access_from_sync_load_block_objects_output)
        || false == m_blockstore->load_block_input(account, block, metrics::blockstore_access_from_sync_load_block_objects_input)
        || false == m_blockstore->load_block_output_offdata(account, block)) {
        xerror("xsync_store_t::load_block_objects fail-load block input or output or offdata. block=%s", block->dump().c_str());
        return false;
    }
    return true;
}

NS_END2
