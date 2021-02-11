// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xversion.hpp"

NS_BEG2(top, common)

struct xtag_network{};
using xversion_t = top::xversion_t<xtag_network, std::uint64_t>;

NS_END2
