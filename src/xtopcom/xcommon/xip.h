// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xvledger/xvaccount.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xextended.h"
#include "xbasic/xid.hpp"
#include "xcommon/xnode_type.h"
#include "xcommon/xversion.h"

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

NS_BEG2(top, common)

using xnetwork_type_t = enum_xnetwork_type;
using xaddress_domain_t = enum_xaddress_domain;
using xip_type_t = enum_xip_type;

struct xtag_network_version {};
struct xtag_network_id {};
struct xtag_zone_id {};
struct xtag_cluster_id {};
struct xtag_group_id {};
struct xtag_slot_id {};

using xnetwork_version_t = xsimple_id_t<xtag_network_version, std::uint8_t, 0, 0x07>;
using xnetwork_id_t = xsimple_id_t<xtag_network_id, std::uint32_t, 0, 0x001FFFFF>;
using xzone_id_t = xsimple_id_t<xtag_zone_id, std::uint8_t, 0, 0x7F>;
using xcluster_id_t = xsimple_id_t<xtag_cluster_id, std::uint8_t, 0, 0x7F>;
using xgroup_id_t = xsimple_id_t<xtag_group_id, std::uint8_t, 0, 0xFF>;
using xslot_id_t = xsimple_id_t<xtag_slot_id, std::uint16_t, 0, 0x3FF>;

XINLINE_CONSTEXPR xnetwork_id_t::value_type xbroadcast_network_id_value{0x001FFFFF};
XINLINE_CONSTEXPR xzone_id_t::value_type xbroadcast_zone_id_value{0x7F};
XINLINE_CONSTEXPR xcluster_id_t::value_type xbroadcast_cluster_id_value{0x7F};
XINLINE_CONSTEXPR xgroup_id_t::value_type xbroadcast_group_id_value{0xFF};
XINLINE_CONSTEXPR xslot_id_t::value_type xbroadcast_slot_id_value{0x03FF};
XINLINE_CONSTEXPR uint16_t xbroadcast_group_size{0x03FF};
XINLINE_CONSTEXPR uint64_t xbroadcast_associated_blk_height{0x3FFFFFFFFFFFFF};

XINLINE_CONSTEXPR xnetwork_id_t::value_type xmax_network_id_value{xbroadcast_network_id_value - 1};
XINLINE_CONSTEXPR xzone_id_t::value_type xmax_zone_id_value{xbroadcast_zone_id_value - 1};
XINLINE_CONSTEXPR xcluster_id_t::value_type xmax_cluster_id_value{xbroadcast_cluster_id_value - 1};
XINLINE_CONSTEXPR xgroup_id_t::value_type xmax_group_id_value{xbroadcast_group_id_value - 1};
XINLINE_CONSTEXPR xslot_id_t::value_type xmax_slot_id_value{xbroadcast_slot_id_value - 1};

XINLINE_CONSTEXPR xnetwork_version_t::value_type xdefault_network_version_value{0x00};

XINLINE_CONSTEXPR std::uint16_t xdefault_group_size_value{0x03FF};
XINLINE_CONSTEXPR std::uint64_t xdefault_associated_blk_height_value{0x3FFFFFFFFFFFFF};

struct xtop_broadcast_id final {
#if defined(XCXX14)
    static XINLINE_CONSTEXPR xnetwork_id_t network{xbroadcast_network_id_value};
#else
    static xnetwork_id_t const network;
#endif

#if defined(XCXX14)
    static XINLINE_CONSTEXPR xzone_id_t zone{xbroadcast_zone_id_value};
#else
    static xzone_id_t const zone;
#endif

#if defined(XCXX14)
    static XINLINE_CONSTEXPR xcluster_id_t cluster{xbroadcast_cluster_id_value};
#else
    static xcluster_id_t const cluster;
#endif

#if defined(XCXX14)
    static XINLINE_CONSTEXPR xgroup_id_t group{xbroadcast_group_id_value};
#else
    static xgroup_id_t const group;
#endif

#if defined(XCXX14)
    static XINLINE_CONSTEXPR xslot_id_t slot{xbroadcast_slot_id_value};
#else
    static xslot_id_t const slot;
#endif
};
using xbroadcast_id_t = xtop_broadcast_id;

/**
 * @brief Structured XIP
 */
class xtop_ip final {
public:
    using value_type = xvip_t;

private:
    value_type m_xip{ 0xFF1FFFFFFFFFFFFF };// network version defaults to ZERO. Don't use network version field for now.

public:
    xtop_ip() = default;
    xtop_ip(xtop_ip const &) = default;
    xtop_ip & operator=(xtop_ip const &) = default;
    xtop_ip(xtop_ip &&) = default;
    xtop_ip & operator=(xtop_ip &&) = default;
    ~xtop_ip() = default;

