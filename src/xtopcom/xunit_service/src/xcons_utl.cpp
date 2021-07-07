// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xcons_utl.h"

#include "xbasic/xutility.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xbook_id_mapping.h"
#include "xconfig/xpredefined_configurations.h"

#include <cinttypes>
NS_BEG2(top, xunit_service)
static xvip2_t broadcast_ip{(uint64_t)-1, (uint64_t)-1};

xvip2_t xcons_utl::to_xip2(const common::xnode_address_t & address, bool bwith_version) {
    xvip2_t xip = bwith_version ? address.xip2() : address.xip2().sharding();
    return xip;
}

xvip2_t xcons_utl::erase_version(const xvip2_t & xip) {
    xvip2_t xip_copy{xip.low_addr, (uint64_t)-1};
    assert(get_network_ver_from_xip2(xip_copy) == 0);
    assert(get_network_ver_from_xip2(xip) == 0);
    // We don't use network version field for now.
    // but in case some peer node modifies this field, any correct node should force obey this rule.
    reset_network_ver_to_xip2(xip_copy);
    set_node_id_to_xip2(xip_copy, 0xFFF);
    return xip_copy;
}

bool xcons_utl::xip_equals(const xvip2_t & left, const xvip2_t & right) {
    return is_xip2_equal(left, right);
}

bool xcons_utl::is_broadcast_address(const xvip2_t & addr) {
    return xip_equals(broadcast_ip, addr);
}

bool xcons_utl::is_auditor(const xvip2_t & xip) {
    auto group_id = get_group_id_from_xip2(xip);
    if (group_id >= common::xauditor_group_id_begin.value() && group_id < common::xauditor_group_id_end.value()) {
        return true;
    }
    return false;
}

bool xcons_utl::is_validator(const xvip2_t & xip) {
    auto group_id = get_group_id_from_xip2(xip);
    if (group_id >= common::xvalidator_group_id_begin.value() && group_id < common::xvalidator_group_id_end.value()) {
        return true;
    }
    return false;
}

common::xnode_address_t xcons_utl::to_address(const xvip2_t & xip2, common::xversion_t const & version) {
    // auto network_version_value = static_cast<common::xnetwork_version_t::value_type>(get_network_ver_from_xip2(xip2));
    auto network_id_value = static_cast<common::xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2));
    auto zone_id_value = static_cast<common::xzone_id_t::value_type>(get_zone_id_from_xip2(xip2));
    auto cluster_id_value = static_cast<common::xcluster_id_t::value_type>(get_cluster_id_from_xip2(xip2));
    auto group_id_value = static_cast<common::xgroup_id_t::value_type>(get_group_id_from_xip2(xip2));
    auto slot_id_value = static_cast<common::xslot_id_t::value_type>(get_node_id_from_xip2(xip2));
    auto shard_size = static_cast<std::uint16_t>(get_group_nodes_count_from_xip2(xip2));
    auto associated_blk_height = static_cast<std::uint64_t>(get_network_height_from_xip2(xip2));

    // assert(common::xnetwork_version_t{network_version_value} == version);  // NOLINT

    common::xslot_id_t slot_id{slot_id_value};
    if (!broadcast(slot_id)) {
        common::xnode_address_t address{
            common::xcluster_address_t{
                common::xnetwork_id_t{network_id_value}, common::xzone_id_t{zone_id_value}, common::xcluster_id_t{cluster_id_value}, common::xgroup_id_t{group_id_value}},
            common::xaccount_election_address_t{common::xnode_id_t{"_fake_node"}, slot_id},
            version,
            shard_size,
            associated_blk_height};
        return address;
    } else {
        common::xnode_address_t address{
            common::xcluster_address_t{
                common::xnetwork_id_t{network_id_value}, common::xzone_id_t{zone_id_value}, common::xcluster_id_t{cluster_id_value}, common::xgroup_id_t{group_id_value}},
            version,
            shard_size,
            associated_blk_height};
        return address;
    }

    // auto type_value = static_cast<common::xnode_type_t>(get_xip2_type_from_xip2_ex(xip));
}

std::string xcons_utl::xip_to_hex(const xvip2_t & xip) {
    char data[35] = {0};
    snprintf(data, sizeof(data) / sizeof(data[0]), "%" PRIx64 ":%" PRIx64, xip.high_addr, xip.low_addr);
    return std::string(data);
}

int32_t xcons_utl::get_groupid_by_account(const xvip2_t & local_xip, const std::string & account) {
    auto xid1 = base::xvaccount_t::get_xid_from_account(account);
    uint16_t table = get_vledger_subaddr(xid1);
    return get_groupid_by_table(local_xip, table);
}

int32_t xcons_utl::get_groupid_by_table(const xvip2_t & local_xip, uint16_t table_id) {
    auto book_id = base::xvaccount_t::get_book_index_from_subaddr(table_id);
    auto const & config_register = top::config::xconfig_register_t::get_instance();
    
    auto const zone_count = XGET_CONFIG(zone_count);
    auto const cluster_count = XGET_CONFIG(cluster_count);

    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);
    auto const validator_group_count = XGET_CONFIG(validator_group_count);

    auto const zone_info = data::mapping_to_zone(book_id, zone_count, std::make_pair(static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(enum_vbucket_has_books_count)));
    auto const cluster_info = data::mapping_to_cluster(book_id, cluster_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(zone_info));
    // auto const auditor_group_info = data::mapping_to_auditor_group(book_id, auditor_group_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(cluster_info));
    auto const validator_group_info =
        data::mapping_to_validator_group(book_id, auditor_group_count, validator_group_count, top::get<std::pair<std::uint16_t, std::uint16_t>>(cluster_info));
    return top::get<common::xgroup_id_t>(validator_group_info).value();
}
NS_END2
