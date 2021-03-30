// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_store.h"
#include "xdata/xgenesis_data.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

xsync_store_t::xsync_store_t(std::string vnode_id, const observer_ptr<base::xvblockstore_t> &blockstore):
m_vnode_id(vnode_id),
m_blockstore(blockstore) {
}

bool xsync_store_t::store_block(base::xvblock_t* block) {
    return m_blockstore->store_block(block);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_connected_block(const std::string & account) {
    return m_blockstore->get_latest_connected_block(account);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_committed_block(const std::string & account) {
    return m_blockstore->get_latest_committed_block(account);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_locked_block(const std::string & account) {
    return m_blockstore->get_latest_locked_block(account);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_cert_block(const std::string & account) {
    return m_blockstore->get_latest_cert_block(account);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::load_block_object(const std::string & account, const uint64_t height, bool ask_full_load) {
    return m_blockstore->load_block_object(account, height, ask_full_load);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_full_block(const std::string & account) {
    return m_blockstore->get_latest_full_block(account);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::query_block(const base::xvaccount_t &account, uint64_t height, const std::string &hash) {
    return m_blockstore->query_block(account, height, hash);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_start_block(const std::string & account, enum_chain_sync_policy sync_policy) {
    if (sync_policy == enum_chain_sync_pocliy_fast) {
        return m_blockstore->get_latest_full_block(account);
    } else if (sync_policy == enum_chain_sync_pocliy_full) {
        return m_blockstore->get_genesis_block(account);
    }

    return nullptr;
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_end_block(const std::string & account, enum_chain_sync_policy sync_policy) {

    if (sync_policy == enum_chain_sync_pocliy_fast) {
        return m_blockstore->get_highest_sync_block(account);
    } else if (sync_policy == enum_chain_sync_pocliy_full) {
        return m_blockstore->get_genesis_current_block(account);
    }

    return nullptr;
}

NS_END2
