// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xvnetwork/xmessage_callback_hub.h"

NS_BEG2(top, vnetwork)

xtop_message_callback_hub::xtop_message_callback_hub(observer_ptr<xvhost_face_t> const & vhost)
    : m_vhost{ vhost } {
}

void
xtop_message_callback_hub::start() {
    assert(m_vhost != nullptr);
    m_vhost->register_message_ready_notify(common::xnode_address_t{ common::build_network_broadcast_sharding_address(m_vhost->network_id()) },
                                           std::bind(&xtop_message_callback_hub::on_message,
                                           shared_from_this(),
                                           std::placeholders::_1,
                                           std::placeholders::_2,
                                           std::placeholders::_3));
}

void
xtop_message_callback_hub::stop() {
    assert(m_vhost != nullptr);
    m_vhost->unregister_message_ready_notify(common::xnode_address_t{ common::build_network_broadcast_sharding_address(m_vhost->network_id()) });
}

common::xnetwork_id_t const &
xtop_message_callback_hub::network_id() const noexcept {
    assert(m_vhost != nullptr);
    return m_vhost->network_id();
}


void
xtop_message_callback_hub::register_message_ready_notify(xmessage_ready_callback_t const & cb) {
    m_callbacks.push_back(cb);
}

void
xtop_message_callback_hub::on_message(common::xnode_address_t const & sender,
                                      xmessage_t const & message,
                                      std::uint64_t const logic_time) {
    for (auto const & call : m_callbacks) {
        try {
            call(sender, message, logic_time);
        } catch (std::exception const & eh) {
            xwarn("[xvnetwork] caught exception: %s", eh.what());
        }
    }
}

NS_END2
