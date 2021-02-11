// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cassert>
#include <limits.h>

#include "xblockchain_checker.h"

NS_BEG2(top, test)

xblockchain_checker_t::xblockchain_checker_t(std::vector<xtestnode_t *> const &nodes)
    : m_nodes(nodes) {}

xblockchain_checker_t::~xblockchain_checker_t() {
    for (auto pair : m_cache) {
        for (auto b : pair.second) {
            b->release_ref();
        }
    }
}

void xblockchain_checker_t::do_check() {
    uint32_t wrong_counts = update_cache();

    uint32_t min = UINT_MAX;
    for (auto pair : m_cache) {
        if (min > pair.second.size()) {
            min = pair.second.size();
        }
    }

    bool vcheck = true;
    if (min > 1) {
        vcheck = vertical_check(min);
    }

    xdbg("same blocks %d, hcheck %s [wrong chain number %d], vcheck %s", min, wrong_counts == 0 ? "true" : "false", wrong_counts, vcheck ? "true" : "false");
    assert(wrong_counts == 0);
}

uint32_t xblockchain_checker_t::update_cache() {
    uint32_t wrong{};
    for (auto n : m_nodes) {
        auto store = n->get_blockstore();
        auto it = m_cache.find(n->get_account());
        if (it == m_cache.end()) {
            m_cache[n->get_account()] = {};
            it = m_cache.find(n->get_account());
        }
        uint64_t height = 0;
        if (!it->second.empty()) {
            height = (*(it->second.rbegin()))->get_height() + 1;
        }
        for (; height < UINT64_MAX; height++) {
            auto block = store->load_block_object(n->get_chainid(), n->get_account(), height);
            if (block == nullptr) {
                break;
            }
            if (!it->second.empty()) {
                auto pre_block = *(it->second.rbegin());
                // check height
                if (pre_block->get_height() + 1 != block->get_height())
                    break;
                // check hash
                if (block->get_last_block_hash() != pre_block->get_block_hash()) {
                    xwarn("[xblockchain_checker_t] hash not connected! account %s, pre height %lu, height %lu, pre hash %s, hash %s",
                          n->get_account().c_str(), pre_block->get_height(), block->get_height(),
                          pre_block->get_block_hash().c_str(), block->get_last_block_hash().c_str());
                    wrong++;
                    break;
                }
            }
            block->add_ref();
            it->second.push_back(block);
        }
    }
    return wrong;
}

bool xblockchain_checker_t::vertical_check(uint32_t max) const {
    for (size_t i = 0; i < max; i++) {
        base::xvblock_t *base = nullptr;
        base::xvblock_t *other = nullptr;
        for (auto pair : m_cache) {
            if (base == nullptr) {
                base = pair.second[i];
            } else {
                other = pair.second[i];
                if (base->get_block_hash() != other->get_block_hash()) {
                    return false;
                }
            }
        }
    }
    return true;
}

NS_END2