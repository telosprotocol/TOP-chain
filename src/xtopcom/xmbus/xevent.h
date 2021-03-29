// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include "xbasic/xns_macro.h"
#include "xbase/xutl.h"

NS_BEG2(top, mbus)

#define DEFINE_SHARED_PTR(type) \
    using type##_ptr_t = std::shared_ptr<type##_t>

enum xevent_major_type_t {
    xevent_major_type_none,
    xevent_major_type_timer,
    xevent_major_type_chain_timer,
    xevent_major_type_store,
    xevent_major_type_sync_executor,
    xevent_major_type_network,
    // for dispatch message, due to multi-vnode
    // there is a dispatcher to send all message
    xevent_major_type_dispatch,
    // major type for all deceit types        
    xevent_major_type_deceit,
    xevent_major_type_consensus,
    xevent_major_type_transaction,
    xevent_major_type_behind,
    xevent_major_type_vnode,
    xevent_major_type_account,
    xevent_major_type_role,
    xevent_major_type_blockfetcher,
    xevent_major_type_sync,
    xevent_major_type_max
};

class xevent_t {
public:

    enum error_type {
        succ,
        fail
    };

    enum direction_type {
        to_listener,
        to_sourcer
    };

    xevent_t(
            xevent_major_type_t _major_type,
            int _minor_type = 0,
            direction_type dir = to_listener,
            bool _sync = true) :
    major_type(_major_type),
    minor_type(_minor_type),
    direction(dir),
    sync(_sync) {
        m_time = base::xtime_utl::gmttime_ms();
    }

    virtual ~xevent_t() {
    }

    xevent_major_type_t major_type;
    int minor_type;
    direction_type direction;
    bool sync;
    error_type err{succ};
    int64_t m_time;
};

DEFINE_SHARED_PTR(xevent);

NS_END2
