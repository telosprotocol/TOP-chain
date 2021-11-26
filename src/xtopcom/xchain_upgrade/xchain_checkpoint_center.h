// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

namespace top {
namespace chain_checkpoint {

struct xtop_checkpoint_info {
    uint64_t height;
    std::string hash;
};
using xcheckpoint_info_t = xtop_checkpoint_info;

// vector<timer_height, <account, info>>
using xcheckpoints_t = std::vector<std::pair<uint64_t, std::map<std::string, xcheckpoint_info_t>>>;

class xtop_chain_checkpoint {
public:
    static void init();
    static xcheckpoints_t const & checkpoints();

private:
    static xcheckpoints_t m_checkpoints;
};
using xchain_checkpoint_t = xtop_chain_checkpoint;

}  // namespace chain_checkpoint
}  // namespace top
