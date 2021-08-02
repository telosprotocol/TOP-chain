// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xelection_result_keepalive_strategy.h"
#include "xcommon/xlogic_time.h"
#include "xelection/xcache/xbasic_element.h"
#include "xelection/xcache/xelement_fwd.h"
#include "xelection/xcache/xgroup_element.h"
#include "xelection/xcache/xgroup_update_result.h"

#include <map>
#include <memory>
#include <mutex>

NS_BEG3(top, election, cache)

class xtop_cluster_element final
  : public xbasic_element_t
  , public std::enable_shared_from_this<xtop_cluster_element> {
private:
    using xbase_t = xbasic_element_t;
    using xgroup_info_container_t = std::map<common::xlogic_time_t, std::shared_ptr<xgroup_element_t>, std::greater<common::xlogic_time_t>>;  // key is the group start time
    using xgroup_elements_container_t = std::map<common::xgroup_id_t, xgroup_info_container_t>;

    std::weak_ptr<xzone_element_t> m_zone_element;
    mutable std::mutex             m_group_elements_mutex{};
    xgroup_elements_container_t    m_group_elements{};

public:
    xtop_cluster_element(xtop_cluster_element const &) = delete;
    xtop_cluster_element & operator=(xtop_cluster_element const &) = delete;
    xtop_cluster_element(xtop_cluster_element &&) = default;
    xtop_cluster_element & operator=(xtop_cluster_element &&) = default;
    ~xtop_cluster_element() override = default;

    xtop_cluster_element(common::xcluster_id_t const & cluster_id, std::shared_ptr<xzone_element_t> const & zone_element);

    std::shared_ptr<xzone_element_t> zone_element() const noexcept;

    bool exist(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const;

    xgroup_update_result_t add_group_element(common::xgroup_id_t const & group_id,
                                             common::xversion_t const &  version,
                                             common::xlogic_time_t const timestamp,
                                             common::xlogic_time_t const start_time,
                                             std::uint16_t const         sharding_size,
                                             std::uint64_t const         associated_election_blk_height,
                                             std::error_code &           ec);

    xgroup_update_result_t add_group_element(common::xgroup_id_t const & group_id,
                                             common::xversion_t const &  version,
                                             common::xlogic_time_t const timestamp,
                                             common::xlogic_time_t const start_time,
                                             std::uint16_t const         sharding_size,
                                             std::uint64_t const         associated_election_blk_height);

    std::shared_ptr<xgroup_element_t> group_element(common::xgroup_id_t const & group_id, common::xversion_t const & version, std::error_code & ec) const;
    std::shared_ptr<xgroup_element_t> group_element(common::xgroup_id_t const & group_id, common::xlogic_epoch_t const & logic_epoch, std::error_code & ec) const;

    std::shared_ptr<xgroup_element_t> group_element(common::xgroup_id_t const & group_id, common::xversion_t const & version) const;
    std::shared_ptr<xgroup_element_t> group_element(common::xgroup_id_t const & group_id, common::xlogic_epoch_t const & logic_epoch) const;

    std::shared_ptr<xgroup_element_t> group_element_by_height(common::xgroup_id_t const & group_id, uint64_t const election_blk_height, std::error_code & ec) const;
    std::shared_ptr<xgroup_element_t> group_element_by_height(common::xgroup_id_t const & group_id, uint64_t const election_blk_height) const;

    std::shared_ptr<xgroup_element_t> group_element_by_logic_time(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time, std::error_code & ec) const;
    std::shared_ptr<xgroup_element_t> group_element_by_logic_time(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const;

    common::xversion_t group_version(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time, std::error_code & ec) const;
    common::xversion_t group_version(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const;

    std::vector<std::shared_ptr<xgroup_element_t>> children(common::xnode_type_t const child_type, common::xlogic_time_t const logic_time, std::error_code & ec) const;
    std::vector<std::shared_ptr<xgroup_element_t>> children(common::xnode_type_t const child_type, common::xlogic_time_t const logic_time) const;

private:
    bool exist_with_lock_hold_outside(common::xgroup_id_t const & group_id) const;
    bool exist_with_lock_hold_outside(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const;

    xgroup_elements_container_t::const_iterator find_with_lock_hold_outside(common::xgroup_id_t const & group_id) const;
    xgroup_info_container_t::const_iterator find_with_lock_hold_outside(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const;

    xgroup_update_result_t add_group_element_with_lock_hold_outside(common::xgroup_id_t const & group_id,
                                                                    common::xversion_t const &  version,
                                                                    common::xlogic_time_t const timestamp,
                                                                    common::xlogic_time_t const start_time,
                                                                    std::uint16_t const         sharding_size,
                                                                    std::uint64_t const         associated_election_blk_height,
                                                                    xgroup_info_container_t &   group_info_container,
                                                                    std::error_code &           ec);
};

NS_END3
