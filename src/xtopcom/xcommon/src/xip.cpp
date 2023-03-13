// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xip.h"

#include "xbase/xlog.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"

#include <cassert>
#include <iomanip>
#include <ios>
#include <sstream>
#include <utility>

NS_BEG2(top, common)

static void reset_raw_xip2_address_domain(xvip2_t & xip2) noexcept {
    reset_address_domain_to_xip2(xip2);
#if !defined NDEBUG
    auto const domain = static_cast<xaddress_domain_t>(get_address_domain_from_xip2(xip2));
    assert(domain == xaddress_domain_t::enum_xaddress_domain_xip);
#endif
}

//static void reset_raw_xip2_type(xvip2_t & xip2) noexcept {
//    reset_xip_type_to_xip2(xip2);
//#if !defined NDEBUG
//    auto const type = static_cast<xip_type_t>(get_xip_type_from_xip2(xip2));
//    assert(type == xip_type_t::enum_xip_type_dynamic);
//#endif
//}

static void reset_raw_xip2_network_type(xvip2_t & xip2) noexcept {
    reset_network_type_to_xip2(xip2);
#if !defined NDEBUG
    auto const network_type = static_cast<xnetwork_type_t>(get_network_type_from_xip2(xip2));
    assert(network_type == xnetwork_type_t::enum_xnetwork_type_any);
#endif
}

//static void reset_raw_xip2_network_version(xvip2_t & xip2) noexcept {
//    reset_network_ver_to_xip2(xip2);
//#if !defined NDEBUG
//    auto const version = static_cast<xnetwork_version_t::value_type>(get_network_ver_from_xip2(xip2));
//    assert(version == 0);
//#endif
//}

static void reset_raw_xip2_network_id(xvip2_t & xip2) noexcept {
    reset_network_id_to_xip2(xip2);
#if !defined NDEBUG
    auto const network_id = static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2));
    assert(network_id == 0);
#endif
}

static void reset_raw_xip2_zone_id(xvip2_t & xip2) noexcept {
    reset_zone_id_to_xip2(xip2);
#if !defined NDEBUG
    auto const zone_id = static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(xip2));
    assert(zone_id == 0);
#endif
}

static void reset_raw_xip2_cluster_id(xvip2_t & xip2) noexcept {
    reset_cluster_id_to_xip2(xip2);
#if !defined NDEBUG
    auto const cluster_id = static_cast<xcluster_id_t::value_type>(get_cluster_id_from_xip2(xip2));
    assert(cluster_id == 0);
#endif
}

static void reset_raw_xip2_group_id(xvip2_t & xip2) noexcept {
    reset_group_id_to_xip2(xip2);
#if !defined NDEBUG
    auto const group_id = static_cast<xgroup_id_t::value_type>(get_group_id_from_xip2(xip2));
    assert(group_id == 0);
#endif
}

static void reset_raw_xip2_slot_id(xvip2_t & xip2) noexcept {
    reset_node_id_to_xip2(xip2);
#if !defined NDEBUG
    auto const slot_id = static_cast<xslot_id_t::value_type>(get_node_id_from_xip2(xip2));
    assert(slot_id == 0);
#endif
}

static void reset_raw_xip2_group_size(xvip2_t & xip2) noexcept {
    reset_group_nodes_count_to_xip2(xip2);
#if !defined NDEBUG
    auto const count = static_cast<uint16_t>(get_group_nodes_count_from_xip2(xip2));
    assert(count == 0);
#endif
}

static void reset_raw_xip2_blk_height(xvip2_t & xip2) noexcept {
    reset_network_height_to_xip2(xip2);
#if !defined NDEBUG
    auto const height = static_cast<uint64_t>(get_network_height_from_xip2(xip2));
    assert(height == 0);
#endif
}

static void set_raw_xip2_address_domain(xvip2_t & xip2, xaddress_domain_t const domain) noexcept {
    reset_raw_xip2_address_domain(xip2);
    set_address_domain_to_xip2(xip2, domain);
#if !defined NDEBUG
    auto const value = static_cast<xaddress_domain_t>(get_address_domain_from_xip2(xip2));
    assert(value == domain);
#endif
}

//static void set_raw_xip_address_domain(xvip_t & xip, xaddress_domain_t const domain) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_address_domain(xip2, domain);
//    xip = xip2.low_addr;
//
//#if !defined NDEBUG
//    xip2.low_addr = xip;
//    auto const v = static_cast<xaddress_domain_t>(get_address_domain_from_xip2(xip2));
//    assert(domain == v);
//#endif
//}

//static void set_raw_xip2_type(xvip2_t & xip2, xip_type_t const type) noexcept {
//    reset_raw_xip2_type(xip2);
//    set_xip_type_to_xip2(xip2, type);
//}

