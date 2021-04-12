// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include "xbase/xns_macro.h"
#include "xbase/xvevent.h"

NS_BEG2(top, mbus)


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

NS_END2
