// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>

#include <map>
#include <string>

namespace top {
namespace store {

using xcheckpoints_t = std::map<uint64_t, std::string>;
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