//static void set_raw_xip_type(xvip_t & xip, xip_type_t const type) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_type(xip2, type);
//    xip = xip2.low_addr;
//}

static void set_raw_xip2_network_type(xvip2_t & xip2, xnetwork_type_t const type) noexcept {
    reset_raw_xip2_network_type(xip2);
    set_network_type_to_xip2(xip2, type);
}

//static void set_raw_xip_network_type(xvip_t & xip, xnetwork_type_t const type) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_network_type(xip2, type);
//    xip = xip2.low_addr;
//}

//static void set_raw_xip2_network_version(xvip2_t & xip2, std::uint8_t const version) noexcept {
//    reset_raw_xip2_network_version(xip2);
//    set_network_ver_to_xip2(xip2, version);
//}

//static void set_raw_xip_network_version(xvip_t & xip, std::uint8_t const version) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_network_version(xip2, version);
//    xip = xip2.low_addr;
//}

static void set_raw_xip2_network_id(xvip2_t & xip2, xnetwork_id_t::value_type const nid) noexcept {
    reset_raw_xip2_network_id(xip2);
    set_network_id_to_xip2(xip2, nid);
#if !defined NDEBUG
    auto const network_id = static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2));
    assert(nid == network_id);
#endif
}

//static void set_raw_xip_network_id(xvip_t & xip, xnetwork_id_t::value_type const network_id) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_network_id(xip2, network_id);
//    xip = xip2.low_addr;
//}

static void set_raw_xip2_zone_id(xvip2_t & xip2, xzone_id_t::value_type const zid) noexcept {
    reset_raw_xip2_zone_id(xip2);
    set_zone_id_to_xip2(xip2, zid);
#if !defined NDEBUG
    auto const zone_id = static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(xip2));
    assert(zid == zone_id);
#endif
}

//static void set_raw_xip_zone_id(xvip_t & xip, xzone_id_t::value_type const zone_id) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_zone_id(xip2, zone_id);
//    xip = xip2.low_addr;
//}

static void set_raw_xip2_cluster_id(xvip2_t & xip2, xcluster_id_t::value_type const cid) noexcept {
    reset_raw_xip2_cluster_id(xip2);
    set_cluster_id_to_xip2(xip2, cid);
#if !defined NDEBUG
    auto const cluster_id = static_cast<xcluster_id_t::value_type>(get_cluster_id_from_xip2(xip2));
    assert(cid == cluster_id);
#endif
}

//static void set_raw_xip_cluster_id(xvip_t & xip, xcluster_id_t::value_type const cluster_id) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_cluster_id(xip2, cluster_id);
//    xip = xip2.low_addr;
//}

static void set_raw_xip2_group_id(xvip2_t & xip2, xgroup_id_t::value_type const gid) noexcept {
    reset_raw_xip2_group_id(xip2);
    set_group_id_to_xip2(xip2, gid);
#if !defined NDEBUG
    auto const group_id = static_cast<xgroup_id_t::value_type>(get_group_id_from_xip2(xip2));
    assert(gid == group_id);
#endif
}

//static void set_raw_xip_group_id(xvip_t & xip, xgroup_id_t::value_type const group_id) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_group_id(xip2, group_id);
//    xip = xip2.low_addr;
//}

static void set_raw_xip2_slot_id(xvip2_t & xip2, xslot_id_t::value_type const sid) noexcept {
    reset_raw_xip2_slot_id(xip2);
    set_node_id_to_xip2(xip2, sid);
#if !defined NDEBUG
    auto const slot_id = static_cast<xslot_id_t::value_type>(get_node_id_from_xip2(xip2));
    assert(sid == slot_id);
#endif
}

//static void set_raw_xip_slot_id(xvip_t & xip, xslot_id_t::value_type const slot_id) noexcept {
//    xvip2_t xip2{xip, 0};
//    set_raw_xip2_slot_id(xip2, slot_id);
//    xip = xip2.low_addr;
//}

//static void set_raw_xip2_group_size(xvip2_t & xip2, uint16_t const size) noexcept {
//    reset_raw_xip2_group_size(xip2);
//    set_group_nodes_count_to_xip2(xip2, size);
//#if !defined NDEBUG
//    auto const sz = static_cast<std::uint16_t>(get_group_nodes_count_from_xip2(xip2));
//    assert(size == sz);
//#endif
//}

//static void set_raw_xip2_blk_height(xvip2_t & xip2, uint64_t const h) noexcept {
//    reset_raw_xip2_blk_height(xip2);
//    set_network_height_to_xip2(xip2, h);
//#if !defined NDEBUG
//    auto const height = static_cast<std::uint64_t>(get_network_height_from_xip2(xip2));
//    assert(h == height);
//#endif
//}

