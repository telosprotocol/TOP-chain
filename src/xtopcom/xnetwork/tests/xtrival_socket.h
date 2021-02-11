// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xsocket_face.h"

NS_BEG3(top, network, tests)

class xtop_trival_socket final : public xsocket_face_t
{
public:
    void
    start() override {
    }

    void
    stop() override {
    }

    bool
    running() const noexcept override {
        return true;
    }

    void
    running(bool const) noexcept override {
    }

    xendpoint_t
    local_endpoint() const noexcept override {
        return { "127.0.0.1", 10000 };
    }

    void
    send_to(xendpoint_t const &,
            xbyte_buffer_t,
            xdeliver_property_t const &) override {
    }

    void
    register_data_ready_notify(xsocket_data_ready_callback_t) override {
    }

    void
    unregister_data_ready_notify() override {
    }
};
using xtrival_socket_t = xtop_trival_socket;

NS_END3
