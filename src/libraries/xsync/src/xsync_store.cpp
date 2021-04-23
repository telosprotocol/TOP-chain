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
    base::xvaccount_t _vaddress(block->get_account());
    return m_blockstore->store_block(_vaddress, block);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_connected_block(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_connected_block(_vaddress);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_committed_block(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_committed_block(_vaddress);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_locked_block(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_locked_block(_vaddress);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_cert_block(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_cert_block(_vaddress);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::load_block_object(const std::string & account, const uint64_t height, bool ask_full_load) {
    base::xvaccount_t _vaddress(account);
    // TODO(jimmy) need changed
    return m_blockstore->load_block_object(_vaddress, height, 0, ask_full_load);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_full_block(const std::string & account) {
    base::xvaccount_t _vaddress(account);
    return m_blockstore->get_latest_full_block(_vaddress);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::query_block(const base::xvaccount_t &account, uint64_t height, const std::string &hash) {
    return m_blockstore->load_block_object(account, height, hash, true);
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_start_block(const std::string & account, enum_chain_sync_policy sync_policy) {
    base::xvaccount_t _vaddress(account);
    if (sync_policy == enum_chain_sync_pocliy_fast) {
        return m_blockstore->get_latest_committed_full_block(account);
    } else if (sync_policy == enum_chain_sync_pocliy_full) {
        return m_blockstore->get_genesis_block(account);;
    }

    return nullptr;
}

base::xauto_ptr<base::xvblock_t> xsync_store_t::get_latest_end_block(const std::string & account, enum_chain_sync_policy sync_policy) {
    base::xvaccount_t _vaddress(account);
    uint32_t underministic_height = 0;
    std::vector<std::vector<xvblock_ptr_t>> blocks;
    base::xvblock_t *block = nullptr;
    bool exist = false;
    if (sync_policy == enum_chain_sync_pocliy_fast) {
        base::xauto_ptr<base::xvblock_t> block1 = m_blockstore->get_latest_connected_block(account);
        block = block1.get();
    } else if (sync_policy == enum_chain_sync_pocliy_full) {
        base::xauto_ptr<base::xvblock_t> block1 = m_blockstore->get_latest_genesis_connected_block(account);
        block = block1.get();
    }

    if (block == nullptr) {
        return nullptr;
    }

    std::vector<xvblock_ptr_t> element;
    element.push_back(xblock_t::raw_vblock_to_object_ptr(block));
    blocks.push_back(element);
    for (uint32_t i = 1; i <= m_undeterministic_heights; i++) {
        auto in_blocks = load_block_objects(account, block->get_height() + i);
        blocks.push_back(in_blocks);
    }

    for (uint32_t i = 0; i < m_undeterministic_heights; i++) {
        auto & undeterministic_blocks = blocks[i+1];
        auto & undeterministic_pre_blocks = blocks[i];
        for (uint32_t j = 0; j < undeterministic_blocks.size();){
            exist = false;
            for (uint32_t k = 0; k < undeterministic_pre_blocks.size(); k++) {
                auto source = undeterministic_blocks[j]->get_last_block_hash();
                auto dst = undeterministic_pre_blocks[k]->get_block_hash();
                if (source != dst) {
                    continue;
                }
                exist = true;
                if ((i+1) > underministic_height) {
                    underministic_height = i+1;
                }
                break;
            }

            if (!exist) {
                undeterministic_blocks.erase(undeterministic_blocks.begin() + j);
                if (undeterministic_blocks.empty()){
                    return blocks[underministic_height].back();
                }
                continue;
            }
            j++;
        }
    }

    return blocks[underministic_height].back();
}


std::vector<data::xvblock_ptr_t> xsync_store_t::load_block_objects(const std::string & account, const uint64_t height) {
    base::xvaccount_t _vaddress(account);
    auto blks_v = m_blockstore->load_block_object(_vaddress, height);
    std::vector<base::xvblock_t*> blks_ptr = blks_v.get_vector();
    std::vector<data::xvblock_ptr_t> blocks;
    for (uint32_t j = 0; j < blks_ptr.size(); j++) {
       blocks.push_back(xblock_t::raw_vblock_to_object_ptr(blks_ptr[j]));
    }
    return blocks;
}

NS_END2
