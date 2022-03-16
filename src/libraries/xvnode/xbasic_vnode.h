// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xcommon/xaddress.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnode/xvnode_face.h"

#include <mutex>
#include <system_error>
#include <unordered_map>
#include <vector>

NS_BEG2(top, vnode)

class xtop_basic_vnode : public xvnode_face_t {
protected:
    // std::shared_ptr<election::cache::xgroup_element_t> m_associated_group_element;

    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr<election::cache::xdata_accessor_face_t> m_election_cache_data_accessor;

    common::xnode_address_t m_address;
    common::xminer_type_t m_miner_type{common::xminer_type_t::invalid};
    bool m_genesis{false};
    uint64_t m_raw_credit_score{0};
    common::xelection_round_t m_joined_election_round;

    mutable std::mutex m_neighbors_xip2_mutex{};
    mutable std::vector<common::xip2_t> m_neighbors_xip2;

    // mutable std::mutex m_associated_nodes_xip2_mutex{};
    // mutable std::unordered_map<common::xip_t, std::vector<common::xip2_t>> m_associated_nodes_xip2;  // only auditor / validator have 'associated' concept.

    mutable std::mutex m_associated_parent_xip2_mutex{};
    mutable std::vector<common::xip2_t> m_associated_parent_xip2;

    mutable std::mutex m_associated_child_xip2_mutex{};
    mutable std::unordered_map<common::xip_t, std::vector<common::xip2_t>> m_associated_child_xip2;

protected:
    xtop_basic_vnode() = default;

    explicit xtop_basic_vnode(common::xnode_address_t address,
                              common::xminer_type_t miner_type,
                              bool genesis,
                              uint64_t raw_credit_score,
                              common::xelection_round_t joined_election_round,
                              observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                              observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor) noexcept;

public:
    xtop_basic_vnode(xtop_basic_vnode const &) = delete;
    xtop_basic_vnode & operator=(xtop_basic_vnode const &) = delete;
    xtop_basic_vnode(xtop_basic_vnode &&) = default;
    xtop_basic_vnode & operator=(xtop_basic_vnode &&) = default;
    ~xtop_basic_vnode() = default;

    common::xnode_type_t type() const noexcept override;
    common::xminer_type_t miner_type() const noexcept override;
    bool genesis() const noexcept override;
    common::xnode_address_t const & address() const noexcept override;

    common::xelection_round_t const & joined_election_round() const noexcept override;

    void broadcast(common::xip2_t const & broadcast_dst, vnetwork::xmessage_t const & msg, std::error_code & ec) override;
    void send_to(common::xip2_t const & unicast_dst, vnetwork::xmessage_t const & msg, std::error_code & ec) override;

    common::xrotation_status_t status() const noexcept override;

    std::vector<common::xip2_t> neighbors_xip2(std::error_code & ec) const override;
    std::vector<common::xip2_t> associated_parent_nodes_xip2(std::error_code & ec) const override;
    std::vector<common::xip2_t> associated_child_nodes_xip2(common::xip2_t const & child_group_xip2, std::error_code & ec) const override;

    //std::vector<common::xip2_t> associated_nodes_xip2(common::xip_t const & group_xip, std::error_code & ec) const override;
    //std::vector<common::xip2_t> nonassociated_nodes_xip2(common::xip_t const & group_xip, std::error_code & ec) const override;

    uint64_t raw_credit_score() const noexcept override;
};
using xbasic_vnode_t = xtop_basic_vnode;

NS_END2
