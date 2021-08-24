// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbasic/xutility.h"
#include "xcommon/xsharding_info.h"
#include "xdata/xbook_id_mapping.h"

#include <cinttypes>

NS_BEG2(top, data)

std::pair<common::xzone_id_t, std::pair<std::uint16_t, std::uint16_t>>
mapping_to_zone(std::uint16_t const book_id,
                std::size_t const zone_count,
                std::pair<std::uint16_t, std::uint16_t> const & book_range) {
    if ((book_range.first > book_id) || book_range.second < book_id) {
        return { common::xzone_id_t{}, std::make_pair(static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(0)) };
    }

    auto const book_count_per_zone = (book_range.second - book_range.first) / zone_count;
    auto const zone_id_offset = (book_id - book_range.first) / book_count_per_zone;
    auto const book_id_begin_in_zone = static_cast<std::uint16_t>(zone_id_offset * book_count_per_zone) + book_range.first;
    auto book_id_end_in_zone = static_cast<std::uint16_t>(book_id_begin_in_zone + book_count_per_zone);
    if (book_id_end_in_zone > book_range.second) {
        book_id_end_in_zone = book_range.second;
    }

    return {
        common::xzone_id_t{ static_cast<common::xzone_id_t::value_type>(zone_id_offset) },
        std::make_pair(book_id_begin_in_zone, book_id_end_in_zone)
    };
}

std::pair<common::xcluster_id_t, std::pair<std::uint16_t, std::uint16_t>>
mapping_to_cluster(std::uint16_t const book_id,
                   std::size_t const cluster_count,
                   std::pair<std::uint16_t, std::uint16_t> const & book_range) {
    if ((book_range.first > book_id) || book_range.second < book_id) {
        return { common::xcluster_id_t{}, std::make_pair(static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(0)) };
    }

    auto const book_count_per_cluster = (book_range.second - book_range.first) / cluster_count;
    auto const cluster_id_offset = (book_id - book_range.first) / book_count_per_cluster;
    auto const book_id_begin_in_cluster = static_cast<std::uint16_t>(cluster_id_offset * book_count_per_cluster) + book_range.first;
    auto book_id_end_in_cluster = static_cast<std::uint16_t>(book_id_begin_in_cluster + book_count_per_cluster);
    if (book_id_end_in_cluster > book_range.second) {
        book_id_end_in_cluster = book_range.second;
    }

    return {
        common::xcluster_id_t{ static_cast<common::xcluster_id_t::value_type>(cluster_id_offset + 1) },
        std::make_pair(book_id_begin_in_cluster, book_id_end_in_cluster)
    };
}

std::pair<common::xgroup_id_t, std::pair<std::uint16_t, std::uint16_t>>
mapping_to_auditor_group(std::uint16_t const book_id,
                         std::size_t const auditor_group_count,
                         std::pair<std::uint16_t, std::uint16_t> const & book_range) {
    if ((book_range.first > book_id) || book_range.second < book_id) {
        return { common::xgroup_id_t{}, std::make_pair(static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(0)) };
    }

    auto const book_count_per_advance_group = (book_range.second - book_range.first) / auditor_group_count;
    auto const advance_group_id_offset = (book_id - book_range.first) / book_count_per_advance_group;
    auto const book_id_begin_in_advance_group = static_cast<std::uint16_t>(advance_group_id_offset * book_count_per_advance_group) + book_range.first;
    auto book_id_end_in_advance_group = static_cast<std::uint16_t>(book_id_begin_in_advance_group + book_count_per_advance_group);
    if (book_id_end_in_advance_group > book_range.second) {
        book_id_end_in_advance_group = book_range.second;
    }

    return {
        common::xgroup_id_t{ static_cast<common::xgroup_id_t::value_type>(advance_group_id_offset + common::xauditor_group_id_value_begin) },
        std::make_pair(book_id_begin_in_advance_group, book_id_end_in_advance_group)
    };
}

