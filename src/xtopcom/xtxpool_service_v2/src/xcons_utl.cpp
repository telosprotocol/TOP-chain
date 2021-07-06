// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service_v2/xcons_utl.h"
NS_BEG2(top, xtxpool_service_v2)
static xvip2_t broadcast_ip{(uint64_t)-1, (uint64_t)-1};

xvip2_t xcons_utl::to_xip2(const common::xnode_address_t & address, bool bwith_version) {
    xvip2_t xip = address.xip2();
    if (!bwith_version) {
        xip = address.xip2().sharding();
    }
    return xip;
}

xvip2_t xcons_utl::erase_version(const xvip2_t & xip) {
    xvip2_t xip_copy{xip.low_addr, (uint64_t)-1};
    assert(get_network_ver_from_xip2(xip_copy) == 0);
    assert(get_network_ver_from_xip2(xip) == 0);
    // We don't use network version field for now.
    // but in case some peer node modifies this field, any correct node should force obey this rule.
    reset_network_ver_to_xip2(xip_copy);
    return xip_copy;
}

bool xcons_utl::xip_equals(const xvip2_t & left, const xvip2_t & right) {
    return is_xip2_equal(left, right);
}

bool xcons_utl::is_broadcast_address(const xvip2_t & addr) {
    return xip_equals(broadcast_ip, addr);
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
            common::xaccount_election_address_t{common::xnode_id_t{"node_test"}, slot_id},
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

NS_END2
