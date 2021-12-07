// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>

#include <map>
#include <string>

namespace top {
namespace chain_checkpoint {

struct xtop_checkpoint_info {
    uint64_t height;
    std::string hash;
};
using xcheckpoint_info_t = xtop_checkpoint_info;

struct xtop_checkpoint_cmp {
    bool operator()(const uint64_t & lhs, const uint64_t & rhs) const {
        return lhs < rhs;
    }
};
using xcheckpoint_cmp_t = xtop_checkpoint_cmp;

using xcheckpoint_unit_t = std::map<std::string, xcheckpoint_info_t>;
using xcheckpoints_t = std::map<uint64_t, xcheckpoint_unit_t, xcheckpoint_cmp_t>;

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
