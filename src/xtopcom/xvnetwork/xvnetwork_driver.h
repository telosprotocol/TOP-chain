// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash_list.hpp"
#include "xbasic/xmemory.hpp"
#include "xcommon/xnode_type.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnetwork/xvnetwork_driver_base.h"

#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

NS_BEG2(top, vnetwork)

class xtop_vnetwork_driver final
  : public xvnetwork_driver_base_t
  , public std::enable_shared_from_this<xtop_vnetwork_driver> {
private:
    observer_ptr<xvhost_face_t> m_vhost;
    observer_ptr<election::cache::xdata_accessor_face_t> m_election_cache_data_accessor;

    common::xnode_address_t m_address;
    mutable std::mutex m_address_mutex{};

    // access seeds used for communicating with other vnodes in different zone/cluster.
    mutable std::mutex m_access_seeds_mutex{};
    std::unordered_map<common::xnode_type_t, std::vector<common::xnode_address_t>> m_access_seeds{};

    mutable std::mutex m_message_cache_mutex{};
    xhash_list2_t<std::uint64_t, std::time_t> m_message_cache{10000};
    common::xelection_round_t m_joined_election_round;

public:
    xtop_vnetwork_driver(xtop_vnetwork_driver const &) = delete;
    xtop_vnetwork_driver & operator=(xtop_vnetwork_driver const &) = delete;
    xtop_vnetwork_driver(xtop_vnetwork_driver &&) = delete;
    xtop_vnetwork_driver & operator=(xtop_vnetwork_driver &&) = delete;
    ~xtop_vnetwork_driver() override = default;

    xtop_vnetwork_driver(observer_ptr<xvhost_face_t> const & vhost,
                         observer_ptr<election::cache::xdata_accessor_face_t> const & election_data_accessor,
                         common::xnode_address_t const & address,
                         common::xelection_round_t const & joined_election_round);

    void start() override;

    void stop() override;

    common::xnetwork_id_t network_id() const noexcept override;

    common::xnode_address_t address() const override;

    void send_to(common::xnode_address_t const & to, xmessage_t const & message, std::error_code & ec) override;

    void send_to(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) override;
    void broadcast(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) override;

    common::xnode_id_t const & account_address() const noexcept override;

    common::xnode_address_t parent_group_address() const override;

    std::map<common::xslot_id_t, data::xnode_info_t> neighbors_info2() const override final;

    std::map<common::xslot_id_t, data::xnode_info_t> parents_info2() const override final;

    std::map<common::xslot_id_t, data::xnode_info_t> children_info2(common::xgroup_id_t const & gid, common::xelection_round_t const & election_round) const override final;

    observer_ptr<xvhost_face_t> virtual_host() const noexcept override;

    common::xnode_type_t type() const noexcept override;

    std::vector<common::xnode_address_t> archive_addresses(common::xenum_node_type node_type) const override;
    std::vector<common::xnode_address_t> fullnode_addresses(std::error_code & ec) const override;
    std::vector<common::xnode_address_t> relay_addresses(std::error_code & ec) const override;

    std::vector<std::uint16_t> table_ids() const override final;
    common::xelection_round_t const & joined_election_round() const override;

private:
    void on_vhost_message_data_ready(common::xnode_address_t const & src, xmessage_t const & msg, common::xlogic_time_t const msg_time);
};
using xvnetwork_driver_t = xtop_vnetwork_driver;

NS_END2
