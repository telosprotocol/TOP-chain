// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"
#include "xvledger/xvcertauth.h"
#include "xstore/xstore_face.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xindexstore/xindexstore_face.h"
#include "xmbus/xmessage_bus.h"

NS_BEG2(top, xtxpool_v2)

class xtxpool_resources_face {
public:
    virtual store::xstore_face_t * get_store() const = 0;
    virtual base::xvblockstore_t * get_vblockstore() const = 0;
    virtual base::xvcertauth_t * get_certauth() const =0;
    virtual store::xindexstorehub_t * get_indexstorehub() const = 0;
    virtual mbus::xmessage_bus_face_t * get_bus() const = 0;
};

NS_END2
