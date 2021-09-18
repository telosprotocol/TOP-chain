// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbase/xvevent.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, mbus)

using xevent_ptr_t = xobject_ptr_t<xevent_t>;

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

class xbus_event_t : public xevent_t {
public:
    explicit xbus_event_t(int _major_type,
                     int _minor_type = 0,
                     direction_type dir = to_listener,
                     bool _sync = true) 
        : xevent_t(_major_type, _minor_type, dir, _sync) {
            auto tag = (metrics::E_SIMPLE_METRICS_TAG)(metrics::xevent_begin + _major_type);
            xassert(tag >= top::metrics::xevent_begin && tag <= top::metrics::xevent_end);
            XMETRICS_GAUGE(tag, 1);
        }
    virtual ~xbus_event_t() {
        auto tag = (metrics::E_SIMPLE_METRICS_TAG)(metrics::xevent_begin + major_type);
        xassert(tag >= top::metrics::xevent_begin && tag <= top::metrics::xevent_end);
        XMETRICS_GAUGE(tag, -1);
    }
};

NS_END2
