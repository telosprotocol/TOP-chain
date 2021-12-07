// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblockstore/src/xvcheckpoint.h"

#include "xchain_checkpoint/xchain_checkpoint.h"

namespace top {
namespace store {

bool xblock_checkpoint::check_block(base::xvblock_t * block) {
    auto const & account = block->get_account();
    auto const & checkpoints = chain_checkpoint::xchain_checkpoint_t::checkpoints(account);
    if (checkpoints.empty()) {
        return true;
    }

    auto const & block_clock = block->get_clock();
    auto const & block_height = block->get_height();
    auto const & block_hash = base::xstring_utl::to_hex(block->get_block_hash());
    auto it = checkpoints.find(block_clock);
    if (it != checkpoints.end()) {
        if (it->second.height == block_height && it->second.hash != block_hash) {
            return false;
        }
    }
    return true;
}

}  // namespace store
}  // namespace top