    explicit xtop_ip(value_type const ip) noexcept : m_xip{ip} {
    }

    xtop_ip(xtop_ip const & xip, xaddress_domain_t const domain) noexcept;

    explicit xtop_ip(xnetwork_id_t const & network_id, xaddress_domain_t const domain = xaddress_domain_t::enum_xaddress_domain_xip) noexcept;

    xtop_ip(xnetwork_id_t const & network_id, xzone_id_t const & zone_id, xaddress_domain_t const domain = xaddress_domain_t::enum_xaddress_domain_xip) noexcept;

    xtop_ip(xnetwork_id_t const & network_id,
            xzone_id_t const & zone_id,
            xcluster_id_t const & cluster_id,
            xaddress_domain_t const domain = xaddress_domain_t::enum_xaddress_domain_xip) noexcept;

    xtop_ip(xnetwork_id_t const & network_id,
            xzone_id_t const & zone_id,
            xcluster_id_t const & cluster_id,
            xgroup_id_t const & group_id,
            xaddress_domain_t const domain = xaddress_domain_t::enum_xaddress_domain_xip) noexcept;

    xtop_ip(xnetwork_id_t const & network_id,
            xzone_id_t const & zone_id,
            xcluster_id_t const & cluster_id,
            xgroup_id_t const & group_id,
            xslot_id_t const & slot_id,
            xaddress_domain_t const domain = xaddress_domain_t::enum_xaddress_domain_xip) noexcept;

    void swap(xtop_ip & other) noexcept;

    bool operator==(xtop_ip const & other) const noexcept;

    bool operator<(xtop_ip const & other) const noexcept;

    bool operator!=(xtop_ip const & other) const noexcept;

    // static constexpr xaddress_domain_t address_domain() noexcept { return xaddress_domain_t::enum_xaddress_domain_xip; }
    xaddress_domain_t address_domain() const noexcept;

    bool empty() const noexcept;

    common::xip_type_t type() const noexcept;

    common::xnetwork_type_t network_type() const noexcept;

    xnetwork_version_t network_version() const noexcept;

    xnetwork_id_t network_id() const noexcept;

    xzone_id_t zone_id() const noexcept;

    xcluster_id_t cluster_id() const noexcept;

    xgroup_id_t group_id() const noexcept;

    xslot_id_t slot_id() const noexcept;

    operator value_type() const noexcept;

    std::size_t hash() const;

    std::string to_string() const;

    value_type value() const noexcept;

    xtop_ip group_xip() const noexcept;
};
using xip_t = xtop_ip;

std::string to_string(xnetwork_version_t const & network_version);

std::string to_string(xnetwork_id_t const & network_id);

std::string to_string(xzone_id_t const & zone_id);

std::string to_string(xcluster_id_t const & cluster_id);

std::string to_string(xgroup_id_t const & group_id);

std::string to_string(xslot_id_t const & slot_id);

NS_END2

NS_BEG1(top)

template <>
class xtop_extended<common::xip_t> final {
    common::xip_t m_xip{};
    std::uint64_t m_extent{std::numeric_limits<std::uint64_t>::max()};

public:
    xtop_extended() = default;
    xtop_extended(xtop_extended const &) = default;
    xtop_extended & operator=(xtop_extended const &) = default;
    xtop_extended(xtop_extended &&) = default;
    xtop_extended & operator=(xtop_extended &&) = default;
    ~xtop_extended() = default;

    xtop_extended(xvip2_t const & xvip2) noexcept;

    xtop_extended(std::uint64_t const low_part, std::uint64_t const high_part) noexcept;

    xtop_extended(common::xip_t const & low, std::uint64_t const high, common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2) noexcept;

    explicit xtop_extended(common::xnetwork_id_t const & network_id, common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2);

    xtop_extended(common::xnetwork_id_t const & network_id,
                  common::xzone_id_t const & zone_id,
                  common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2);

    xtop_extended(common::xnetwork_id_t const & network_id,
                  common::xzone_id_t const & zone_id,
                  common::xcluster_id_t const & cluster_id,
                  common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2);

    xtop_extended(common::xnetwork_id_t const & network_id,
                  common::xzone_id_t const & zone_id,
                  common::xcluster_id_t const & cluster_id,
                  common::xgroup_id_t const & group_id,
                  common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2);