xtop_ip::xtop_ip(xtop_ip const & xip, xaddress_domain_t const domain) noexcept : xtop_ip{xip} {
    xvip2_t xip2{m_xip, 0};
    set_raw_xip2_address_domain(xip2, domain);
    m_xip = xip2.low_addr;
}

xtop_ip::xtop_ip(xnetwork_id_t const & network_id, xaddress_domain_t const domain) noexcept {
    // assert(network_id.has_value());

    xvip2_t xip2{m_xip, 0};
    set_raw_xip2_address_domain(xip2, domain);
    set_raw_xip2_network_type(xip2, xnetwork_type_t::enum_xnetwork_type_xchain);
    set_raw_xip2_network_id(xip2, network_id.value());
    m_xip = xip2.low_addr;

    assert(static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2)) == (network_id.value() & xbroadcast_network_id_value));
}

xtop_ip::xtop_ip(xnetwork_id_t const & network_id, xzone_id_t const & zone_id, xaddress_domain_t const domain) noexcept {
    // assert(network_id.has_value());
    // assert(zone_id.has_value());

    xvip2_t xip2{m_xip, 0};
    set_raw_xip2_address_domain(xip2, domain);
    set_raw_xip2_network_type(xip2, xnetwork_type_t::enum_xnetwork_type_xchain);
    set_raw_xip2_network_id(xip2, network_id.value());
    set_raw_xip2_zone_id(xip2, zone_id.value());
    m_xip = xip2.low_addr;

    assert(static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2)) == (network_id.value() & xbroadcast_network_id_value));
    assert(static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(xip2)) == (zone_id.value() & xbroadcast_zone_id_value));
}

xtop_ip::xtop_ip(xnetwork_id_t const & network_id, xzone_id_t const & zone_id, xcluster_id_t const & cluster_id, xaddress_domain_t const domain) noexcept {
    // assert(network_id.has_value());
    // assert(zone_id.has_value());
    // assert(cluster_id.has_value());

    xvip2_t xip2{m_xip, 0};
    set_raw_xip2_address_domain(xip2, domain);
    set_raw_xip2_network_type(xip2, xnetwork_type_t::enum_xnetwork_type_xchain);
    set_raw_xip2_network_id(xip2, network_id.value());
    set_raw_xip2_zone_id(xip2, zone_id.value());
    set_raw_xip2_cluster_id(xip2, cluster_id.value());
    m_xip = xip2.low_addr;

    assert(static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2)) == (network_id.value() & xbroadcast_network_id_value));
    assert(static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(xip2)) == (zone_id.value() & xbroadcast_zone_id_value));
    assert(static_cast<xcluster_id_t::value_type>(get_cluster_id_from_xip2(xip2)) == (cluster_id.value() & xbroadcast_cluster_id_value));
}

xtop_ip::xtop_ip(xnetwork_id_t const & network_id,
                 xzone_id_t const & zone_id,
                 xcluster_id_t const & cluster_id,
                 xgroup_id_t const & group_id,
                 xaddress_domain_t const domain) noexcept {
    // assert(network_id.has_value());
    // assert(zone_id.has_value());
    // assert(cluster_id.has_value());
    // assert(group_id.has_value());

    xvip2_t xip2{m_xip, 0};
    set_raw_xip2_address_domain(xip2, domain);
    set_raw_xip2_network_type(xip2, xnetwork_type_t::enum_xnetwork_type_xchain);
    set_raw_xip2_network_id(xip2, network_id.value());
    set_raw_xip2_zone_id(xip2, zone_id.value());
    set_raw_xip2_cluster_id(xip2, cluster_id.value());
    set_raw_xip2_group_id(xip2, group_id.value());
    m_xip = xip2.low_addr;

    assert(static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2)) == (network_id.value() & xbroadcast_network_id_value));
    assert(static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(xip2)) == (zone_id.value() & xbroadcast_zone_id_value));
    assert(static_cast<xcluster_id_t::value_type>(get_cluster_id_from_xip2(xip2)) == (cluster_id.value() & xbroadcast_cluster_id_value));
    assert(static_cast<xgroup_id_t::value_type>(get_group_id_from_xip2(xip2)) == (group_id.value() & xbroadcast_group_id_value));
}

