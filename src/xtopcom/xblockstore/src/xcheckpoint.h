// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>

#include <map>
#include <string>

namespace top {
namespace store {

struct xtop_checkpoint_cmp {
    bool operator()(const uint64_t & lhs, const uint64_t & rhs) const {
        return lhs < rhs;
    }
};
using xcheckpoint_cmp_t = xtop_checkpoint_cmp;

using xcheckpoints_t = std::map<uint64_t, std::string, xcheckpoint_cmp_t>;
using xcheckpoints_map_t = std::map<std::string, xcheckpoints_t>;

class xtop_chain_checkpoint {
public:
    static xcheckpoints_t const & checkpoints(std::string const & account);

private:
    static xcheckpoints_map_t m_checkpoints_map;
};
using xchain_checkpoint_t = xtop_chain_checkpoint;

}  // namespace store
}  // namespace top
