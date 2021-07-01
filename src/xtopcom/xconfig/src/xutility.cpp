// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xconfig/xutility.h"

#include "xconfig/xpredefined_configurations.h"
#include "xvledger/xvblock.h"

NS_BEG2(top, config)

common::xlogic_time_t gmttime_to_logic_time(std::time_t const gmttime) noexcept {
    return static_cast<common::xlogic_time_t>((gmttime - base::TOP_BEGIN_GMTIME) / XGLOBAL_TIMER_INTERVAL_IN_SECONDS);
}

NS_END2
