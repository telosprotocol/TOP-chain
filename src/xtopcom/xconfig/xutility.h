// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xlogic_time.h"

#include <ctime>

NS_BEG2(top, config)

common::xlogic_time_t gmttime_to_logic_time(std::time_t const gmttime) noexcept;

NS_END2
