// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvblock.h"

namespace top {
namespace store {

class xblock_checkpoint {
public:
    static bool check_block(base::xvblock_t * block);
};

}  // namespace store
}  // namespace top
