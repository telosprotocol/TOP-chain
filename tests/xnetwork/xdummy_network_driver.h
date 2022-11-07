// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xnetwork_driver_face.h"

using top::network::xdht_node_t;
using top::network::xnetwork_message_ready_callback_t;
using top::network::xnode_t;
using top::network::p2p::xdht_host_face_t;

NS_BEG3(top, tests, network)

class xtop_dummy_network_driver : public top::network::xnetwork_driver_face_t
{
protected:
    common::xaccount_address_t m_account_address{};

public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_network_driver);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_network_driver);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_network_driver);

    void
    start() override
    {
    }

    void
    stop() override
    {
    }

    bool
    running() const noexcept override
    {
        return true;
    }

    void
    running(bool const) noexcept override
    {
    }

    common::xnode_id_t const &
    host_node_id() const noexcept override
    {
        const static common::xnode_id_t nid = common::xnode_id_t("test1");
        if (m_account_address.has_value()) {
            return m_account_address;
        }

        return nid;
    }

    void account_address(common::xaccount_address_t account_address) {
        m_account_address = std::move(account_address);
    }

    common::xaccount_address_t const & account_address() const noexcept {
        return m_account_address;
    }

    top::network::xnode_t
    host_node() const noexcept
    {
        return {};
    }

    void send_to(common::xnode_id_t const &, xbyte_buffer_t const &) const {
        m_counter_send_to++;
        m_counter++;
    }

    void
    spread_rumor(xbyte_buffer_t const &) const
    {
        m_counter_spread_rumor++;
        m_counter++;
    }
    void
    spread_rumor(const common::xsharding_info_t &shardInfo, xbyte_buffer_t const &byte_msg) const
    {
        m_counter_spread_rumor++;
        m_counter++;
    };

    void
    forward_broadcast(const common::xsharding_info_t &shardInfo, common::xnode_type_t node_type, xbyte_buffer_t const &byte_msg) const
    {
        m_counter++;
        m_counter_forward_broadcast++;
    };

    void
    register_message_ready_notify(xnetwork_message_ready_callback_t) noexcept
    {
    }

    void
    unregister_message_ready_notify()
    {
    }

    bool
    p2p_bootstrap(std::vector<xdht_node_t> const &) const
    {
        return true;
    }

    std::vector<common::xnode_id_t>
    neighbors() const
    {
        return {};
    }

    std::size_t
    neighbor_size_upper_limit() const noexcept
    {
        return 0;
    }

public:
    mutable int m_counter_send_to;
    mutable int m_counter_spread_rumor;
    mutable int m_counter;
    mutable int m_counter_forward_broadcast;
};
using xdummy_network_driver_t = xtop_dummy_network_driver;

extern xdummy_network_driver_t xdummy_network_driver;

NS_END3