    xtop_extended(common::xnetwork_id_t const & network_id,
                  common::xzone_id_t const & zone_id,
                  common::xcluster_id_t const & cluster_id,
                  common::xgroup_id_t const & group_id,
                  uint16_t const size,
                  uint64_t const height,
                  common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2);

    xtop_extended(common::xnetwork_id_t const & network_id,
                  common::xzone_id_t const & zone_id,
                  common::xcluster_id_t const & cluster_id,
                  common::xgroup_id_t const & group_id,
                  common::xslot_id_t const & slot_id,
                  common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2);

    xtop_extended(common::xnetwork_id_t const & network_id,
                  common::xzone_id_t const & zone_id,
                  common::xcluster_id_t const & cluster_id,
                  common::xgroup_id_t const & group_id,
                  common::xslot_id_t const & slot_id,
                  uint16_t const size,
                  uint64_t const height,
                  common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2);

private:
    explicit xtop_extended(common::xip_t const & low, common::xaddress_domain_t const domain = common::xaddress_domain_t::enum_xaddress_domain_xip2) noexcept;

public:
    void swap(xtop_extended & other) noexcept;

    bool operator==(xtop_extended const & other) const noexcept;

    bool operator!=(xtop_extended const & other) const noexcept;

    common::xaddress_domain_t address_domain() const noexcept;

    common::xip_type_t type() const noexcept;

    common::xnetwork_type_t network_type() const noexcept;

    // common::xnetwork_version_t network_version() const noexcept;

    common::xnetwork_id_t network_id() const noexcept;

    common::xzone_id_t zone_id() const noexcept;

    common::xcluster_id_t cluster_id() const noexcept;

    common::xgroup_id_t group_id() const noexcept;

    common::xslot_id_t slot_id() const noexcept;

    uint16_t size() const noexcept;

    uint64_t height() const noexcept;

    common::xip_t xip() const noexcept;

    std::uint64_t raw_high_part() const noexcept;

    std::uint64_t raw_low_part() const noexcept;

    std::size_t hash() const;

    std::string to_string() const;

    xvip2_t value() const noexcept;

    xtop_extended group_xip2() const noexcept {
        return xtop_extended{xip().group_xip()};
    }

    operator xvip2_t() const noexcept;
};

NS_END1

NS_BEG2(top, common)

using xip2_t = top::xextended_t<top::common::xip_t>;

xnode_type_t node_type_from(xzone_id_t const & zone_id);

xnode_type_t node_type_from(xzone_id_t const & zone_id, xcluster_id_t const & cluster_id);

xnode_type_t node_type_from(xzone_id_t const & zone_id, xcluster_id_t const & cluster_id, xgroup_id_t const & group_id);

xnetwork_id_t network_id() noexcept;

#if defined(XCXX14)
XINLINE_CONSTEXPR xnetwork_version_t
#else
xnetwork_version_t const
#endif
    xdefault_network_version{xdefault_network_version_value};

///////////////////////////////////////////////////////////////////////////////
/// pre-defined network ids
///////////////////////////////////////////////////////////////////////////////

#if defined(XCXX14)
XINLINE_CONSTEXPR xnetwork_id_t
#else
xnetwork_id_t const
#endif
    xbeacon_network_id{static_cast<xzone_id_t::value_type>(base::enum_main_chain_id)};

#if defined(XCXX14)
XINLINE_CONSTEXPR xnetwork_id_t
#else
xnetwork_id_t const
#endif
    xtopchain_network_id{static_cast<xzone_id_t::value_type>(base::enum_main_chain_id)};

/**
 * @brief testnet network id
 */
XINLINE_CONSTEXPR xnetwork_id_t::value_type xtestnet_id_value{base::enum_test_chain_id};

#if defined(XCXX14)
XINLINE_CONSTEXPR xnetwork_id_t
#else
xnetwork_id_t const
#endif
    xtestnet_id{xtestnet_id_value};

///////////////////////////////////////////////////////////////////////////////
/// pre-defined zone ids
///////////////////////////////////////////////////////////////////////////////

