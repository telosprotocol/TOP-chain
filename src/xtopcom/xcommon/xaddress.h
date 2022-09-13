// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined (__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined (__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined (_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xmem.h"

#if defined (__clang__)
#    pragma clang diagnostic pop
#elif defined (__GNUC__)
#    pragma GCC diagnostic pop
#elif defined (_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xhashable.hpp"
#include "xcommon/xip.h"
#include "xcommon/xnode_id.h"
#include "xcommon/xnode_type.h"
#include "xcommon/xsharding_info.h"
#include "xcommon/xversion.h"

#include <functional>
#include <ostream>
#include <string>

NS_BEG2(top, common)

class xtop_cluster_address final : public xhashable_t<xtop_cluster_address> {
    xip_t m_xip{};
    xnode_type_t m_type{ xnode_type_t::invalid };

public:
    using hash_result_type = xhashable_t<xtop_cluster_address>::hash_result_type;

    xtop_cluster_address()                                         = default;
    xtop_cluster_address(xtop_cluster_address const &)             = default;
    xtop_cluster_address & operator=(xtop_cluster_address const &) = default;
    xtop_cluster_address(xtop_cluster_address &&)                  = default;
    xtop_cluster_address & operator=(xtop_cluster_address &&)      = default;
    ~xtop_cluster_address() override                               = default;

    explicit
    xtop_cluster_address(xip_t const & xip);

    explicit
    xtop_cluster_address(xnetwork_id_t const & nid);

    xtop_cluster_address(xnetwork_id_t const & nid,
                         xzone_id_t const & zid);

    xtop_cluster_address(xnetwork_id_t const & nid,
                         xzone_id_t const & zid,
                         xcluster_id_t const & cid);

    xtop_cluster_address(xnetwork_id_t const & nid,
                         xzone_id_t const & zid,
                         xcluster_id_t const & cid,
                         xgroup_id_t const & gid);

    bool
    operator==(xtop_cluster_address const & other) const noexcept;

    bool
    operator!=(xtop_cluster_address const & other) const noexcept;

    bool
    operator<(xtop_cluster_address const & other) const noexcept;

    bool
    operator>(xtop_cluster_address const & other) const noexcept;

    bool
    operator<=(xtop_cluster_address const & other) const noexcept;

    bool
    operator>=(xtop_cluster_address const & other) const noexcept;

    bool
    empty() const noexcept;

    xnetwork_id_t
    network_id() const noexcept;

    xzone_id_t
    zone_id() const noexcept;

    xcluster_id_t
    cluster_id() const noexcept;

    xgroup_id_t
    group_id() const noexcept;

    xsharding_info_t
    sharding_info() const noexcept;

    xip_t const &
    xip() const noexcept;

    xnode_type_t
    type() const noexcept;

    bool
    contains(xtop_cluster_address const & address) const noexcept;

    hash_result_type
    hash() const override;

    std::string
    to_string() const;

    void
    swap(xtop_cluster_address & other) noexcept;

    friend std::int32_t operator <<(base::xstream_t & stream, xtop_cluster_address const & o);

    friend std::int32_t operator >>(base::xstream_t & stream, xtop_cluster_address & o);

    friend std::int32_t operator <<(base::xbuffer_t & stream, xtop_cluster_address const & o);

    friend std::int32_t operator >>(base::xbuffer_t & stream, xtop_cluster_address & o);

private:
    std::int32_t
    do_write(base::xstream_t & stream) const;

    std::int32_t
    do_read(base::xstream_t & stream);

    std::int32_t do_write(base::xbuffer_t & buffer) const;
    std::int32_t do_read(base::xbuffer_t & buffer);
};
using xcluster_address_t = xtop_cluster_address;
using xsharding_address_t = xcluster_address_t;
using xgroup_address_t = xtop_cluster_address;

std::int32_t operator <<(base::xstream_t & stream, xcluster_address_t const & o);
std::int32_t operator >>(base::xstream_t & stream, xcluster_address_t & o);
std::int32_t operator <<(base::xbuffer_t & buffer, xcluster_address_t const & o);
std::int32_t operator >>(base::xbuffer_t & buffer, xcluster_address_t & o);