//xtop_ip::xtop_ip(xnetwork_id_t const & network_id,
//                 xzone_id_t const & zone_id,
//                 xcluster_id_t const & cluster_id,
//                 xgroup_id_t const & group_id,
//                 xnetwork_version_t const & network_version,
//                 xaddress_domain_t const domain) noexcept {
//    assert(network_id.has_value());
//    assert(zone_id.has_value());
//    assert(cluster_id.has_value());
//    assert(group_id.has_value());
//    assert(network_version.has_value());
//
//    xvip2_t xip2{m_xip, 0};
//    set_raw_xip2_address_domain(xip2, domain);
//    set_raw_xip2_network_type(xip2, xnetwork_type_t::enum_xnetwork_type_xchain);
//    set_raw_xip2_network_id(xip2, network_id.value());
//    set_raw_xip2_zone_id(xip2, zone_id.value());
//    set_raw_xip2_cluster_id(xip2, cluster_id.value());
//    set_raw_xip2_group_id(xip2, group_id.value());
//    set_raw_xip2_network_version(xip2, network_version.value());
//    m_xip = xip2.low_addr;
//
//    assert(static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2)) == (network_id.value() & xbroadcast_network_id_value));
//    assert(static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(xip2)) == (zone_id.value() & xbroadcast_zone_id_value));
//    assert(static_cast<xcluster_id_t::value_type>(get_cluster_id_from_xip2(xip2)) == (cluster_id.value() & xbroadcast_cluster_id_value));
//    assert(static_cast<xgroup_id_t::value_type>(get_group_id_from_xip2(xip2)) == (group_id.value() & xbroadcast_group_id_value));
//    assert(static_cast<xnetwork_version_t::value_type>(get_network_ver_from_xip2(xip2)) == (network_version.value() & xdefault_network_version_value));
//}

xtop_ip::xtop_ip(xnetwork_id_t const & network_id,
                 xzone_id_t const & zone_id,
                 xcluster_id_t const & cluster_id,
                 xgroup_id_t const & group_id,
                 xslot_id_t const & slot_id,
                 xaddress_domain_t const domain) noexcept {
    // assert(network_id.has_value());
    // assert(zone_id.has_value());
    // assert(cluster_id.has_value());
    // assert(group_id.has_value());
    // assert(slot_id.has_value());

    xvip2_t xip2{m_xip, 0};
    set_raw_xip2_address_domain(xip2, domain);
    set_raw_xip2_network_type(xip2, xnetwork_type_t::enum_xnetwork_type_xchain);
    set_raw_xip2_network_id(xip2, network_id.value());
    set_raw_xip2_zone_id(xip2, zone_id.value());
    set_raw_xip2_cluster_id(xip2, cluster_id.value());
    set_raw_xip2_group_id(xip2, group_id.value());
    set_raw_xip2_slot_id(xip2, slot_id.value());
    m_xip = xip2.low_addr;

    assert(static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2)) == (network_id.value() & xbroadcast_network_id_value));
    assert(static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(xip2)) == (zone_id.value() & xbroadcast_zone_id_value));
    assert(static_cast<xcluster_id_t::value_type>(get_cluster_id_from_xip2(xip2)) == (cluster_id.value() & xbroadcast_cluster_id_value));
    assert(static_cast<xgroup_id_t::value_type>(get_group_id_from_xip2(xip2)) == (group_id.value() & xbroadcast_group_id_value));
    assert(static_cast<xslot_id_t::value_type>(get_node_id_from_xip2(xip2)) == (slot_id.value() & xbroadcast_slot_id_value));
}

//xtop_ip::xtop_ip(xnetwork_id_t const & network_id,
//                 xzone_id_t const & zone_id,
//                 xcluster_id_t const & cluster_id,
//                 xgroup_id_t const & group_id,
//                 xslot_id_t const & slot_id,
//                 xnetwork_version_t const & network_version,
//                 xaddress_domain_t const domain) noexcept {
//    assert(network_id.has_value());
//    assert(zone_id.has_value());
//    assert(cluster_id.has_value());
//    assert(group_id.has_value());
//    assert(slot_id.has_value());
//    assert(network_version.has_value());
//
//    xvip2_t xip2{m_xip, 0};
//    set_raw_xip2_address_domain(xip2, domain);
//    set_raw_xip2_network_type(xip2, xnetwork_type_t::enum_xnetwork_type_xchain);
//    set_raw_xip2_network_id(xip2, network_id.value());
//    set_raw_xip2_zone_id(xip2, zone_id.value());
//    set_raw_xip2_cluster_id(xip2, cluster_id.value());
//    set_raw_xip2_group_id(xip2, group_id.value());
//    set_raw_xip2_slot_id(xip2, slot_id.value());
//    set_raw_xip2_network_version(xip2, network_version.value());
//    m_xip = xip2.low_addr;
//
//    assert(static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(xip2)) == (network_id.value() & xbroadcast_network_id_value));
//    assert(static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(xip2)) == (zone_id.value() & xbroadcast_zone_id_value));
//    assert(static_cast<xcluster_id_t::value_type>(get_cluster_id_from_xip2(xip2)) == (cluster_id.value() & xbroadcast_cluster_id_value));
//    assert(static_cast<xgroup_id_t::value_type>(get_group_id_from_xip2(xip2)) == (group_id.value() & xbroadcast_group_id_value));
//    assert(static_cast<xslot_id_t::value_type>(get_node_id_from_xip2(xip2)) == (slot_id.value() & xbroadcast_slot_id_value));
//    assert(static_cast<xnetwork_version_t::value_type>(get_network_ver_from_xip2(xip2)) == (network_version.value() & xdefault_network_version_value));
//}

