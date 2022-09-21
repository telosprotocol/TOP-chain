// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <string>
#include "xmbus/xevent.h"
#include "xdata/xdata_common.h"
#include "xdata/xblock.h"

NS_BEG2(top, mbus)

class xevent_state_sync_t : public xbus_event_t {
public:

    enum _minor_type_ {
        none
    };

    xevent_state_sync_t(
            const common::xaccount_address_t & _table_addr,
            const xhash256_t & _root_hash,
            const xhash256_t & _table_state_hash,
            std::error_code _ec,
            direction_type dir = to_listener,
            bool _sync = true) :
    xbus_event_t(xevent_major_type_state_sync,
    (int) none,
    dir,
    _sync),
    table_addr(_table_addr),
    root_hash(_root_hash),
    table_state_hash(_table_state_hash),
    ec(_ec) {
    }

    common::xaccount_address_t table_addr;
    xhash256_t root_hash;
    xhash256_t table_state_hash;
    std::error_code ec;
};

using xevent_state_sync_ptr_t = xobject_ptr_t<xevent_state_sync_t>;

NS_END2