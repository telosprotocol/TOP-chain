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

    //xsync_dbg("[xsync_store] store_block(before) verify:%d %s", verify, block->dump().c_str());

    bool ret = m_blockstore->store_block(block);

    //xsync_dbg("[xsync_store] store_block(after) verify:%d ret=%d %s", verify, ret, block->dump().c_str());

    return ret;
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

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_current_block(const std::string &address) {
#if 1
    return m_blockstore->get_latest_current_block(address);
#else
    base::xauto_ptr<base::xvblock_t> connected_block = m_blockstore->get_latest_connected_block(address);
    base::xauto_ptr<base::xvblock_t> cert_block = get_latest_cert_block(address);
    uint64_t connected_height = connected_block->get_height();
    if (connected_height == cert_block->get_height())
        return connected_block;

    base::xvblock_t* prev_block = connected_block.get();
    prev_block->add_ref();

    for (uint64_t height = connected_height+1; height <= cert_block->get_height()-1; height++) {
        base::xauto_ptr<base::xvblock_t> blk = load_block_object(address, height);
        if (blk == nullptr) {
            return prev_block;
        }

        prev_block->release_ref();
        prev_block = blk.get();
        prev_block->add_ref();
    }

    prev_block->release_ref();

    return cert_block;
#endif
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::load_block_object(const std::string & account, const uint64_t height, bool ask_full_load) {
    return m_blockstore->load_block_object(account, height, ask_full_load);
}

bool xsync_store_t::load_block_input(base::xvblock_t* block) {
    return m_blockstore->load_block_input(block);
}

bool xsync_store_t::load_block_output(base::xvblock_t* block) {
    return m_blockstore->load_block_output(block);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_full_block(const std::string & account) {
    return m_blockstore->get_latest_full_block(account);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::query_block(const base::xvaccount_t &account, uint64_t height, const std::string &hash) {
    return m_blockstore->query_block(account, height, hash);
}

NS_END2