xaddress_domain_t xtop_ip::address_domain() const noexcept {
    xvip2_t xip2{m_xip, 0};
    return static_cast<xaddress_domain_t>(get_address_domain_from_xip2(xip2));
}

bool xtop_ip::empty() const noexcept {
    return m_xip == 0 || (broadcast(network_id()) && broadcast(zone_id()) && broadcast(cluster_id()) && broadcast(group_id()));
}

void xtop_ip::swap(xtop_ip & other) noexcept {
    std::swap(m_xip, other.m_xip);
}

bool xtop_ip::operator==(xtop_ip const & other) const noexcept {
    return m_xip == other.m_xip;
}

bool xtop_ip::operator<(xtop_ip const & other) const noexcept {
    return m_xip < other.m_xip;
}

bool xtop_ip::operator!=(xtop_ip const & other) const noexcept {
    return !(*this == other);
}

common::xip_type_t xtop_ip::type() const noexcept {
    xvip2_t tmp_xip{m_xip, 0};
    return static_cast<common::xip_type_t>(get_xip_type_from_xip2(tmp_xip));
}

common::xnetwork_type_t xtop_ip::network_type() const noexcept {
    xvip2_t tmp_xip{m_xip, 0};
    return static_cast<common::xnetwork_type_t>(get_network_type_from_xip2(tmp_xip));
}

xnetwork_version_t xtop_ip::network_version() const noexcept {
    // xvip2_t tmp_xip{m_xip, 0};
    // auto ret = xnetwork_version_t{static_cast<xnetwork_version_t::value_type>(get_network_ver_from_xip2(tmp_xip))};
    // assert(ret == xdefault_network_version);
    // return ret;
    return xdefault_network_version;
}

xnetwork_id_t xtop_ip::network_id() const noexcept {
    xvip2_t tmp_xip{m_xip, 0};
    return xnetwork_id_t{static_cast<xnetwork_id_t::value_type>(get_network_id_from_xip2(tmp_xip))};
}

xzone_id_t xtop_ip::zone_id() const noexcept {
    xvip2_t tmp_xip{m_xip, 0};
    return xzone_id_t{static_cast<xzone_id_t::value_type>(get_zone_id_from_xip2(tmp_xip))};
}

xcluster_id_t xtop_ip::cluster_id() const noexcept {
    xvip2_t tmp_xip{m_xip, 0};
    return xcluster_id_t{static_cast<xcluster_id_t::value_type>(get_cluster_id_from_xip2(tmp_xip))};
}

xgroup_id_t xtop_ip::group_id() const noexcept {
    xvip2_t tmp_xip{m_xip, 0};
    return xgroup_id_t{static_cast<xgroup_id_t::value_type>(get_group_id_from_xip2(tmp_xip))};
}

xslot_id_t xtop_ip::slot_id() const noexcept {
    xvip2_t tmp_xip{m_xip, 0};
    return xslot_id_t{static_cast<xslot_id_t::value_type>(get_node_id_from_xip2(tmp_xip))};
}

std::string xtop_ip::to_string() const {
    std::ostringstream oss;
    oss << std::right << std::setfill('0') << std::setw(sizeof(m_xip) * 2) << std::hex << m_xip;
    return oss.str();
}

xtop_ip::value_type xtop_ip::value() const noexcept {
    return m_xip;
}

xtop_ip xtop_ip::group_xip() const noexcept {
    return {network_id(), zone_id(), cluster_id(), group_id()};
}

std::string to_string(xnetwork_version_t const network_version) {
    return std::to_string(static_cast<std::uint16_t>(network_version.value()));
}

std::string to_string(xnetwork_id_t const & nid) {
    return std::to_string(nid.value());
}

std::string to_string(xzone_id_t const & zid) {
    return std::to_string(static_cast<std::uint16_t>(zid.value()));
}

std::string to_string(xcluster_id_t const & cid) {
    return std::to_string(static_cast<std::uint16_t>(cid.value()));
}

