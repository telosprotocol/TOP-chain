// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xthreading/xutility.h"
#include "xvnetwork/xvnetwork_driver_base.h"

#include <cassert>

NS_BEG2(top, vnetwork)

void
xtop_vnetwork_driver_base::register_message_ready_notify(common::xmessage_category_t const message_category,
                                                         xvnetwork_message_ready_callback_t cb) {
    // assert(!running());

    XLOCK_GUARD(m_callbacks_mutex) {
        m_callbacks.insert({ message_category, std::move(cb) });
    }
}

void
xtop_vnetwork_driver_base::unregister_message_ready_notify(common::xmessage_category_t const message_category) {
    // assert(!running());

    XLOCK_GUARD(m_callbacks_mutex) {
        m_callbacks.erase(message_category);
    }
}


NS_END2

