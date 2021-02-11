// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xrunnable.h"
#include "xnetwork/xendpoint.h"
#include "xnetwork/xmessage_transmission_property.h"
#include "xnetwork/xsocket_data_ready_callback.h"


NS_BEG2(top, network)

class xtop_socket_face : public xbasic_runnable_t<xtop_socket_face>
{
public:
    xtop_socket_face()                                     = default;
    xtop_socket_face(xtop_socket_face const &)             = delete;
    xtop_socket_face & operator=(xtop_socket_face const &) = delete;
    xtop_socket_face(xtop_socket_face &&)                  = default;
    xtop_socket_face & operator=(xtop_socket_face &&)      = default;
    ~xtop_socket_face() override                           = default;

    virtual
    xendpoint_t
    local_endpoint() const noexcept = 0;

    virtual
    void
    send_to(xendpoint_t const & peer,
            xbyte_buffer_t data,
            xdeliver_property_t const & deliver_property) = 0;

    virtual
    void
    register_data_ready_notify(xsocket_data_ready_callback_t callback) = 0;

    virtual
    void
    unregister_data_ready_notify() = 0;
};

using xsocket_face_t = xtop_socket_face;

NS_END2