std::string to_string(xgroup_id_t const & gid) {
    return std::to_string(static_cast<std::uint16_t>(gid.value()));
}

std::string to_string(xslot_id_t const & sid) {
    return std::to_string(static_cast<std::uint16_t>(sid.value()));
}

xtop_ip::operator xtop_ip::value_type() const noexcept {
    return m_xip;
}

std::size_t xtop_ip::hash() const {
    return m_xip;
}

NS_END2

NS_BEG1(top)

xtop_extended<common::xip_t>::xtop_extended(xvip2_t const & xvip2) noexcept : m_xip{xvip2.low_addr}, m_extent{xvip2.high_addr} {
}

xtop_extended<common::xip_t>::xtop_extended(std::uint64_t const low, std::uint64_t const high) noexcept : m_xip{low}, m_extent{high} {
}

xtop_extended<common::xip_t>::xtop_extended(common::xip_t const & xip, common::xaddress_domain_t const domain) noexcept : m_xip{xip, domain} {
}

xtop_extended<common::xip_t>::xtop_extended(common::xip_t const & xip, std::uint64_t const high, common::xaddress_domain_t const domain) noexcept
  : m_xip{xip, domain}, m_extent{high} {
}

xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id, common::xaddress_domain_t const domain) : m_xip{network_id, domain} {
}

xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id, common::xzone_id_t const & zone_id, common::xaddress_domain_t const domain)
  : m_xip{network_id, zone_id, domain} {
}

xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id,
                                            common::xzone_id_t const & zone_id,
                                            common::xcluster_id_t const & cluster_id,
                                            common::xaddress_domain_t const domain)
  : m_xip{network_id, zone_id, cluster_id, domain} {
}

xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id,
                                            common::xzone_id_t const & zone_id,
                                            common::xcluster_id_t const & cluster_id,
                                            common::xgroup_id_t const & group_id,
                                            common::xaddress_domain_t const domain)
  : m_xip{network_id, zone_id, cluster_id, group_id, domain} {
}

//xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id,
//                                            common::xzone_id_t const & zone_id,
//                                            common::xcluster_id_t const & cluster_id,
//                                            common::xgroup_id_t const & group_id,
//                                            common::xnetwork_version_t const & network_version,
//                                            common::xaddress_domain_t const domain)
//  : m_xip{network_id, zone_id, cluster_id, group_id, network_version, domain} {
//}

xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id,
                                            common::xzone_id_t const & zone_id,
                                            common::xcluster_id_t const & cluster_id,
                                            common::xgroup_id_t const & group_id,
                                            uint16_t const size,
                                            uint64_t const height,
                                            common::xaddress_domain_t const domain)
  : m_xip{network_id, zone_id, cluster_id, group_id, domain} {
    xvip2_t xip2{0, 0};
    set_group_nodes_count_to_xip2(xip2, size);
    set_network_height_to_xip2(xip2, height);

    m_extent = xip2.high_addr;
}

//xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id,
//                                            common::xzone_id_t const & zone_id,
//                                            common::xcluster_id_t const & cluster_id,
//                                            common::xgroup_id_t const & group_id,
//                                            common::xnetwork_version_t const & network_version,
//                                            uint16_t const size,
//                                            uint64_t const height,
//                                            common::xaddress_domain_t const domain)
//  : m_xip{network_id, zone_id, cluster_id, group_id, domain} {
//    xvip2_t xip2{0, 0};
//    set_group_nodes_count_to_xip2(xip2, size);
//    set_network_height_to_xip2(xip2, height);
//
//    m_extent = xip2.high_addr;
//}

xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id,
                                            common::xzone_id_t const & zone_id,
                                            common::xcluster_id_t const & cluster_id,
                                            common::xgroup_id_t const & group_id,
                                            common::xslot_id_t const & slot_id,
                                            // common::xnetwork_version_t const & network_version,
                                            common::xaddress_domain_t const domain)
  : m_xip{network_id, zone_id, cluster_id, group_id, slot_id, domain} {
}

xtop_extended<common::xip_t>::xtop_extended(common::xnetwork_id_t const & network_id,
                                            common::xzone_id_t const & zone_id,
                                            common::xcluster_id_t const & cluster_id,
                                            common::xgroup_id_t const & group_id,
                                            common::xslot_id_t const & slot_id,
                                            // common::xnetwork_version_t const & network_version,
                                            uint16_t const size,
                                            uint64_t const height,
                                            common::xaddress_domain_t const domain)
  : m_xip{network_id, zone_id, cluster_id, group_id, slot_id, domain} {
    xvip2_t xip2{0, 0};
    set_group_nodes_count_to_xip2(xip2, size);
    set_network_height_to_xip2(xip2, height);

    m_extent = xip2.high_addr;
}

