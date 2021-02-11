// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction.h"
#include "xstore/xstore_face.h"

#include <string>

NS_BEG2(top, xtxpool)

enum enum_xtxpool_order_strategy {
    enum_xtxpool_order_strategy_default,
};

class xtxpool_resources_face {
public:
    virtual store::xstore_face_t * get_store() const = 0;
    virtual base::xvblockstore_t * get_vblockstore() const = 0;
    virtual mbus::xmessage_bus_face_t * get_mbus() const = 0;
    virtual base::xvcertauth_t * get_certauth() const = 0;
    virtual time::xchain_time_face_t * get_chain_timer() const = 0;
    virtual enum_xtxpool_order_strategy get_order_strategy() const = 0;
    virtual uint64_t get_receipt_valid_window() const = 0;
    virtual void set_receipt_valid_window_day(uint64_t day) = 0;
};

NS_END2