XINLINE_CONSTEXPR xzone_id_t::value_type xcommittee_zone_id_value{base::enum_chain_zone_beacon_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xcommittee_zone_id{xcommittee_zone_id_value};

XINLINE_CONSTEXPR xzone_id_t::value_type xzec_zone_id_value{base::enum_chain_zone_zec_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xzec_zone_id{xzec_zone_id_value};

XINLINE_CONSTEXPR xzone_id_t::value_type xconsensus_zone_id_value{base::enum_chain_zone_consensus_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xconsensus_zone_id{xconsensus_zone_id_value};

XINLINE_CONSTEXPR xzone_id_t::value_type xfrozen_zone_id_value{base::enum_chain_zone_frozen_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xfrozen_zone_id{xfrozen_zone_id_value};

XINLINE_CONSTEXPR xzone_id_t::value_type xevm_zone_id_value{base::enum_chain_zone_evm_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xevm_zone_id{xevm_zone_id_value};

XINLINE_CONSTEXPR xzone_id_t::value_type xrelay_zone_id_value{base::enum_chain_zone_relay_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xrelay_zone_id{xrelay_zone_id_value};

XINLINE_CONSTEXPR xzone_id_t::value_type xfullnode_zone_id_value{base::enum_chain_zone_fullnode_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xfullnode_zone_id{xfullnode_zone_id_value};

/**
 * @brief default archive zone id
 */
XINLINE_CONSTEXPR xzone_id_t::value_type xstorage_zone_id_value{base::enum_chain_zone_storage_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xstorage_zone_id{xstorage_zone_id_value};

/**
 * @brief default edge zone id
 */
XINLINE_CONSTEXPR xzone_id_t::value_type xedge_zone_id_value{base::enum_chain_zone_edge_index};
#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xedge_zone_id{xedge_zone_id_value};

/**
 * @brief Default zone id
 */
XINLINE_CONSTEXPR xzone_id_t::value_type xdefault_zone_id_value{xconsensus_zone_id_value};

#if defined(XCXX14)
XINLINE_CONSTEXPR xzone_id_t
#else
xzone_id_t const
#endif
    xdefault_zone_id{xdefault_zone_id_value};

///////////////////////////////////////////////////////////////////////////////
/// pre-defined cluster ids
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief topnetwork the begin of customized cluster id
 */
XINLINE_CONSTEXPR xcluster_id_t::value_type xcommittee_cluster_id_value{0};
XINLINE_CONSTEXPR xcluster_id_t::value_type xdefault_cluster_id_value{1};
XINLINE_CONSTEXPR xcluster_id_t::value_type xexchange_cluster_id_value{2};
XINLINE_CONSTEXPR xcluster_id_t::value_type xnormal_cluster_id_value_begin{1};
XINLINE_CONSTEXPR xcluster_id_t::value_type xnormal_cluster_id_value_end{127};

#if defined(XCXX14)
XINLINE_CONSTEXPR xcluster_id_t
#else
xcluster_id_t const
#endif
    xcommittee_cluster_id{xcommittee_cluster_id_value};

#if defined(XCXX14)
XINLINE_CONSTEXPR xcluster_id_t
#else
xcluster_id_t const
#endif
    xdefault_cluster_id{xdefault_cluster_id_value};

#if defined(XCXX14)
XINLINE_CONSTEXPR xcluster_id_t
#else
xcluster_id_t const
#endif
    xexchange_cluster_id{xexchange_cluster_id_value};

#if defined(XCXX14)
XINLINE_CONSTEXPR xcluster_id_t
#else
xcluster_id_t const
#endif
    xnormal_cluster_id_begin{xnormal_cluster_id_value_begin};

#if defined(XCXX14)
XINLINE_CONSTEXPR xcluster_id_t
#else
xcluster_id_t const
#endif
    xnormal_cluster_id_end{xnormal_cluster_id_value_end};

#if defined(XCXX14)
XINLINE_CONSTEXPR xcluster_id_t
#else
xcluster_id_t const
#endif
    xmax_cluster_id{xmax_cluster_id_value};

///////////////////////////////////////////////////////////////////////////////
/// pre-defined group ids
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief default topnetwork committee group id
 */
XINLINE_CONSTEXPR xgroup_id_t::value_type xcommittee_group_id_value{0};
XINLINE_CONSTEXPR xgroup_id_t::value_type xunassociatable_group_id_value{0};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xcommittee_group_id{xcommittee_group_id_value};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xunassociatable_group_id{xunassociatable_group_id_value};

/*
 * @brief default topnetwork auditor group id range
 */
// [1, 64)
XINLINE_CONSTEXPR xgroup_id_t::value_type xauditor_group_id_value_begin{1};
XINLINE_CONSTEXPR xgroup_id_t::value_type xauditor_group_id_value_end{64};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xauditor_group_id_begin{xauditor_group_id_value_begin};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xauditor_group_id_end{xauditor_group_id_value_end};

// [64, 127)
XINLINE_CONSTEXPR xgroup_id_t::value_type xvalidator_group_id_value_begin{64};
XINLINE_CONSTEXPR xgroup_id_t::value_type xvalidator_group_id_value_end{127};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xvalidator_group_id_begin{xvalidator_group_id_value_begin};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xvalidator_group_id_end{xvalidator_group_id_value_end};

XINLINE_CONSTEXPR xgroup_id_t::value_type xarchive_group_id_value_begin{ 1 };
XINLINE_CONSTEXPR xgroup_id_t::value_type xarchive_group_id_value{ 1 };
XINLINE_CONSTEXPR xgroup_id_t::value_type xlegacy_exchange_group_id_value{ 2 };
XINLINE_CONSTEXPR xgroup_id_t::value_type xexchange_group_id_value{ 1 };
XINLINE_CONSTEXPR xgroup_id_t::value_type xarchive_group_id_value_end{ 3 };

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
xarchive_group_id_begin{ xarchive_group_id_value_begin };

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
xarchive_group_id{ xarchive_group_id_value_begin };

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
xlegacy_exchange_group_id{ xlegacy_exchange_group_id_value };

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
xexchange_group_id{ xexchange_group_id_value };

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
xarchive_group_id_end{ xarchive_group_id_value_end };

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xmax_group_id{xmax_group_id_value};

XINLINE_CONSTEXPR xgroup_id_t::value_type xdefault_group_id_value{1};
XINLINE_CONSTEXPR xgroup_id_t::value_type xnormal_group_id_value_begin{1};
XINLINE_CONSTEXPR xgroup_id_t::value_type xnormal_group_id_value_end{127};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xdefault_group_id{xdefault_group_id_value};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xnormal_group_id_begin{xnormal_group_id_value_begin};

#if defined(XCXX14)
XINLINE_CONSTEXPR xgroup_id_t
#else
xgroup_id_t const
#endif
    xnormal_group_id_end{xnormal_group_id_value_end};

template <typename IdT, typename std::enable_if<std::is_same<IdT, xnetwork_id_t>::value>::type * = nullptr>
IdT get(xip2_t const & xip2) noexcept {
    return xip2.network_id();
}

template <typename IdT, typename std::enable_if<std::is_same<IdT, xzone_id_t>::value>::type * = nullptr>
IdT get(xip2_t const & xip2) noexcept {
    return xip2.zone_id();
}

template <typename IdT, typename std::enable_if<std::is_same<IdT, xcluster_id_t>::value>::type * = nullptr>
IdT get(xip2_t const & xip2) noexcept {
    return xip2.cluster_id();
}

template <typename IdT, typename std::enable_if<std::is_same<IdT, xgroup_id_t>::value>::type * = nullptr>
IdT get(xip2_t const & xip2) noexcept {
    return xip2.group_id();
}

template <typename IdT, typename std::enable_if<std::is_same<IdT, xslot_id_t>::value>::type * = nullptr>
IdT get(xip2_t const & xip2) noexcept {
    return xip2.slot_id();
}

template <typename NullableIdT>
bool broadcast(NullableIdT const & id) noexcept;

template <>
bool broadcast<xnetwork_id_t>(xnetwork_id_t const & network_id) noexcept;

template <>
bool broadcast<xzone_id_t>(xzone_id_t const & zone_id) noexcept;

template <>
bool broadcast<xcluster_id_t>(xcluster_id_t const & cluster_id) noexcept;

template <>
bool broadcast<xgroup_id_t>(xgroup_id_t const & group_id) noexcept;

template <>
bool broadcast<xslot_id_t>(xslot_id_t const & slot_id) noexcept;

template <>
bool broadcast<uint16_t>(uint16_t const & group_size) noexcept;

template <>
bool broadcast<uint64_t>(uint64_t const & associated_blk_height) noexcept;

template <typename NullableIdT>
NullableIdT const & broadcast() noexcept;

template <>
xnetwork_id_t const & broadcast<xnetwork_id_t>() noexcept;

template <>
xzone_id_t const & broadcast<xzone_id_t>() noexcept;

template <>
xcluster_id_t const & broadcast<xcluster_id_t>() noexcept;

template <>
xgroup_id_t const & broadcast<xgroup_id_t>() noexcept;

template <>
xslot_id_t const & broadcast<xslot_id_t>() noexcept;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xip2_t> final {
    std::size_t operator()(top::common::xip2_t const & xip2) const noexcept {
        return xip2.hash();
    }
};

template <>
struct hash<top::common::xip_t> final {
    std::size_t operator()(top::common::xip_t const & xip) const noexcept {
        return xip.hash();
    }
};

NS_END1
