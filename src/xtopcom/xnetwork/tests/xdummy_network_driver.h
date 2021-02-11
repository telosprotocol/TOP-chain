// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xnetwork_driver_face.h"

NS_BEG3(top, network, tests)

class xtop_dummy_network_driver : public xnetwork_driver_face_t {
public:
    xtop_dummy_network_driver()                                              = default;
    xtop_dummy_network_driver(xtop_dummy_network_driver const &)             = delete;
    xtop_dummy_network_driver & operator=(xtop_dummy_network_driver const &) = delete;
    xtop_dummy_network_driver(xtop_dummy_network_driver &&)                  = default;
    xtop_dummy_network_driver & operator=(xtop_dummy_network_driver &&)      = default;
    ~xtop_dummy_network_driver()                                             = default;

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

    common::xnode_id_t const &
    host_node_id() const noexcept {
        static const common::xnode_id_t nid;
        return nid;
    }

    xnode_t
    host_node() const noexcept {
        return {};
    }

    void
    send_to(common::xnode_id_t const &,
            xbyte_buffer_t const &,
            xtransmission_property_t const &) const {
    }

    void
    spread_rumor(xbyte_buffer_t const &) const {
    }

    void
    register_message_ready_notify(xnetwork_message_ready_callback_t) noexcept {
    }

    void
    unregister_message_ready_notify() {
    }

    bool
    p2p_bootstrap(std::vector<xdht_node_t> const &) const {
        return true;
    }

    void
    direct_send_to(xnode_t const &,
                   xbyte_buffer_t,
                   xtransmission_property_t const &) {
    }

    std::vector<common::xnode_id_t>
    neighbors() const {
        return {};
    }

    std::size_t
    neighbor_size_upper_limit() const noexcept {
        return 0;
    }

    p2p::xdht_host_face_t const &
    dht_host() const noexcept {
        static p2p::xdht_host_face_t * fact = nullptr;
        return *fact;
    }
};
using xdummy_network_driver_t = xtop_dummy_network_driver;

NS_END3
