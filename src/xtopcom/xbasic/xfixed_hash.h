// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/fixed_hash.h"

NS_BEG1(top)

// Common types of FixedHash.
using xh2048_t = evm_common::h2048;
using xh1024_t = evm_common::h1024;
using xh520_t = evm_common::h520;
using xh512_t = evm_common::h512;
using xh256_t = evm_common::h256;
using xh160_t = evm_common::h160;
using xh128_t = evm_common::h128;
using xh64_t = evm_common::h64;
using xh512s_t = std::vector<xh512_t>;
using xh256s_t = std::vector<xh256_t>;
using xh160s_t = std::vector<xh160_t>;

NS_END1
