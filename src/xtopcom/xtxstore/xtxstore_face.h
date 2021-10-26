// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver_fwd.h"
#include "xmbus/xmessage_bus.h"
#include "xvledger/xvtxstore.h"

NS_BEG2(top, txstore)

// base::xvtxstore_t * get_txstore();
base::xvtxstore_t * create_txstore(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<xbase_timer_driver_t> const & timer_driver);

NS_END2
