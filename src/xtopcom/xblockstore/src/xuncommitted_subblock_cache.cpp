// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xutl.h"
#include "xuncommitted_subblock_cache.h"

#include <cinttypes>

namespace top {
namespace store {
uint64_t xuncommitted_subblock_cache_t::get_cert_height() const {
    return m_cert_height;
}

const std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & xuncommitted_subblock_cache_t::get_lock_cache() const {
    return m_lock_cache;
}

const std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & xuncommitted_subblock_cache_t::get_cert_cache() const {
    return m_cert_cache;
}

void xuncommitted_subblock_cache_t::add_blocks(uint64_t height, const std::vector<base::xvblock_ptr_t> & cert_blocks, const std::vector<base::xvblock_ptr_t> & lock_blocks) {
    if (height < m_cert_height) {
        xerror("xuncommitted_subblock_cache_t::add_blocks height invalid.height:%llu,m_cert_height:%llu", height, m_cert_height);
        return;
    }
    if (height == m_cert_height) {
        // do nothing.
    } else if (height == m_cert_height + 1) {
        m_lock_cache.clear();
        m_lock_cache.swap(m_cert_cache);

    } else {
        m_lock_cache.clear();
        m_cert_cache.clear();
    }

    xdbg("xuncommitted_subblock_cache_t::add_blocks height update from %llu to %llu", m_cert_height, height);
    add_blocks_to_cache(cert_blocks, m_cert_cache);
    add_blocks_to_cache(lock_blocks, m_lock_cache);
    m_cert_height = height;
}

std::vector<base::xvbindex_t *> xuncommitted_subblock_cache_t::get_block_indexs_from_cache(const std::string & account,
                                                                                           const std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & block_cache,
                                                                                           uint64_t height) const {
    std::vector<base::xvbindex_t *> blocks;
    for (auto cache_iter = block_cache.rbegin(); cache_iter != block_cache.rend(); cache_iter++) {
        auto & block_map = cache_iter->second;
        auto block_map_iter = block_map.find(account);
        if (block_map_iter != block_map.end()) {
            auto & block = block_map_iter->second;
            if (block->get_height() == height) {
                block->add_ref();
                blocks.push_back(block);
            }
        }
    }
    return blocks;
}

base::xvbindex_t * xuncommitted_subblock_cache_t::get_block_index_from_cache(const std::string & account,
                                                                             const std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & block_cache,
                                                                             const xblock_match_base_t & match_func) const {
    for (auto cache_iter = block_cache.rbegin(); cache_iter != block_cache.rend(); cache_iter++) {
        auto & block_map = cache_iter->second;
        auto block_map_iter = block_map.find(account);
        if (block_map_iter != block_map.end()) {
            auto & block = block_map_iter->second;
            if (match_func.is_match(block)) {
                block->add_ref();
                return block;
            }
        }
    }
    return nullptr;
}

base::xauto_ptr<base::xvbindex_t> xuncommitted_subblock_cache_t::load_block_index(const base::xvaccount_t & account, const xblock_match_base_t & match_func) const {
    auto block_index = get_block_index_from_cache(account.get_account(), m_cert_cache, match_func);
    if (block_index == nullptr) {
        block_index = get_block_index_from_cache(account.get_account(), m_lock_cache, match_func);
    }
    if (block_index != nullptr) {
        return block_index;
    }
    return nullptr;
}

base::xauto_ptr<base::xvblock_t> xuncommitted_subblock_cache_t::load_block_object(const base::xvaccount_t & account, const xblock_match_base_t & match_func) const {
    base::xauto_ptr<base::xvbindex_t> block_index = load_block_index(account, match_func);
    if (block_index != nullptr) {
        base::xvblock_t * raw_block_ptr = block_index->get_this_block();
        raw_block_ptr->add_ref();
        return raw_block_ptr;
    }
    return nullptr;
}

base::xblock_vector xuncommitted_subblock_cache_t::load_blocks_object(const base::xvaccount_t & account, uint64_t height) const {
    std::vector<base::xvblock_t *> block_list;

    base::xvbindex_vector block_index_vec = load_blocks_index(account, height);
    for (auto & block_index : block_index_vec.get_vector()) {
        auto raw_block = block_index->get_this_block();
        raw_block->add_ref();
        block_list.push_back(raw_block);
    }
    return block_list;
}

base::xvbindex_vector xuncommitted_subblock_cache_t::load_blocks_index(const base::xvaccount_t & account, const uint64_t height) const {
    std::vector<base::xvbindex_t *> block_list;
    auto cert_blocks = get_block_indexs_from_cache(account.get_account(), m_cert_cache, height);
    for (auto & block : cert_blocks) {
        block->add_ref();
        block_list.push_back(block);
    }

    auto lock_blocks = get_block_indexs_from_cache(account.get_account(), m_lock_cache, height);
    for (auto & block : lock_blocks) {
        block->add_ref();
        block_list.push_back(block);
    }
    return block_list;
}

void xuncommitted_subblock_cache_t::add_blocks_to_cache(const std::vector<base::xvblock_ptr_t> & blocks,
                                                        std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & block_cache) {
    for (auto & block : blocks) {
        std::map<std::string, base::xvbindex_t *> block_map;
        xdbg("xuncommitted_subblock_cache_t::add_blocks_to_cache table block:%s", block->dump().c_str());
        if (block->get_block_class() == base::enum_xvblock_class_light && block->get_block_level() == base::enum_xvblock_level_table) {
            std::vector<base::xvblock_ptr_t> sub_blocks;
            if (block->extract_sub_blocks(sub_blocks)) {
                for (auto & subblock : sub_blocks) {
                    xdbg("xuncommitted_subblock_cache_t::add_blocks_to_cache sub block:%s", subblock->dump().c_str());
                    base::xvbindex_t * new_index = new base::xvbindex_t(*(subblock.get()));
                    new_index->add_ref();
                    new_index->reset_this_block(subblock.get());
                    block_map[subblock->get_account()] = new_index;
                }
            } else {
                xerror("add_blocks_to_cache extract_sub_blocks fail. block:%s", block->dump().c_str());
            }
        }
        // block_map can be empty.
        block_cache[block->get_viewid()] = block_map;
    }
}
};  // namespace store
};  // namespace top