void xtop_extended<common::xip_t>::swap(xtop_extended<common::xip_t> & other) noexcept {
    m_xip.swap(other.m_xip);
    std::swap(m_extent, other.m_extent);
}

bool xtop_extended<common::xip_t>::operator==(xtop_extended const & other) const noexcept {
    return m_xip == other.m_xip && m_extent == other.m_extent;
}

bool xtop_extended<common::xip_t>::operator!=(xtop_extended const & other) const noexcept {
    return !(*this == other);
}

common::xaddress_domain_t xtop_extended<common::xip_t>::address_domain() const noexcept {
    assert(common::xaddress_domain_t::enum_xaddress_domain_xip2 == m_xip.address_domain());
    return m_xip.address_domain();
}

common::xip_type_t xtop_extended<common::xip_t>::type() const noexcept {
    return m_xip.type();
}

common::xnetwork_type_t xtop_extended<common::xip_t>::network_type() const noexcept {
    assert(m_xip.network_type() == common::xnetwork_type_t::enum_xnetwork_type_xchain);
    return m_xip.network_type();
}

//common::xnetwork_version_t xtop_extended<common::xip_t>::network_version() const noexcept {
//    return m_xip.network_version();
//}

common::xnetwork_id_t xtop_extended<common::xip_t>::network_id() const noexcept {
    return m_xip.network_id();
}

common::xzone_id_t xtop_extended<common::xip_t>::zone_id() const noexcept {
    return m_xip.zone_id();
}

common::xcluster_id_t xtop_extended<common::xip_t>::cluster_id() const noexcept {
    return m_xip.cluster_id();
}

common::xgroup_id_t xtop_extended<common::xip_t>::group_id() const noexcept {
    return m_xip.group_id();
}

common::xslot_id_t xtop_extended<common::xip_t>::slot_id() const noexcept {
    return m_xip.slot_id();
}

uint16_t xtop_extended<common::xip_t>::size() const noexcept {
    xvip2_t xip2{0, m_extent};
    return static_cast<uint16_t>(get_group_nodes_count_from_xip2(xip2));
}

uint64_t xtop_extended<common::xip_t>::height() const noexcept {
    xvip2_t xip2{0, m_extent};
    return static_cast<uint64_t>(get_network_height_from_xip2(xip2));
}

common::xip_t xtop_extended<common::xip_t>::xip() const noexcept {
    return m_xip;
}

std::uint64_t xtop_extended<common::xip_t>::raw_high_part() const noexcept {
    return m_extent;
}

std::uint64_t xtop_extended<common::xip_t>::raw_low_part() const noexcept {
    return static_cast<std::uint64_t>(m_xip);
}

std::size_t xtop_extended<common::xip_t>::hash() const {
    return m_xip.hash() ^ m_extent;
}

std::string xtop_extended<common::xip_t>::to_string() const {
    std::ostringstream oss;
    oss << m_xip.to_string() << ".";
    oss << std::right << std::setfill('0') << std::setw(sizeof(m_extent) * 2) << std::hex << m_extent;
    return oss.str();
}

xvip2_t xtop_extended<common::xip_t>::value() const noexcept {
    return xvip2_t{m_xip.value(), m_extent};
}

xtop_extended<common::xip_t>::operator xvip2_t() const noexcept {
    return value();
}

NS_END1

NS_BEG2(top, common)

xnode_type_t node_type_from(common::xzone_id_t const & zone_id) {
    if (zone_id == xcommittee_zone_id) {
        return xnode_type_t::committee;
    }

    if (zone_id == xconsensus_zone_id) {
        return xnode_type_t::consensus;
    }

    if (zone_id == xstorage_zone_id) {
        return xnode_type_t::storage;
    }

    if (zone_id == xedge_zone_id) {
        return xnode_type_t::edge;
    }

    if (zone_id == xfullnode_zone_id) {
        return xnode_type_t::fullnode;
    }

    if (zone_id == xzec_zone_id) {
        return xnode_type_t::zec;
    }

    if (zone_id == xfrozen_zone_id) {
        return xnode_type_t::frozen;
    }

    if (zone_id == xevm_zone_id) {
        return xnode_type_t::evm;
    }

    if (zone_id == xrelay_zone_id) {
        return xnode_type_t::relay;
    }

    return xnode_type_t::invalid;
}

xnode_type_t node_type_from(common::xzone_id_t const & zone_id, xcluster_id_t const & /* cluster_id */) {
    return node_type_from(zone_id);
}

