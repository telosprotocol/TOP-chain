// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"
#include "xmbus/xmessage_bus.h"
#include "xplugin/xplugin_manager.h"
#include "xtxpool_v2/xreceiptid_state_cache.h"
#include "xvledger/xvcertauth.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

class xtxpool_resources_face {
public:
    virtual base::xvblockstore_t * get_vblockstore() const = 0;
    virtual base::xvcertauth_t * get_certauth() const = 0;
    virtual mbus::xmessage_bus_face_t * get_bus() const = 0;
    virtual xreceiptid_state_cache_t & get_receiptid_state_cache() = 0;
    virtual data::xplugin_manager_t * get_plugin_mgr() const = 0;
};

NS_END2
