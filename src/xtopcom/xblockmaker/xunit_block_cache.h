// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvblock.h"
#include "xdata/xblock.h"

#include <string>
#include <vector>

NS_BEG2(top, blockmaker)

class xunit_block_set {
public:
   xunit_block_set(uint64_t height, uint64_t viewid, std::vector<xobject_ptr_t<base::xvblock_t>> unit_blocks) : m_height(height), m_viewid(viewid) {
      for (auto block : unit_blocks) {
         data::xblock_ptr_t unit_block = data::xblock_t::raw_vblock_to_object_ptr(block.get());
         m_unit_blocks[block->get_account()] = unit_block;
      }
   }
   uint64_t height() const {return m_height;}
   uint64_t viewid() const {return m_viewid;}
   data::xblock_ptr_t get_unit_block(const std::string & account_addr) const;
private:
    uint64_t m_height{0};
    uint64_t m_viewid{0};
    std::map<std::string, data::xblock_ptr_t> m_unit_blocks;
};

class xunit_block_cache {
public:
    void update_table_blocks(std::vector<data::xblock_ptr_t> table_blocks);
    std::map<uint64_t, data::xblock_ptr_t> get_unit_blocks(const std::string account_addr) const;
private:
    std::vector<std::shared_ptr<xunit_block_set>> m_unit_cache;
};

NS_END2
