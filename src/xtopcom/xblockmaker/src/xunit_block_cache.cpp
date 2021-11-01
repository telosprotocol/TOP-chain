// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblockmaker/xunit_block_cache.h"

#include "xbasic/xmodule_type.h"

#include <cinttypes>
#include <string>

NS_BEG2(top, blockmaker)

data::xblock_ptr_t xunit_block_set::get_unit_block(const std::string & account_addr) const {
    auto iter = m_unit_blocks.find(account_addr);
    if (iter != m_unit_blocks.end()) {
        return iter->second;
    }
    return nullptr;
}

void xunit_block_cache::update_table_blocks(std::vector<data::xblock_ptr_t> table_blocks) {
    std::set<uint32_t> exist_table_height;

    uint64_t lower_table_height = 0xFFFFFFFFFFFFFFFF;
    uint64_t higher_table_height = 0;
    for (auto & block : table_blocks) {
        xinfo("xunit_block_cache::update_table_blocks block:%s", block->dump().c_str());
        if (lower_table_height > block->get_height()) {
            lower_table_height = block->get_height();
        }
        if (higher_table_height < block->get_height()) {
            higher_table_height = block->get_height();
        }
    }

    for (auto iter = m_unit_cache.begin(); iter != m_unit_cache.end();) {
        if ((*iter)->height() > higher_table_height) {
            xwarn("xunit_block_cache::update_table_blocks forked. table:%s,height:%llu:%llu", table_blocks[0]->get_account().c_str(), (*iter)->height(), higher_table_height);
            iter = m_unit_cache.erase(iter);
            continue;
        }
        if ((*iter)->height() < lower_table_height) {
            iter = m_unit_cache.erase(iter);
            continue;
        }
        bool iter_changed = false;
        for (auto & block : table_blocks) {
            if ((*iter)->height() == block->get_height()) {
                if ((*iter)->viewid() != block->get_viewid()) {
                    xwarn("xunit_block_cache::update_table_blocks table forked.old viewid:%llu,block:%s", (*iter)->viewid(), block->dump().c_str());
                    iter = m_unit_cache.erase(iter);
                    iter_changed = true;
                } else {
                    exist_table_height.insert(block->get_height());
                }
                break;
            }
        }
        if (!iter_changed) {
            iter++;
        }
    }

    for (auto & block : table_blocks) {
        if (block->get_block_class() != base::enum_xvblock_class_light) {
            continue;
        }
        auto it = exist_table_height.find(block->get_height());
        if (it != exist_table_height.end()) {
            continue;
        }
        std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
        if (block->extract_sub_blocks(sub_blocks)) {
            std::shared_ptr<xunit_block_set> unit_set = std::make_shared<xunit_block_set>(block->get_height(), block->get_viewid(), sub_blocks);
            m_unit_cache.push_back(unit_set);
        }
    }
}

std::map<uint64_t, data::xblock_ptr_t> xunit_block_cache::get_unit_blocks(const std::string account_addr) const {
    std::map<uint64_t, data::xblock_ptr_t> unit_blocks;
    for (auto & unit_set : m_unit_cache) {
        auto block = unit_set->get_unit_block(account_addr);
        if (block != nullptr) {
            unit_blocks[block->get_height()] = block;
        }
    }
    return unit_blocks;
}

NS_END2
