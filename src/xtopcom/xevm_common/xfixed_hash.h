// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/fixed_hash.h"

namespace top {
namespace evm_common {

// Common types of FixedHash.
using xh2048_t = h2048;
using xh1024_t = h1024;
using xh520_t = h520;
using xh512_t = h512;
using xh256_t = h256;
using xh160_t = h160;
using xh128_t = h128;
using xh64_t = h64;
using xh512s_t = std::vector<xh512_t>;
using xh256s_t = std::vector<xh256_t>;
using xh160s_t = std::vector<xh160_t>;


}  // namespace evm_common
}  // namespace top
