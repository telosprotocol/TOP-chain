// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtable_id_mapping.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xbook_id_mapping.h"

NS_BEG2(top, data)

std::vector<uint16_t> get_table_ids(common::xzone_id_t const & zone_id,
                                    common::xcluster_id_t const & cluster_id,
                                    common::xgroup_id_t const & group_id,
                                    common::xgroup_id_t const & associated_parent_group_id) {

    auto const type = common::node_type_from(zone_id, cluster_id, group_id);

    std::vector<std::uint16_t> book_ids;
    book_ids.reserve(enum_vbucket_has_books_count);

    switch (type) {
    case common::xnode_type_t::fullnode:
        XATTRIBUTE_FALLTHROUGH;
    case common::xnode_type_t::storage_archive:
        XATTRIBUTE_FALLTHROUGH;
    case common::xnode_type_t::storage_exchange:
        XATTRIBUTE_FALLTHROUGH;
    case common::xnode_type_t::committee: {
        auto const zone_range = data::book_ids_belonging_to_zone(common::xcommittee_zone_id, 1, {0, static_cast<std::uint16_t>(enum_vbucket_has_books_count)});
        for (auto i = zone_range.first; i < zone_range.second; ++i) {
            book_ids.push_back(i);
        }

        break;
    }

    case common::xnode_type_t::zec: {
        uint32_t zone_count = XGET_CONFIG(zone_count);
        auto const zone_range = data::book_ids_belonging_to_zone(zone_id, zone_count, {0, static_cast<std::uint16_t>(enum_vbucket_has_books_count)});
        for (auto i = zone_range.first; i < zone_range.second; ++i) {
            book_ids.push_back(i);
        }

        break;
    }

    case common::xnode_type_t::consensus_auditor: {
        uint32_t zone_count = XGET_CONFIG(zone_count);
        uint32_t cluster_count = XGET_CONFIG(cluster_count);

        auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

        auto const zone_range = data::book_ids_belonging_to_zone(zone_id, zone_count, {0, static_cast<std::uint16_t>(enum_vbucket_has_books_count)});
        auto const cluster_range = data::book_ids_belonging_to_cluster(cluster_id, cluster_count, zone_range);
        auto const advance_range = data::book_ids_belonging_to_auditor_group(group_id, auditor_group_count, cluster_range);
        for (auto i = advance_range.first; i < advance_range.second; ++i) {
            book_ids.push_back(i);
        }

        break;
    }

    case common::xnode_type_t::consensus_validator: {
        try {
            std::uint16_t zone_count = XGET_CONFIG(zone_count);
            std::uint16_t cluster_count = XGET_CONFIG(cluster_count);

            auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

            auto const validator_group_count = XGET_CONFIG(validator_group_count);

            auto const zone_range = data::book_ids_belonging_to_zone(zone_id, zone_count, {0, static_cast<std::uint16_t>(enum_vbucket_has_books_count)});
            auto const cluster_range = data::book_ids_belonging_to_cluster(cluster_id, cluster_count, zone_range);
            auto const consensus_range =
                data::book_ids_belonging_to_validator_group(associated_parent_group_id, group_id, auditor_group_count, validator_group_count, cluster_range);
            for (auto i = consensus_range.first; i < consensus_range.second; ++i) {
                book_ids.push_back(i);
            }
        } catch (top::error::xtop_error_t const & eh) {
            xwarn("[vnetwork] xtop_error_t exception caught: cateogry:%s; msg:%s; error code:%d; error msg:%s",
                  eh.code().category().name(),
                  eh.what(),
                  eh.code().value(),
                  eh.code().message().c_str());
        } catch (std::exception const & eh) {
            xwarn("[vnetwork driver] caught std::exception %s", eh.what());
        } catch (...) {
            xerror("[vnetwork driver] caught unknown exception");
        }
        break;
    }

    case common::xnode_type_t::frozen: {
        // TODO george
        // get all beacon table
        book_ids.push_back(0);
        break;
    }

    case common::xnode_type_t::edge: {
        // TODO george
        // get all zec table
        book_ids.push_back(0);
        break;
    }

    case common::xnode_type_t::evm_auditor: {
        break;
    }

    case common::xnode_type_t::evm_validator: {
        break;
    }

    default: {
        assert(false);
        break;
    }
    }

    std::vector<std::uint16_t> table_ids;
    table_ids.reserve(book_ids.size() * enum_vbook_has_tables_count);

    for (auto const book_id : book_ids) {
        for (auto i = 0; i < enum_vbook_has_tables_count; ++i) {
            table_ids.push_back(book_id * enum_vbook_has_tables_count + i);
        }
    }

    // rec and zec only use some tables
    if (common::has<common::xnode_type_t::committee>(type) || common::has<common::xnode_type_t::frozen>(type)) {
        table_ids.resize(MAIN_CHAIN_REC_TABLE_USED_NUM);
    } else if (common::has<common::xnode_type_t::zec>(type) || common::has<common::xnode_type_t::edge>(type)) {
        table_ids.resize(MAIN_CHAIN_ZEC_TABLE_USED_NUM);
    } else if (common::has<common::xnode_type_t::evm>(type)) {
        table_ids.resize(MAIN_CHAIN_EVM_TABLE_USED_NUM);
    }

    return table_ids;
}

std::vector<uint16_t> get_table_ids(common::xgroup_address_t const & group_address, common::xgroup_id_t const & associated_parent_group_id) {
    return get_table_ids(group_address.zone_id(), group_address.cluster_id(), group_address.group_id(), associated_parent_group_id);
}

NS_END2
