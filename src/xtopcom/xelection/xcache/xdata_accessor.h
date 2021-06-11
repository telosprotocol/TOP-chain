// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xip.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xelection/xcache/xnetwork_element.h"

#include <memory>
#include <unordered_map>

NS_BEG3(top, election, cache)

class xtop_data_accessor final : public xdata_accessor_face_t {
private:
    std::shared_ptr<xnetwork_element_t> m_network_element;
    observer_ptr<time::xchain_time_face_t> m_logic_timer;

public:
    xtop_data_accessor(xtop_data_accessor const &) = delete;
    xtop_data_accessor & operator=(xtop_data_accessor const &) = delete;
    xtop_data_accessor(xtop_data_accessor &&) = default;
    xtop_data_accessor & operator=(xtop_data_accessor &&) = default;
    ~xtop_data_accessor() override = default;

    xtop_data_accessor(common::xnetwork_id_t const & network_id, observer_ptr<time::xchain_time_face_t> const & chain_timer);

    common::xnetwork_id_t network_id() const noexcept override;

    std::unordered_map<common::xcluster_address_t, xgroup_update_result_t> update_zone(common::xzone_id_t const & zone_id,
                                                                                       data::election::xelection_result_store_t const & election_result_store,
                                                                                       std::uint64_t const associated_blk_height,
                                                                                       std::error_code & ec) override;

    std::map<common::xslot_id_t, data::xnode_info_t> sharding_nodes(common::xcluster_address_t const & address,
                                                                    common::xversion_t const & version,
                                                                    std::error_code & ec) const override;

    common::xnode_address_t parent_address(common::xsharding_address_t const & child_address, common::xversion_t const & child_version, std::error_code & ec) const
        noexcept override;

    std::shared_ptr<xnode_element_t> node_element(common::xnode_address_t const & address, std::error_code & ec) const override;

    common::xnode_id_t node_id_from(common::xip2_t const & xip2, std::error_code & ec) const override;

    std::shared_ptr<xgroup_element_t> group_element(common::xsharding_address_t const & sharding_address, common::xversion_t const & version, std::error_code & ec) const override;

    std::shared_ptr<xgroup_element_t> group_element_by_height(common::xgroup_address_t const & group_address,
                                                              uint64_t const election_blk_height,
                                                              std::error_code & ec) const override;

    std::shared_ptr<xgroup_element_t> group_element_by_logic_time(common::xsharding_address_t const & sharding_address,
                                                                  common::xlogic_time_t const logic_time,
                                                                  std::error_code & ec) const override;

    std::shared_ptr<xgroup_element_t> parent_group_element(common::xsharding_address_t const & child_sharding_address,
                                                           common::xversion_t const & child_sharding_version,
                                                           std::error_code & ec) const override;

    common::xversion_t version_from(common::xip2_t const & xip2, std::error_code & ec) const override;

private:
    std::unordered_map<common::xcluster_address_t, xgroup_update_result_t> update_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                       data::election::xelection_result_store_t const & election_result_store,
                                                                                       std::uint64_t const associated_blk_height,
                                                                                       std::error_code & ec);

    std::unordered_map<common::xsharding_address_t, xgroup_update_result_t> update_committee_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                  data::election::xelection_result_store_t const & election_result_store,
                                                                                                  std::uint64_t const associated_blk_height,
                                                                                                  std::error_code & ec);

    std::unordered_map<common::xsharding_address_t, xgroup_update_result_t> update_consensus_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                  data::election::xelection_result_store_t const & election_result_store,
                                                                                                  std::uint64_t const associated_blk_height,
                                                                                                  std::error_code & ec);

    std::unordered_map<common::xsharding_address_t, xgroup_update_result_t> update_edge_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                             data::election::xelection_result_store_t const & election_result_store,
                                                                                             std::uint64_t const associated_blk_height,
                                                                                             std::error_code & ec);

    std::unordered_map<common::xsharding_address_t, xgroup_update_result_t> update_storage_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                data::election::xelection_result_store_t const & election_result_store,
                                                                                                std::uint64_t const associated_blk_height,
                                                                                                std::error_code & ec);

    std::unordered_map<common::xsharding_address_t, xgroup_update_result_t> update_zec_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                            data::election::xelection_result_store_t const & election_result_store,
                                                                                            std::uint64_t const associated_blk_height,
                                                                                            std::error_code & ec);

    std::unordered_map<common::xsharding_address_t, xgroup_update_result_t> update_frozen_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                               data::election::xelection_result_store_t const & election_result_store,
                                                                                               std::uint64_t const associated_blk_height,
                                                                                               std::error_code & ec);

    std::unordered_map<common::xcluster_address_t, xgroup_update_result_t> update_cluster(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                          std::shared_ptr<xcluster_element_t> const & cluster_element,
                                                                                          data::election::xelection_cluster_result_t const & cluster_result,
                                                                                          std::uint64_t const associated_blk_height,
                                                                                          std::error_code & ec);

    void update_group(std::shared_ptr<xzone_element_t> const & zone_element,
                      std::shared_ptr<xcluster_element_t> const & cluster_element,
                      std::shared_ptr<xgroup_element_t> const & group_element,
                      data::election::xelection_group_result_t const & group_result,
                      std::error_code & ec);

    std::shared_ptr<xgroup_element_t> group_element(common::xnetwork_id_t const & network_id,
                                                    common::xzone_id_t const & zone_id,
                                                    common::xcluster_id_t const & cluster_id,
                                                    common::xgroup_id_t const & group_id,
                                                    common::xversion_t const & version,
                                                    std::error_code & ec) const;

    std::shared_ptr<xgroup_element_t> group_element_by_height(common::xnetwork_id_t const & network_id,
                                                              common::xzone_id_t const & zone_id,
                                                              common::xcluster_id_t const & cluster_id,
                                                              common::xgroup_id_t const & group_id,
                                                              uint64_t const election_block_height,
                                                              std::error_code & ec) const;

    std::shared_ptr<xgroup_element_t> group_element_by_logic_time(common::xnetwork_id_t const & network_id,
                                                                  common::xzone_id_t const & zone_id,
                                                                  common::xcluster_id_t const & cluster_id,
                                                                  common::xgroup_id_t const & group_id,
                                                                  common::xlogic_time_t const logic_time,
                                                                  std::error_code & ec) const;

    std::shared_ptr<xcluster_element_t> cluster_element(common::xnetwork_id_t const & network_id,
                                                        common::xzone_id_t const & zone_id,
                                                        common::xcluster_id_t const & cluster_id,
                                                        std::error_code & ec) const;
};
using xdata_accessor_t = xtop_data_accessor;

NS_END3
