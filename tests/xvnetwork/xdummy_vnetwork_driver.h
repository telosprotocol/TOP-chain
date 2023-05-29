// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvnetwork/xvnetwork_driver_face.h"

using top::vnetwork::xmessage_t;
using top::vnetwork::xvhost_face_t;
using top::vnetwork::xvnetwork_message_ready_callback_t;
using top::vnetwork::xvnode_address_t;

NS_BEG3(top, tests, vnetwork)

class xtop_dummy_vnetwork_driver : public top::vnetwork::xvnetwork_driver_face_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_vnetwork_driver);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_vnetwork_driver);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_vnetwork_driver);

    void start() override {}

    void stop() override {}

    void register_message_ready_notify(common::xmessage_category_t const, xvnetwork_message_ready_callback_t) override {}

    void unregister_message_ready_notify(common::xmessage_category_t const) override {}

    common::xnetwork_id_t network_id() const noexcept override { return common::xtopchain_network_id; }

    xvnode_address_t address() const override {
        return {};
    }

    void send_to(xvnode_address_t const & to, xmessage_t const & message, std::error_code & ec) override {}

    void send_to(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) override {}

    void broadcast(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) override {}

    // void send_to(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) override {}
    // void broadcast(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) override {}

    // void broadcast_to(xvnode_address_t const & dst, xmessage_t const & message) {}

    common::xaccount_address_t const & account_address() const noexcept override {
        static common::xaccount_address_t nid;
        return nid;
    }

    xvnode_address_t parent_group_address() const override { return {}; }

    observer_ptr<xvhost_face_t> virtual_host() const noexcept override { return observer_ptr<xvhost_face_t>{}; }

    common::xnode_type_t type() const noexcept override { return common::xnode_type_t::invalid; }

    std::vector<xvnode_address_t> archive_addresses(common::xnode_type_t) const override { return {}; }
    std::vector<xvnode_address_t> fullnode_addresses(std::error_code &) const override { return {}; }
    std::vector<xvnode_address_t> relay_addresses(std::error_code &) const override { return {}; }

    std::vector<std::uint16_t> table_ids() const override { return {}; }

    std::uint64_t start_time() const noexcept { return 0; }

    std::map<common::xslot_id_t, data::xnode_info_t> neighbors_info2() const override { return {}; }

    std::map<common::xslot_id_t, data::xnode_info_t> parents_info2() const override { return {}; }

    std::map<common::xslot_id_t, data::xnode_info_t> children_info2(common::xgroup_id_t const & gid, common::xelection_round_t const & version) const override { return {}; }

    common::xelection_round_t const & joined_election_round() const override {
        static common::xelection_round_t joined_election_round;
        return joined_election_round;
    }
};
using xdummy_vnetwork_driver_t = xtop_dummy_vnetwork_driver;

NS_END3
