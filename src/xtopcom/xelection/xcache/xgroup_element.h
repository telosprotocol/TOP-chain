// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xcommon/xnode_id.h"
#include "xcommon/xrotation_aware.h"
#include "xcommon/xversion.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xelection/xcache/xbasic_element.h"
#include "xelection/xcache/xelement_fwd.h"
#include "xelection/xcache/xnode_element.h"

#include <forward_list>
#include <map>
#include <memory>
#include <mutex>
#include <system_error>
#include <typeinfo>

NS_BEG3(top, election, cache)

class xtop_group_element final : public xbasic_element_t
                               , public common::xbasic_rotation_aware_t<xtop_group_element>
                               , public std::enable_shared_from_this<xtop_group_element> {
    using xbase_t = xbasic_element_t;

    mutable std::mutex m_node_elements_mutex{};
    std::map<common::xslot_id_t, std::shared_ptr<xnode_element_t>> m_node_elements{};

    std::weak_ptr<xcluster_element_t> m_cluster_element;

    mutable std::mutex m_associated_parent_group_mutex{};
    std::weak_ptr<xtop_group_element> m_associated_parent_group{};

    using xassociated_group_container_t = std::map<common::xlogic_time_t, std::weak_ptr<xgroup_element_t>, std::greater<common::xlogic_time_t>>;
    using xassociated_group_store_t = std::map<common::xgroup_id_t, xassociated_group_container_t>;

    mutable std::mutex m_associated_child_groups_mutex{};
    xassociated_group_store_t m_associated_child_groups{};

public:
    xtop_group_element(xtop_group_element const &)             = delete;
    xtop_group_element & operator=(xtop_group_element const &) = delete;
    xtop_group_element(xtop_group_element &&)                  = default;
    xtop_group_element & operator=(xtop_group_element &&)      = default;
    ~xtop_group_element() override                             = default;

    xtop_group_element(common::xversion_t const & version,
                       common::xgroup_id_t const & group_id,
                       std::uint16_t const sharding_size,
                       std::uint64_t const associated_election_blk_height,
                       std::shared_ptr<xcluster_element_t> const & cluster_element);

    std::shared_ptr<xcluster_element_t>
    cluster_element() const noexcept;

    bool
    exist(common::xslot_id_t const & slot_id) const;

    std::shared_ptr<xnode_element_t>
    node_element(common::xnode_id_t const & node_id, std::error_code & ec) const;

    std::shared_ptr<xnode_element_t>
    node_element(common::xnode_id_t const & node_id) const;

    std::shared_ptr<xnode_element_t>
    node_element(common::xslot_id_t const & slot_id, std::error_code & ec) const;

    std::shared_ptr<xnode_element_t>
    node_element(common::xslot_id_t const & slot_id) const;

    std::map<common::xslot_id_t, std::shared_ptr<xnode_element_t>>
    children() const;

    std::map<common::xslot_id_t, std::shared_ptr<xnode_element_t>> children(std::error_code & ec) const;

    bool
    enabled(common::xlogic_time_t const logic_time) const noexcept;

    bool
    contains(common::xnode_id_t const & node_id) const noexcept;

    void
    set_node_elements(std::map<common::xslot_id_t, data::election::xelection_info_bundle_t> const & election_data);

    void
    associate_parent_group(std::shared_ptr<xtop_group_element> const & parent_group,
                           std::error_code & ec);

    void
    associate_parent_group(std::shared_ptr<xtop_group_element> const & parent_group);

    std::shared_ptr<xtop_group_element>
    associated_parent_group() const;

    std::shared_ptr<xtop_group_element>
    associated_parent_group(std::error_code & ec) const;

    std::vector<std::shared_ptr<xtop_group_element>>
    associated_child_groups(common::xlogic_time_t const logic_time, std::error_code & ec) const;

private:
    void
    associate_child_group(std::shared_ptr<xtop_group_element> const & child_group,
                          std::error_code & ec);

    bool
    exist_with_lock_hold_outside(common::xslot_id_t const & slot_id) const noexcept;
};
using xgroup_element_t = xtop_group_element;

NS_END3