std::pair<common::xgroup_id_t, std::pair<std::uint16_t, std::uint16_t>>
mapping_to_validator_group(std::uint16_t const book_id,
                           std::size_t const auditor_group_count,
                           std::size_t const validator_group_count,
                           std::pair<std::uint16_t, std::uint16_t> const & book_range) {
    if ((book_range.first > book_id) || book_range.second < book_id) {
        return { common::xgroup_id_t{}, std::make_pair(static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(0)) };
    }

    auto const validator_group_count_per_auditor_group = validator_group_count / auditor_group_count;

    auto const advance_group_info = mapping_to_auditor_group(book_id, auditor_group_count, book_range);
    auto const & advance_group_id = top::get<common::xgroup_id_t>(advance_group_info);
    auto const & new_book_range   = top::get<std::pair<std::uint16_t, std::uint16_t>>(advance_group_info);
    assert(new_book_range.first <= book_id);
    assert(book_id < new_book_range.second);

    auto const book_count_per_consensus_group = (new_book_range.second - new_book_range.first) / validator_group_count_per_auditor_group;
    auto const consensus_group_id_offset = (book_id - new_book_range.first) / book_count_per_consensus_group;
    auto const book_id_begin_in_consensus_group = static_cast<std::uint16_t>(consensus_group_id_offset * book_count_per_consensus_group) + new_book_range.first;
    auto book_id_end_in_consensus_group = static_cast<std::uint16_t>(book_id_begin_in_consensus_group + book_count_per_consensus_group);
    if (book_id_end_in_consensus_group > book_range.second) {
        book_id_end_in_consensus_group = book_range.second;
    }

    auto const advance_group_id_offset = advance_group_id.value() - common::xauditor_group_id_value_begin;
    return {
        common::xgroup_id_t{ static_cast<common::xgroup_id_t::value_type>(consensus_group_id_offset + advance_group_id_offset * validator_group_count_per_auditor_group + common::xvalidator_group_id_value_begin) },
        std::make_pair(book_id_begin_in_consensus_group, book_id_end_in_consensus_group)
    };
}

std::pair<std::uint16_t, std::uint16_t>
book_ids_belonging_to_zone(common::xzone_id_t const & zid,
                           std::size_t const zone_count,
                           std::pair<std::uint16_t, std::uint16_t> const & book_range) {
    if (zid == common::xedge_zone_id) {
        xwarn("[beacon] book id mapping for zone %s is empty", zid.to_string().c_str());
        return { 0, 0 };
    }

    if (zid == common::xcommittee_zone_id || zid == common::xzec_zone_id) {
        xdbg("[beacon] book id for beacon is [%" PRIu16 ", %" PRIu16 ")", book_range.first, book_range.second);
        return book_range;
    }

    auto const book_count_per_zone = (book_range.second - book_range.first) / zone_count;
    auto const zone_id_offset = zid.value() - common::xdefault_zone_id.value();

    auto const book_id_begin_in_zone = static_cast<std::uint16_t>(zone_id_offset * book_count_per_zone) + book_range.first;
    auto book_id_end_in_zone = static_cast<std::uint16_t>(book_id_begin_in_zone + book_count_per_zone);
    if (book_id_end_in_zone > book_range.second) {
        book_id_end_in_zone = book_range.second;
    }

    xdbg("[beacon] book id for zone %" PRIu16 " is [%" PRIu16 ", %" PRIu16 ")",
         static_cast<std::uint16_t>(zid.value()),
         book_id_begin_in_zone,
         book_id_end_in_zone);
    return { book_id_begin_in_zone, book_id_end_in_zone };
}

std::pair<std::uint16_t, std::uint16_t>
book_ids_belonging_to_cluster(common::xcluster_id_t const & cid,
                              std::size_t const cluster_count,
                              std::pair<std::uint16_t, std::uint16_t> const & book_range) {
    if (broadcast(cid)) {
        xwarn("[zec contract] book id belongs to cluster (null) is empty");
        return { 0, 0 };
    }

    // if (cid == common::xcommittee_cluster_id) {
    //     xdbg("[zec contract] book id belongs to cluster zec or rec is [%" PRIu16 ", %" PRIu16 ")", book_range.first, book_range.second);
    //     return book_range;
    // }

    auto const book_count_per_cluster = (book_range.second - book_range.first) / cluster_count;
    auto const cluster_id_offset = cid.value() - common::xdefault_cluster_id.value();
    auto const book_id_begin_in_cluster = static_cast<std::uint16_t>(cluster_id_offset * book_count_per_cluster) + book_range.first;
    auto book_id_end_in_cluster = static_cast<std::uint16_t>(book_id_begin_in_cluster + book_count_per_cluster);
    if (book_id_end_in_cluster > book_range.second) {
        book_id_end_in_cluster = book_range.second;
    }

    xdbg("[zec contract] book id belongs to cluster %" PRIu16 " is [%" PRIu16 ", %" PRIu16 ")",
         static_cast<std::uint16_t>(cid.value()),
         book_id_begin_in_cluster,
         book_id_end_in_cluster);
    return { book_id_begin_in_cluster, book_id_end_in_cluster };
}

