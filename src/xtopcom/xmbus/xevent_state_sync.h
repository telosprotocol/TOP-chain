// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address.h"
#include "xevm_common/xfixed_hash.h"
#include "xmbus/xevent.h"

#include <string>

NS_BEG2(top, mbus)

class xevent_state_sync_t : public xbus_event_t {
public:

    enum _minor_type_ {
        none
    };

    xevent_state_sync_t(
            const common::xaccount_address_t & _table_addr,
            const uint64_t h,
            const evm_common::xh256_t & _table_block_hash,
            const evm_common::xh256_t & _table_state_hash,
            const evm_common::xh256_t & _root_hash,
            std::error_code _ec,
            direction_type dir = to_listener,
            bool _sync = true) :
    xbus_event_t(xevent_major_type_state_sync,
    (int) none,
    dir,
    _sync),
    table_addr(_table_addr),
    height(h),
    table_block_hash(_table_block_hash),
    table_state_hash(_table_state_hash),
    root_hash(_root_hash),
    ec(_ec) {
    }

    common::xaccount_address_t table_addr;
    uint64_t height{0};
    evm_common::xh256_t table_block_hash;
    evm_common::xh256_t table_state_hash;
    evm_common::xh256_t root_hash;
    std::error_code ec;
};

using xevent_state_sync_ptr_t = xobject_ptr_t<xevent_state_sync_t>;

NS_END2