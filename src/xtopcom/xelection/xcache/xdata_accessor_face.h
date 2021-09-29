// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xversion.h"
#include "xdata/xelection/xelection_data_struct.h"
#include "xdata/xnode_info.h"
#include "xelection/xcache/xcluster_element.h"
#include "xelection/xcache/xnode_element.h"

#include <system_error>
#include <unordered_map>

NS_BEG3(top, election, cache)

class xtop_data_accessor_face {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_data_accessor_face);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_data_accessor_face);
    XDECLARE_DEFAULTED_VIRTULA_DESTRUCTOR(xtop_data_accessor_face);

    virtual common::xnetwork_id_t network_id() const noexcept = 0;

    virtual std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> update_zone(common::xzone_id_t const & zone_id,
                                                                                             data::election::xelection_result_store_t const & election_result_store,
                                                                                             std::uint64_t const associated_blk_height,
                                                                                             std::error_code & ec) = 0;

    XATTRIBUTE_DEPRECATED virtual std::map<common::xslot_id_t, data::xnode_info_t> sharding_nodes(common::xgroup_address_t const & address,
                                                                                                  common::xelection_round_t const & election_round,
                                                                                                  std::error_code & ec) const = 0;

    virtual std::map<common::xslot_id_t, data::xnode_info_t> group_nodes(common::xgroup_address_t const & group_address,
                                                                         common::xlogic_epoch_t const & group_logic_epoch,
                                                                         std::error_code & ec) const = 0;

    XATTRIBUTE_DEPRECATED virtual common::xnode_address_t parent_address(common::xgroup_address_t const & child_address,
                                                                         common::xelection_round_t const & child_election_round,
                                                                         std::error_code & ec) const noexcept = 0;

    virtual common::xnode_address_t parent_address(common::xgroup_address_t const & child_address,
                                                   common::xlogic_epoch_t const & child_logic_epoch,
                                                   std::error_code & ec) const noexcept = 0;

    virtual std::vector<common::xnode_address_t> child_addresses(common::xgroup_address_t const & parent_group_address,
                                                                 common::xlogic_epoch_t const & parent_logic_epoch,
                                                                 std::error_code & ec) const noexcept = 0;

    virtual std::shared_ptr<xnode_element_t> node_element(common::xgroup_address_t const & address,
                                                          common::xlogic_epoch_t const & logic_epoch,
                                                          common::xslot_id_t const & slot_id,
                                                          std::error_code & ec) const = 0;

    virtual common::xaccount_address_t account_address_from(common::xip2_t const & xip2, std::error_code & ec) const = 0;

    XATTRIBUTE_DEPRECATED virtual std::shared_ptr<xgroup_element_t> group_element(common::xgroup_address_t const & group_address,
                                                                                  common::xelection_round_t const & election_round,
                                                                                  std::error_code & ec) const = 0;

    virtual std::shared_ptr<xgroup_element_t> group_element(common::xgroup_address_t const & group_address,
                                                            common::xlogic_epoch_t const & group_logic_epoch,
                                                            std::error_code & ec) const = 0;

    virtual std::shared_ptr<xgroup_element_t> group_element_by_height(common::xgroup_address_t const & group_address,
                                                                      uint64_t const election_blk_height,
                                                                      std::error_code & ec) const = 0;

    virtual std::shared_ptr<xgroup_element_t> group_element_by_logic_time(common::xgroup_address_t const & group_address,
                                                                          common::xlogic_time_t const logic_time,
                                                                          std::error_code & ec) const = 0;

    XATTRIBUTE_DEPRECATED virtual std::shared_ptr<xgroup_element_t> parent_group_element(common::xgroup_address_t const & child_sharding_address,
                                                                                         common::xelection_round_t const & child_group_election_round,
                                                                                         std::error_code & ec) const = 0;

    virtual std::shared_ptr<xgroup_element_t> parent_group_element(common::xgroup_address_t const & child_group_address,
                                                                   common::xlogic_epoch_t const & child_logical_version,
                                                                   std::error_code & ec) const = 0;

    virtual common::xelection_round_t election_epoch_from(common::xip2_t const & xip2, std::error_code & ec) const = 0;
};
using xdata_accessor_face_t = xtop_data_accessor_face;

NS_END3