xnode_type_t node_type_from(common::xzone_id_t const & zone_id, common::xcluster_id_t const & cluster_id, xgroup_id_t const & group_id) {
    auto node_type = node_type_from(zone_id);

    switch (node_type) {
    case xnode_type_t::consensus: {
        if (xauditor_group_id_begin <= group_id && group_id < xauditor_group_id_end) {
            node_type |= xnode_type_t::consensus_auditor;
        } else if (xvalidator_group_id_begin <= group_id && group_id < xvalidator_group_id_end) {
            node_type |= xnode_type_t::consensus_validator;
        } else {
            assert(false);
            node_type = xnode_type_t::invalid;
        }

        break;
    }

    case xnode_type_t::storage: {
        do {
            if (cluster_id == xexchange_cluster_id) {
                assert(group_id == common::xexchange_group_id);
                node_type |= xnode_type_t::storage_exchange;
                break;
            }

            if (cluster_id == xdefault_cluster_id) {
                assert(xarchive_group_id == group_id);
                node_type |= xnode_type_t::storage_archive;
                break;
            }

            assert(false);
            node_type = xnode_type_t::invalid;
        } while (false);

        break;
    }

    case xnode_type_t::evm: {
        if (xauditor_group_id_begin <= group_id && group_id < xauditor_group_id_end) {
            node_type |= xnode_type_t::evm_auditor;
        } else if (xvalidator_group_id_begin <= group_id && group_id < xvalidator_group_id_end) {
            node_type |= xnode_type_t::evm_validator;
        } else {
            assert(false);
            node_type = xnode_type_t::invalid;
        }
        break;
    }

    default: {
        break;
    }

    }

    return node_type;
}

xnetwork_id_t network_id() noexcept {
    return xnetwork_id_t{static_cast<xnetwork_id_t::value_type>(top::config::to_chainid(XGET_CONFIG(chain_name)))};
}

template <>
bool broadcast<xnetwork_id_t>(xnetwork_id_t const & network_id) noexcept {
    return (network_id & xbroadcast_id_t::network) == xbroadcast_id_t::network;
}

template <>
bool broadcast<xzone_id_t>(xzone_id_t const & zone_id) noexcept {
    return (zone_id & xbroadcast_id_t::zone) == xbroadcast_id_t::zone;
}

template <>
bool broadcast<xcluster_id_t>(xcluster_id_t const & cluster_id) noexcept {
    return (cluster_id & xbroadcast_id_t::cluster) == xbroadcast_id_t::cluster;
}

template <>
bool broadcast<xgroup_id_t>(xgroup_id_t const & group_id) noexcept {
    return (group_id & xbroadcast_id_t::group) == xbroadcast_id_t::group;
}

template <>
bool broadcast<xslot_id_t>(xslot_id_t const & slot_id) noexcept {
    return (slot_id & xbroadcast_id_t::slot) == xbroadcast_id_t::slot;
}

template <>
bool broadcast<uint16_t>(uint16_t const & group_size) noexcept {
    return (group_size & xbroadcast_group_size) == xbroadcast_group_size;
}

template <>
bool broadcast<uint64_t>(uint64_t const & associated_blk_height) noexcept {
    return (associated_blk_height & xbroadcast_associated_blk_height) == xbroadcast_associated_blk_height;
}

template <>
xnetwork_id_t const & broadcast<xnetwork_id_t>() noexcept {
    static xnetwork_id_t broadcast_id{xbroadcast_network_id_value};
    return broadcast_id;
}

template <>
xzone_id_t const & broadcast<xzone_id_t>() noexcept {
    static xzone_id_t broadcast_id{xbroadcast_zone_id_value};
    return broadcast_id;
}

template <>
xcluster_id_t const & broadcast<xcluster_id_t>() noexcept {
    static xcluster_id_t broadcast_id{xbroadcast_cluster_id_value};
    return broadcast_id;
}

template <>
xgroup_id_t const & broadcast<xgroup_id_t>() noexcept {
    static xgroup_id_t broadcast_id{xbroadcast_group_id_value};
    return broadcast_id;
}

template <>
xslot_id_t const & broadcast<xslot_id_t>() noexcept {
    static xslot_id_t broadcast_id{xbroadcast_slot_id_value};
    return broadcast_id;
}

#if !defined(XCXX14)

xnetwork_id_t const xtop_broadcast_id::network{xbroadcast_network_id_value};
xzone_id_t const xtop_broadcast_id::zone{xbroadcast_zone_id_value};
xcluster_id_t const xtop_broadcast_id::cluster{xbroadcast_cluster_id_value};
xgroup_id_t const xtop_broadcast_id::group{xbroadcast_group_id_value};
xslot_id_t const xtop_broadcast_id::slot{xbroadcast_slot_id_value};

#endif

NS_END2
