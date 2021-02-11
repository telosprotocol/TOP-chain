// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xvnetwork/xvhost_base.h"

#include <cassert>
#include <stdexcept>

NS_BEG2(top, vnetwork)

void
xtop_vhost_base::register_message_ready_notify(xvnode_address_t const & vaddr,
                                               xmessage_ready_callback_t cb) {
    assert(cb != nullptr);

    std::lock_guard<std::mutex> lock{ m_callbacks_mutex };
    auto & callback = m_callbacks[vaddr];
    assert(callback == nullptr);

    xdbg("[vnetwork] register message %s callback", vaddr.to_string().c_str());

    callback = std::move(cb);
}

void
xtop_vhost_base::unregister_message_ready_notify(xvnode_address_t const & vaddr) {
    std::lock_guard<std::mutex> lock{ m_callbacks_mutex };
    m_callbacks.erase(vaddr);
}

NS_END2
