// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent.h"
#include "xvnetwork/xaddress.h"
#include "xvnetwork/xmessage.h"

NS_BEG2(top, mbus)

// <editor-fold defaultstate="collapsed" desc="event type network">

class xevent_network_t : public xbus_event_t {
public:

    enum _minor_type_ {
        none,
        synchro, // for synchronization component
    };

    xevent_network_t(
            const vnetwork::xvnode_address_t& _from_address,
            const vnetwork::xvnode_address_t& _network_self,
            const xbyte_buffer_t& msg,
            vnetwork::xmessage_t::message_type _msg_type,
            int _minor_type = (int) none,
            direction_type dir = to_listener,
            bool _sync = true) :
    xbus_event_t(xevent_major_type_network,
    _minor_type,
    dir,
    _sync),
    from_address(_from_address),
    network_self(_network_self),
    message(msg),
    msg_type(_msg_type) {
    }

    vnetwork::xvnode_address_t from_address;
    vnetwork::xvnode_address_t network_self;
    xbyte_buffer_t message;
    vnetwork::xmessage_t::message_type msg_type;
};

using xevent_network_ptr_t = xobject_ptr_t<xevent_network_t>;

class xevent_network_sync_t : public xevent_network_t {
public:

    xevent_network_sync_t(
            const vnetwork::xvnode_address_t& _from_address,
            const vnetwork::xvnode_address_t& _network_self, 
            const xbyte_buffer_t& msg,
            vnetwork::xmessage_t::message_type msg_type,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_network_t(
    _from_address,
    _network_self,
    msg,
    msg_type,
    synchro,
    dir,
    _sync) {
    }
};

using xevent_network_sync_ptr_t = xobject_ptr_t<xevent_network_sync_t>;

// </editor-fold>
        
NS_END2