// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xhash.hpp"

NS_BEG1(top)

template <std::size_t Bytes>
constexpr std::size_t xtop_hash<Bytes>::bytes_size;

template <std::size_t Bytes>
constexpr std::size_t xtop_hash<Bytes>::bits_size;

template
class xtop_hash<32>;

NS_END1