std::pair<std::uint16_t, std::uint16_t>
book_ids_belonging_to_auditor_group(common::xgroup_id_t const & auditor_gid,
                                    std::size_t const auditor_group_count,
                                    std::pair<std::uint16_t, std::uint16_t> const & book_range) {
    if (broadcast(auditor_gid)                                      ||
        auditor_gid.value() < common::xauditor_group_id_value_begin ||
        auditor_gid.value() >= common::xauditor_group_id_value_end) {
        xwarn("[zec contract] book id belongs to auditor group %s is empty", auditor_gid.to_string().c_str());
        return { 0, 0 };
    }

    auto const book_count_per_advance_group = (book_range.second - book_range.first) / auditor_group_count;
    auto const advance_group_id_offset = auditor_gid.value() - common::xauditor_group_id_value_begin;
    auto const book_id_begin_in_advance_group = static_cast<std::uint16_t>(advance_group_id_offset * book_count_per_advance_group) + book_range.first;
    auto book_id_end_in_advance_group = static_cast<std::uint16_t>(book_id_begin_in_advance_group + book_count_per_advance_group);
    if (book_id_end_in_advance_group > book_range.second) {
        book_id_end_in_advance_group = book_range.second;
    }

    xdbg("[zec contract] book id belongs to auditor group %" PRIu16 " is [%" PRIu16 ", %" PRIu16 ")",
         static_cast<std::uint16_t>(auditor_gid.value()),
         book_id_begin_in_advance_group,
         book_id_end_in_advance_group);
    return { book_id_begin_in_advance_group, book_id_end_in_advance_group };
}

std::pair<std::uint16_t, std::uint16_t>
book_ids_belonging_to_validator_group(common::xgroup_id_t const & parent_advance_group_id,
                                      common::xgroup_id_t const & consensus_group_id,
                                      std::size_t const auditor_group_count,
                                      std::size_t const validator_group_count,
                                      std::pair<std::uint16_t, std::uint16_t> const & book_range) {
    if (broadcast(parent_advance_group_id)                                ||
        broadcast(consensus_group_id)                                     ||
        consensus_group_id.value() < common::xvalidator_group_id_value_begin ||
        consensus_group_id.value() >= common::xvalidator_group_id_value_end) {
        xwarn("[zec contract] book id belongs to auditor group %s validator group %s is empty",
              parent_advance_group_id.to_string().c_str(),
              consensus_group_id.to_string().c_str());
        return { 0, 0 };
    }

    auto const advance_group_range = book_ids_belonging_to_auditor_group(parent_advance_group_id, auditor_group_count, book_range);
    if (advance_group_range.second == 0) {
        xwarn("[zec constract] book id belongs to auditor group %" PRIu16 " is empty",
              static_cast<std::uint16_t>(parent_advance_group_id.value()));
        return { 0, 0 };
    }

    auto const validator_group_count_per_auditor_group = validator_group_count / auditor_group_count;
    auto const book_count_per_consensus_group = (advance_group_range.second - advance_group_range.first) / validator_group_count_per_auditor_group;
    auto const consensus_group_id_offset = (consensus_group_id.value() - common::xvalidator_group_id_value_begin) % validator_group_count_per_auditor_group;
    auto const book_id_begin_in_consensus_group = static_cast<std::uint16_t>(consensus_group_id_offset * book_count_per_consensus_group) + advance_group_range.first;
    auto book_id_end_in_consensus_group = static_cast<std::uint16_t>(book_id_begin_in_consensus_group + book_count_per_consensus_group);
    if (book_id_end_in_consensus_group > book_range.second) {
        book_id_end_in_consensus_group = book_range.second;
    }

    xdbg("[zec contract] book id belongs to auditor group %" PRIu16 " validator group %" PRIu16 " is [%" PRIu16 ", %" PRIu16 ")",
         static_cast<std::uint16_t>(parent_advance_group_id.value()),
         static_cast<std::uint16_t>(consensus_group_id.value()),
         book_id_begin_in_consensus_group,
         book_id_end_in_consensus_group);
    return { book_id_begin_in_consensus_group, book_id_end_in_consensus_group };
}

NS_END2
