// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstatestore/xaccount_index_cache.h"

NS_BEG2(top, statestore)

#define KEEP_CACHE_BLOCK_NUM (4)

void xaccount_index_cache_t::update_new_cert_block(base::xvblock_t * cert_block, const std::map<std::string, base::xaccount_index_t> & account_index_map) {
    std::lock_guard<std::mutex> l(m_mutex);
    for (auto it = m_cache.begin(); it != m_cache.end();) {
        uint64_t height = it->first;
        if (height + KEEP_CACHE_BLOCK_NUM <= cert_block->get_height()) {
            it = m_cache.erase(it);
            continue;
        } else if (height + 1 == cert_block->get_height()) {
            if (it->second.get_block_hash() != cert_block->get_last_block_hash()) {
                xinfo("xaccount_index_cache_t::update_new_cert_block hash not match cache cert_block:%s", cert_block->dump().c_str());
                it = m_cache.erase(it);
                continue;
            }
        } else if (height == cert_block->get_height()) {
            xwarn("xaccount_index_cache_t::update_new_cert_block cert fork cert_block:%s", cert_block->dump().c_str());
            it = m_cache.erase(it);
            continue;
        } else if (height > cert_block->get_height()) {
            xerror("xaccount_index_cache_t::update_new_cert_block cache height is more higher height:%llu cert_block:%s", height, cert_block->dump().c_str());
            return;
        }
        it++;
    }

    m_cache.emplace(cert_block->get_height(), xblock_account_indexes_t(cert_block->get_block_hash(), account_index_map));
}

bool xaccount_index_cache_t::get_account_index(base::xvblock_t * block, const std::string & account, base::xaccount_index_t & account_index) {
    std::lock_guard<std::mutex> l(m_mutex);
    if (m_cache.empty()) {
        return false;
    }
    uint64_t block_height = block->get_height();
    uint64_t cert_height = m_cache.rbegin()->first;
    uint64_t lowest_height = m_cache.begin()->first;
    if (cert_height < block_height || lowest_height > block_height) {
        return false;
    }
    uint64_t last_height = cert_height + 1;
    auto & block_hash = block->get_block_hash();
    bool height_hash_match = false;
    for (auto it = m_cache.rbegin(); it != m_cache.rend();) {
        auto height = it->first;
        auto & hash = it->second.get_block_hash();
        // if block forked or cache not continuous, not use cache to get account index.
        if (last_height != height + 1) {
            return false;
        }
        if (height > block_height) {
            it++;
            last_height = height;
            continue;
        }
        if (!height_hash_match) {
            if ((height == block_height && hash == block_hash)) {
                height_hash_match = true;
            } else {
                if (height <= block_height) {
                    return false;
                }
                it++;
                last_height = height;
                continue;
            }
        }

        auto ret = it->second.get_account_index(account, account_index);
        if (ret) {
            xdbg("xaccount_index_cache_t::get_account_index succ.block=%s,account=%s,height=%llu,cert height=%llu", block->dump().c_str(), account.c_str(), height, cert_height);
            return true;
        }
        it++;
        last_height = height;
    }
    return false;
}

xblock_account_indexes_t::xblock_account_indexes_t(const std::string & block_hash, const std::map<std::string, base::xaccount_index_t> & account_index_map)
  : m_block_hash(block_hash), m_account_index_map(account_index_map) {
}

bool xblock_account_indexes_t::get_account_index(const std::string & account, base::xaccount_index_t & account_index) const {
    auto it = m_account_index_map.find(account);
    if (it == m_account_index_map.end()) {
        return false;
    }
    account_index = it->second;
    return true;
}

NS_END2