class xtop_account_election_address final : public xhashable_t<xtop_account_election_address> {
    xaccount_address_t m_account_address{};
    xslot_id_t m_slot_id{};

public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_account_election_address);
    XDECLARE_DEFAULTED_COPY_AND_MOVE_SEMANTICS(xtop_account_election_address);
    XDECLARE_DEFAULTED_DESTRUCTOR(xtop_account_election_address);

    using hash_result_type = xhashable_t<xtop_account_election_address>::hash_result_type;

    xtop_account_election_address(xaccount_address_t const & account, xslot_id_t const & slot_id);

    xtop_account_election_address(xaccount_address_t && account, xslot_id_t && slot_id) noexcept;

    xaccount_address_t const &
    account_address() const noexcept;

    xnode_id_t const &
    node_id() const noexcept;

    xslot_id_t const &
    slot_id() const noexcept;

    void
    swap(xtop_account_election_address & other) noexcept;

    bool
    empty() const noexcept;

    bool
    operator==(xtop_account_election_address const & other) const noexcept;

    bool
    operator!=(xtop_account_election_address const & other) const noexcept;

    bool
    operator<(xtop_account_election_address const & other) const noexcept;

    bool
    operator<=(xtop_account_election_address const & other) const noexcept;

    bool
    operator>(xtop_account_election_address const & other) const noexcept;

    bool
    operator>=(xtop_account_election_address const & other) const noexcept;

    hash_result_type
    hash() const override;

    std::string
    to_string() const;
};
using xaccount_election_address_t = xtop_account_election_address;

class xtop_logical_version final : public xhashable_t<xtop_logical_version>
                                 , public xenable_to_string_t<xtop_logical_version> {
    xelection_round_t m_election_round{};
    std::uint16_t m_group_size{std::numeric_limits<std::uint16_t>::max()};
    std::uint64_t m_associated_blk_height{std::numeric_limits<std::uint64_t>::max()};

public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_logical_version);
    XDECLARE_DEFAULTED_COPY_AND_MOVE_SEMANTICS(xtop_logical_version);
    XDECLARE_DEFAULTED_DESTRUCTOR(xtop_logical_version);

    using hash_result_type = xhashable_t<xtop_logical_version>::hash_result_type;

    xtop_logical_version(xelection_round_t const & election_round, std::uint16_t const sharding_size, std::uint64_t const associated_blk_height);
    xtop_logical_version(std::uint16_t const sharding_size, std::uint64_t const associated_blk_height);

    xelection_round_t const & election_round() const noexcept;

    std::uint16_t group_size() const noexcept;

    std::uint64_t associated_blk_height() const noexcept;

    void swap(xtop_logical_version & other) noexcept;

    bool empty() const noexcept;

    bool has_value() const noexcept;

    bool operator==(xtop_logical_version const & other) const noexcept;

    bool operator!=(xtop_logical_version const & other) const noexcept;

    bool operator<(xtop_logical_version const & other) const noexcept;

    bool operator>(xtop_logical_version const & other) const noexcept;

    bool operator<=(xtop_logical_version const & other) const noexcept;

    bool operator>=(xtop_logical_version const & other) const noexcept;

    bool contains(xtop_logical_version const & logical_version) const noexcept;

    hash_result_type hash() const override;

    std::string to_string() const override;

    void from_string(std::string const & input, std::error_code & ec) override;
};
using xlogical_version_t = xtop_logical_version;
using xlogic_epoch_t = xtop_logical_version;

class xtop_node_address final : public xhashable_t<xtop_node_address>
{
private:
    xgroup_address_t m_cluster_address{};
    xtop_account_election_address m_account_election_address{};
    xlogic_epoch_t m_logic_epoch{};

public:
    using hash_result_type = xhashable_t<xtop_node_address>::hash_result_type;

    xtop_node_address()                                      = default;
    xtop_node_address(xtop_node_address const &)             = default;
    xtop_node_address & operator=(xtop_node_address const &) = default;
    xtop_node_address(xtop_node_address &&)                  = default;
    xtop_node_address & operator=(xtop_node_address &&)      = default;
    ~xtop_node_address()                                     = default;

    explicit
    xtop_node_address(xgroup_address_t const & group_address);

    xtop_node_address(xgroup_address_t const & group_address,
                      xlogic_epoch_t const & group_logic_epoch);

    xtop_node_address(xgroup_address_t const & group_address,
                      xelection_round_t const & election_round,
                      std::uint16_t const sharding_size,
                      std::uint64_t const associated_blk_height);

    xtop_node_address(xgroup_address_t const & group_address,
                      xaccount_election_address_t const & account_election_address);

