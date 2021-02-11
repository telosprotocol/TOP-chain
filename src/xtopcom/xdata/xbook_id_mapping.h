// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xip.h"

#include <cstdint>
#include <utility>

NS_BEG2(top, data)

std::pair<common::xzone_id_t, std::pair<std::uint16_t, std::uint16_t>>
mapping_to_zone(std::uint16_t const book_id,
                std::size_t const zone_count,
                std::pair<std::uint16_t, std::uint16_t> const & book_range);

std::pair<common::xcluster_id_t, std::pair<std::uint16_t, std::uint16_t>>
mapping_to_cluster(std::uint16_t const book_id,
                   std::size_t const cluster_count,
                   std::pair<std::uint16_t, std::uint16_t> const & book_range);

std::pair<common::xgroup_id_t, std::pair<std::uint16_t, std::uint16_t>>
mapping_to_auditor_group(std::uint16_t const book_id,
                         std::size_t const auditor_group_count,
                         std::pair<std::uint16_t, std::uint16_t> const & book_range);

std::pair<common::xgroup_id_t, std::pair<std::uint16_t, std::uint16_t>>
mapping_to_validator_group(std::uint16_t const book_id,
                           std::size_t const auditor_group_count,
                           std::size_t const validator_group_count,
                           std::pair<std::uint16_t, std::uint16_t> const & book_range);

std::pair<std::uint16_t, std::uint16_t>
book_ids_belonging_to_zone(common::xzone_id_t const & zid,
                           std::size_t const zone_count,
                           std::pair<std::uint16_t, std::uint16_t> const & book_range);

std::pair<std::uint16_t, std::uint16_t>
book_ids_belonging_to_cluster(common::xcluster_id_t const & cid,
                              std::size_t const cluster_count,
                              std::pair<std::uint16_t, std::uint16_t> const & book_range);

std::pair<std::uint16_t, std::uint16_t>
book_ids_belonging_to_auditor_group(common::xgroup_id_t const & auditor_gid,
                                    std::size_t const auditor_group_count,
                                    std::pair<std::uint16_t, std::uint16_t> const & book_range);

std::pair<std::uint16_t, std::uint16_t>
book_ids_belonging_to_validator_group(common::xgroup_id_t const & parent_advance_group_id,
                                      common::xgroup_id_t const & consensus_group_id,
                                      std::size_t const auditor_group_count,
                                      std::size_t const validator_group_count,
                                      std::pair<std::uint16_t, std::uint16_t> const & book_range);

NS_END2
