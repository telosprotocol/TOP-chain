#pragma once

#include <string>
#include <vector>
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xcons_transaction.h"
#include "tests/mock/xcertauth_util.hpp"

namespace top {
namespace mock {

using namespace top::data;

class xblock_generator {
 public:
    xblock_generator(const std::string & address)
    : m_account(address) {}
 public:
    const std::string &                 get_account() const {return m_account;}
    const std::vector<xblock_ptr_t> &   get_blocks() const {return m_history_blocks;}

 public:  // generate methods
    const std::vector<xblock_ptr_t> &   generate_all_empty_blocks(uint64_t max_height) {
        base::xvblock_t* genesis = xblocktool_t::create_genesis_empty_block(m_account);
        xblock_ptr_t prev_block;
        prev_block.attach((xblock_t*)genesis);
        m_history_blocks.push_back(prev_block);
        for (uint64_t i = 1; i <= max_height; i++) {
            base::xvblock_t* next_block = xblocktool_t::create_next_emptyblock(prev_block.get());
            assert(next_block != nullptr);
            xcertauth_util::instance().do_multi_sign(next_block);
            prev_block.attach((xblock_t*)next_block);
            m_history_blocks.push_back(prev_block);
        }
        return get_blocks();
    }

 private:
    std::string                     m_account;
    std::vector<xblock_ptr_t>       m_history_blocks;
};

}
}