    xtop_node_address(xgroup_address_t const & group_address,
                      xaccount_election_address_t const & account_election_address,
                      xlogic_epoch_t const & group_logic_epoch);

    xtop_node_address(xgroup_address_t const & group_address,
                      xaccount_election_address_t const & account_election_address,
                      xelection_round_t const & election_round,
                      std::uint16_t const sharding_size,
                      std::uint64_t const associated_blk_height);

    bool
    operator==(xtop_node_address const & other) const noexcept;

    bool
    operator!=(xtop_node_address const & other) const noexcept;

    bool
    operator<(xtop_node_address const & other) const noexcept;

    bool
    operator>(xtop_node_address const & other) const noexcept;

    bool
    operator<=(xtop_node_address const & other) const noexcept;

    bool
    operator>=(xtop_node_address const & other) const noexcept;

    bool
    empty() const noexcept;

    xaccount_address_t const &
    account_address() const noexcept;

    xaccount_election_address_t const &
    account_election_address() const noexcept;

    xgroup_address_t const &
    cluster_address() const noexcept;

    xgroup_address_t const &
    sharding_address() const noexcept;

    xgroup_address_t const & group_address() const noexcept;

    xnetwork_id_t
    network_id() const noexcept;

    xzone_id_t
    zone_id() const noexcept;

    xcluster_id_t
    cluster_id() const noexcept;

    xgroup_id_t
    group_id() const noexcept;

    xslot_id_t
    slot_id() const noexcept;

    xnode_id_t const &
    node_id() const noexcept;

    xlogical_version_t const &
    logical_version() const noexcept;

    xlogic_epoch_t const & logic_epoch() const noexcept;

    xelection_round_t const & election_round() const noexcept;

    std::uint16_t group_size() const noexcept;

    std::uint64_t
    associated_blk_height() const noexcept;

    xip2_t
    xip2() const noexcept;

    xnode_type_t
    type() const noexcept;

    bool contains(xtop_node_address const & address) const noexcept;

    hash_result_type
    hash() const override;

    std::string
    to_string() const;

    void
    swap(xtop_node_address & other) noexcept;

    friend
    std::int32_t
    operator <<(base::xstream_t & stream, xtop_node_address const & o);

    friend
    std::int32_t
    operator >>(base::xstream_t & stream, xtop_node_address & o);

private:
    std::int32_t
    do_write(base::xstream_t & stream) const;

    std::int32_t
    do_read(base::xstream_t & stream);
};
using xnode_address_t = xtop_node_address;

xgroup_address_t
build_committee_sharding_address(xnetwork_id_t const & network_id);

xgroup_address_t
build_zec_sharding_address(xnetwork_id_t const & network_id);

xgroup_address_t
build_evm_group_address(xnetwork_id_t const & network_id, common::xnode_type_t const & type);

xgroup_address_t 
build_relay_group_address(xnetwork_id_t const & network_id);

xgroup_address_t
build_edge_sharding_address(xnetwork_id_t const & network_id);

xgroup_address_t
build_archive_sharding_address(xgroup_id_t const & group_id, xnetwork_id_t const & network_id);

xgroup_address_t
build_exchange_sharding_address(xnetwork_id_t const & network_id);

xgroup_address_t 
build_fullnode_group_address(xnetwork_id_t const & network_id);

xgroup_address_t 
build_evm_group_address(xnetwork_id_t const & network_id);

xgroup_address_t
build_consensus_sharding_address(xgroup_id_t const & group_id, xnetwork_id_t const & network_id = xtopchain_network_id);

xgroup_address_t
build_network_broadcast_sharding_address(xnetwork_id_t const & network_id);

xgroup_address_t
build_platform_broadcast_sharding_address();

xgroup_address_t
build_frozen_sharding_address(xnetwork_id_t const & network_id = xtopchain_network_id, xcluster_id_t const & cluster_id = xdefault_cluster_id, xgroup_id_t const & group_id = xdefault_group_id);

xgroup_address_t
build_group_address(xnetwork_id_t const & network_id, xnode_type_t const node_type);

NS_END2

std::ostream &
operator<<(std::ostream & o, top::common::xnode_address_t const & addr);

NS_BEG1(std)

template <>
struct hash<top::common::xnode_address_t> final
{
    std::size_t
    operator()(top::common::xnode_address_t const & vnode_address) const;
};

template <>
struct hash<top::common::xgroup_address_t> final
{
    std::size_t
    operator()(top::common::xgroup_address_t const & cluster_address) const;
};

NS_END1
