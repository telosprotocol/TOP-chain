// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <map>
#include "xsync/xsync_dispatcher.h"
NS_BEG2(top, sync)

bool xsync_session_handler_t::handler(common::xmessage_id_t msgid, uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    vnetwork::xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {
    if (is_request(msgid)) {
        on_request(msgid, msg_size, from_address, network_self, header, stream, msg_hash, recv_time);
        return true;
    }

    if (is_response(msgid)) {
        on_response(msgid, msg_size, from_address, network_self, header, stream, msg_hash, recv_time);
        return true;
    }

    return false;
}

void xsync_session_dispatcher_t::handle(common::xmessage_id_t msgid, uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time) {

    for (auto it = m_handlers.begin(); it != m_handlers.end(); it++){
        if (it->second->handler(msgid, msg_size, from_address, network_self, header, stream, msg_hash, recv_time)) {
            return;
        }
    }

    // not support
    return;
}

void xsync_session_dispatcher_t::add(xobject_ptr_t<xsync_session_handler_t> handler) {
    for (auto it = m_handlers.begin(); it != m_handlers.end(); it++){
        if (handler->id() == it->second->id()) {
            return;
        }
    }

    m_handlers.emplace(handler->id(), handler);
}

void xsync_session_dispatcher_t::remove(xobject_ptr_t<xsync_session_handler_t> handler) {
    for (auto it = m_handlers.begin(); it != m_handlers.end(); it++){
        if (handler->id() == it->second->id()) {
            m_handlers.erase(it);
            return;
        }
    }
}

NS_END2
