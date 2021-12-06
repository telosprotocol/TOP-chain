// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xcommon/xsharding_info.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xbook_id_mapping.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_addr_type.h"
#include "xrouter/xroute_helper.h"
#include "xrouter/xrouter.h"
#include "xvm/manager/xcontract_address_map.h"

#include <cinttypes>
NS_BEG2(top, router)

common::xsharding_address_t xtop_router::sharding_address_from_account(common::xaccount_address_t const & target_account,
                                                                       common::xnetwork_id_t const & nid,
                                                                       common::xnode_type_t type) const {
    auto xid = base::xvaccount_t::get_xid_from_account(target_account.value());
    base::xtable_index_t tableindex = base::xtable_index_t(xid);
    xdbg("xtop_router::cluster_address_from_account addr:%s zone_index:%d subaddr:%d", target_account.c_str(), tableindex.get_zone_index(), tableindex.get_subaddr());
    return sharding_address_from_tableindex(tableindex, nid, type);
}

common::xsharding_address_t xtop_router::sharding_address_from_tableindex(base::xtable_index_t const & target_tableindex,
                                                                        common::xnetwork_id_t const & nid,
                                                                        common::xnode_type_t type) const {
    assert(common::has<common::xnode_type_t::consensus_validator>(type) || common::has<common::xnode_type_t::consensus_auditor>(type));

    switch (target_tableindex.get_zone_index()) {
    case base::enum_chain_zone_beacon_index:
        return {nid, common::xcommittee_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id};
    case base::enum_chain_zone_zec_index:
        return {nid, common::xzec_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id};
    case base::enum_chain_zone_consensus_index:
        return address_of_table_id(target_tableindex.get_subaddr(), type, nid);
    default:
        assert(0);
        return {nid, common::xzec_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id};
    }
}

common::xsharding_address_t xtop_router::address_of_table_id(std::uint16_t const table_id,
                                                             common::xnode_type_t type,
                                                             common::xnetwork_id_t const & nid) const {
    if (common::has<common::xnode_type_t::edge>(type)) {
        return {};
    }

    if (common::has<common::xnode_type_t::storage_archive>(type)) {
        return common::build_archive_sharding_address(common::xarchive_group_id, nid);
    }

    if (common::has<common::xnode_type_t::storage_exchange>(type)) {
        return common::build_archive_sharding_address(common::xexchange_group_id, nid);
    }

    if (common::has<common::xnode_type_t::committee>(type)) {
        return common::build_committee_sharding_address(nid);
    }

    if (common::has<common::xnode_type_t::zec>(type)) {
        return common::build_zec_sharding_address(nid);
    }

    return do_address_of_table_id(table_id, type, nid);
}

common::xsharding_address_t xtop_router::address_of_book_id(std::uint16_t const book_id,
                                                            common::xnode_type_t type,
                                                            common::xnetwork_id_t const & nid) const {
    if (common::has<common::xnode_type_t::edge>(type)) {
        return {};
    }

    if (common::has<common::xnode_type_t::storage_archive>(type)) {
        return common::build_archive_sharding_address(common::xarchive_group_id, nid);
    }

    if (common::has<common::xnode_type_t::storage_exchange>(type)) {
        return common::build_archive_sharding_address(common::xexchange_group_id, nid);
    }

    if (common::has<common::xnode_type_t::committee>(type)) {
        return common::build_committee_sharding_address(nid);
    }

    if (common::has<common::xnode_type_t::zec>(type)) {
        return common::build_zec_sharding_address(nid);
    }

    return do_address_of_book_id(book_id, type, nid);
}

common::xsharding_address_t xtop_router::do_address_of_book_id(std::uint16_t const book_id,
                                                               common::xnode_type_t type,
                                                               common::xnetwork_id_t const & nid) const {
    assert(common::has<common::xnode_type_t::consensus_validator>(type) || common::has<common::xnode_type_t::consensus_auditor>(type));

    auto const & config_register = top::config::xconfig_register_t::get_instance();

    auto const zone_count = XGET_CONFIG(zone_count);
    auto const cluster_count = XGET_CONFIG(cluster_count);

    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

    auto const validator_group_count = XGET_CONFIG(validator_group_count);

    auto const zone_info = data::mapping_to_zone(book_id, zone_count, std::make_pair(static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(enum_vbucket_has_books_count)));
    auto const cluster_info = data::mapping_to_cluster(book_id, cluster_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(zone_info));
    auto const auditor_group_info = data::mapping_to_auditor_group(book_id, auditor_group_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(cluster_info));
    auto const validator_group_info =
        data::mapping_to_validator_group(book_id, auditor_group_count, validator_group_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(cluster_info));

    return {nid,
            top::get<common::xzone_id_t>(zone_info),
            top::get<common::xcluster_id_t>(cluster_info),
            common::has<common::xnode_type_t::consensus_validator>(type) ? top::get<common::xgroup_id_t>(validator_group_info) : top::get<common::xgroup_id_t>(auditor_group_info)};
}

common::xsharding_address_t xtop_router::do_address_of_table_id(std::uint16_t const table_id,
                                                                common::xnode_type_t type,
                                                                common::xnetwork_id_t const & nid) const {
    assert(common::has<common::xnode_type_t::consensus_validator>(type) || common::has<common::xnode_type_t::consensus_auditor>(type));

    auto const & config_register = top::config::xconfig_register_t::get_instance();

    auto book_id = base::xvaccount_t::get_book_index_from_subaddr(table_id);

    auto const zone_count = XGET_CONFIG(zone_count);
    auto const cluster_count = XGET_CONFIG(cluster_count);

    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

    auto const validator_group_count = XGET_CONFIG(validator_group_count);

    auto const zone_info = data::mapping_to_zone(book_id, zone_count, std::make_pair(static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(enum_vbucket_has_books_count)));
    auto const cluster_info = data::mapping_to_cluster(book_id, cluster_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(zone_info));
    auto const auditor_group_info = data::mapping_to_auditor_group(book_id, auditor_group_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(cluster_info));
    auto const validator_group_info =
        data::mapping_to_validator_group(book_id, auditor_group_count, validator_group_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(cluster_info));

    return {nid,
            top::get<common::xzone_id_t>(zone_info),
            top::get<common::xcluster_id_t>(cluster_info),
            common::has<common::xnode_type_t::consensus_validator>(type) ? top::get<common::xgroup_id_t>(validator_group_info) : top::get<common::xgroup_id_t>(auditor_group_info)};
}
NS_END2
