// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xbasic/xerror/xerror.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xaddress.h"
#include "xcommon/xaddress_error.h"
#include "xcommon/xcodec/xmsgpack/xaccount_election_address_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xcluster_address_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xnode_address_codec.hpp"
#include "xcommon/xnode_type.h"
#include "xutility/xhash.h"

#include <type_traits>

NS_BEG2(top, common)

xtop_cluster_address::xtop_cluster_address(xip_t const & xip)
    : m_xip{ xip.network_id(), xip.zone_id(), xip.cluster_id(), xip.group_id() } {
    if (m_xip.group_id() != xbroadcast_id_t::group) {
        m_type = xnode_type_t::group | node_type_from(m_xip.zone_id(), m_xip.cluster_id(), m_xip.group_id());
    } else if (m_xip.cluster_id() != xbroadcast_id_t::cluster) {
        m_type = xnode_type_t::cluster | node_type_from(m_xip.zone_id(), m_xip.cluster_id());
    } else if (m_xip.zone_id() != xbroadcast_id_t::zone) {
        m_type = xnode_type_t::zone | node_type_from(m_xip.zone_id());
    } else {
        m_type = xnode_type_t::network;
    }
}

xtop_cluster_address::xtop_cluster_address(xnetwork_id_t const & network_id)
    : m_xip{ network_id }, m_type{ xnode_type_t::network } {
    assert(!broadcast(network_id));
    assert(broadcast(m_xip.zone_id()));
    assert(broadcast(m_xip.cluster_id()));
    assert(broadcast(m_xip.group_id()));
    assert(broadcast(m_xip.slot_id()));
    assert(m_xip.network_version() == xdefault_network_version);
}

xtop_cluster_address::xtop_cluster_address(xnetwork_id_t const & network_id,
                                           xzone_id_t const & zone_id)
    : m_xip{ network_id, zone_id }
    , m_type{ xnode_type_t::zone | node_type_from(zone_id) } {
    assert(!broadcast(network_id) && !broadcast(zone_id));
    assert(broadcast(m_xip.cluster_id()));
    assert(broadcast(m_xip.group_id()));
    assert(broadcast(m_xip.slot_id()));
    assert(m_xip.network_version() == xdefault_network_version);
}

xtop_cluster_address::xtop_cluster_address(xnetwork_id_t const & network_id,
                                           xzone_id_t const & zone_id,
                                           xcluster_id_t const & cluster_id)
    : m_xip{ network_id, zone_id, cluster_id }
    , m_type{ xnode_type_t::cluster | node_type_from(zone_id, cluster_id) } {
    assert(!broadcast(network_id));
    assert(!broadcast(zone_id));
    assert(!broadcast(cluster_id));
    assert(broadcast(m_xip.group_id()));
    assert(broadcast(m_xip.slot_id()));
    assert(m_xip.network_version() == xdefault_network_version);
}

xtop_cluster_address::xtop_cluster_address(xnetwork_id_t const & network_id,
                                           xzone_id_t const & zone_id,
                                           xcluster_id_t const & cluster_id,
                                           xgroup_id_t const & group_id)
    : m_xip{ network_id, zone_id, cluster_id, group_id } {
    if (group_id != xbroadcast_id_t::group) {
        m_type = xnode_type_t::group | node_type_from(zone_id, cluster_id, group_id);
    } else if (cluster_id != xbroadcast_id_t::cluster) {
        m_type = xnode_type_t::cluster | node_type_from(zone_id, cluster_id);
    } else if (zone_id != xbroadcast_id_t::zone) {
        m_type = xnode_type_t::zone | node_type_from(zone_id);
    } else {
        m_type = xnode_type_t::network;
    }
    assert(broadcast(m_xip.slot_id()));
    assert(m_xip.network_version() == xdefault_network_version);
}

bool
xtop_cluster_address::operator==(xtop_cluster_address const & other) const noexcept {
    return m_xip == other.m_xip;
}

bool
xtop_cluster_address::operator!=(xtop_cluster_address const & other) const noexcept {
    return !(*this == other);
}

bool
xtop_cluster_address::operator<(xtop_cluster_address const & other) const noexcept {
    return m_xip < other.m_xip;
}

bool
xtop_cluster_address::operator>(xtop_cluster_address const & other) const noexcept {
    return other < *this;
}

bool
xtop_cluster_address::operator<=(xtop_cluster_address const & other) const noexcept {
    return !(other < *this);
}

bool
xtop_cluster_address::operator>=(xtop_cluster_address const & other) const noexcept {
    return !(*this < other);
}

xnetwork_id_t
xtop_cluster_address::network_id() const noexcept {
    return m_xip.network_id();
}

xzone_id_t
xtop_cluster_address::zone_id() const noexcept {
    return m_xip.zone_id();
}

xcluster_id_t
xtop_cluster_address::cluster_id() const noexcept {
    return m_xip.cluster_id();
}

xgroup_id_t
xtop_cluster_address::group_id() const noexcept {
    return m_xip.group_id();
}

xsharding_info_t
xtop_cluster_address::sharding_info() const noexcept {
    return xsharding_info_t{ m_xip };
}

xip_t const &
xtop_cluster_address::xip() const noexcept {
    return m_xip;
}

xnode_type_t
xtop_cluster_address::type() const noexcept {
    return m_type;
}

bool
xtop_cluster_address::contains(xtop_cluster_address const & address) const noexcept {
    if (broadcast(network_id())) {
        return true;
    }

    if (network_id() != address.network_id()) {
        return false;
    }

    if (broadcast(zone_id())) {
        return true;
    }

    if (zone_id() != address.zone_id()) {
        return false;
    }

    if (broadcast(cluster_id())) {
        return true;
    }

    if (cluster_id() != address.cluster_id()) {
        return false;
    }

    if (broadcast(group_id())) {
        return true;
    }

    if (group_id() != address.group_id()) {
        return false;
    }

    return true;
}

void
xtop_cluster_address::swap(xtop_cluster_address & other) noexcept {
    m_xip.swap(other.m_xip);
    std::swap(m_type, other.m_type);
}

bool
xtop_cluster_address::empty() const noexcept {
#if defined DEBUG
    if (!m_xip.empty()) {
        assert(m_type != common::xnode_type_t::invalid);
    }
#endif
    return m_xip.empty();
}

xtop_cluster_address::hash_result_type
xtop_cluster_address::hash() const {
    return m_xip.hash();
}

std::string
xtop_cluster_address::to_string() const {
    return top::common::to_string(m_type) + "/" + m_xip.to_string();
}

std::int32_t
xtop_cluster_address::do_write(base::xstream_t & stream) const {
    try {
        return stream << codec::msgpack_encode(*this);
    } catch (...) {
        xerror("[cluster address] do_write stream failed %s", to_string().c_str());
        return -1;
    }
}

std::int32_t
xtop_cluster_address::do_read(base::xstream_t & stream) {
    try {
        xbyte_buffer_t buffer;
        auto const size = stream >> buffer;
        *this = codec::msgpack_decode<xtop_cluster_address>(buffer);
        return size;
    } catch (...) {
        xerror("[cluster address] do_read stream failed");
        return -1;
    }
}

std::int32_t xtop_cluster_address::do_write(base::xbuffer_t & buffer) const {
    try {
        return buffer << codec::msgpack_encode(*this);
    } catch (...) {
        xerror("[cluster address] do_write buffer failed %s", to_string().c_str());
        return -1;
    }
}

std::int32_t xtop_cluster_address::do_read(base::xbuffer_t & buffer) {
    try {
        xbyte_buffer_t bytes;
        auto const size = buffer >> bytes;
        *this = codec::msgpack_decode<xtop_cluster_address>(bytes);
        return size;
    } catch (...) {
        xerror("[cluster address] do_read buffer failed");
        return -1;
    }
}

std::int32_t
operator<<(base::xstream_t & stream, xtop_cluster_address const & o) {
    return o.do_write(stream);
}

std::int32_t
operator>>(base::xstream_t & stream, xtop_cluster_address & o) {
    return o.do_read(stream);
}

std::int32_t
operator<<(base::xbuffer_t & buffer, xtop_cluster_address const & o) {
    return o.do_write(buffer);
}

std::int32_t
operator>>(base::xbuffer_t & buffer, xtop_cluster_address & o) {
    return o.do_read(buffer);
}

xtop_account_election_address::xtop_account_election_address(xaccount_address_t const & account, xslot_id_t const & slot_id)
    : m_account_address{ account }, m_slot_id{ slot_id } {
}

xtop_account_election_address::xtop_account_election_address(xaccount_address_t && account, xslot_id_t && slot_id) noexcept
    : m_account_address{ std::move(account) }, m_slot_id{ std::move(slot_id) } {
}

xaccount_address_t const &
xtop_account_election_address::account_address() const noexcept {
    return m_account_address;
}

xnode_id_t const &
xtop_account_election_address::node_id() const noexcept {
    return m_account_address;
}

xslot_id_t const &
xtop_account_election_address::slot_id() const noexcept {
    return m_slot_id;
}

bool
xtop_account_election_address::empty() const noexcept {
    return m_account_address.empty();
}

void
xtop_account_election_address::swap(xtop_account_election_address & other) noexcept {
    m_account_address.swap(other.m_account_address);
    m_slot_id.swap(other.m_slot_id);
}

bool
xtop_account_election_address::operator==(xtop_account_election_address const & other) const noexcept {
    return m_account_address == other.m_account_address && m_slot_id == other.m_slot_id;
}

bool
xtop_account_election_address::operator!=(xtop_account_election_address const & other) const noexcept {
    return !(*this == other);
}

bool
xtop_account_election_address::operator<(xtop_account_election_address const & other) const noexcept {
    if (m_account_address != other.m_account_address) {
        return m_account_address < other.m_account_address;
    }

    return m_slot_id < other.m_slot_id;
}

bool
xtop_account_election_address::operator>(xtop_account_election_address const & other) const noexcept {
    return other < *this;
}

bool
xtop_account_election_address::operator<=(xtop_account_election_address const & other) const noexcept {
    return !(other < *this);
}

bool
xtop_account_election_address::operator>=(xtop_account_election_address const & other) const noexcept {
    return !(*this < other);
}

xtop_account_election_address::hash_result_type
xtop_account_election_address::hash() const {
    utl::xxh64_t hasher;

    auto const account_address_hash = m_account_address.hash();
    hasher.update(&account_address_hash, sizeof(account_address_hash));

    auto const slot_id_hash = m_slot_id.hash();
    hasher.update(&slot_id_hash, sizeof(slot_id_hash));

    return hasher.get_hash();
}

std::string
xtop_account_election_address::to_string() const {
    return m_account_address.to_string() + "/" + m_slot_id.to_string();
}

xtop_logical_version::xtop_logical_version(xelection_round_t const & election_round, std::uint16_t const group_size, std::uint64_t const associated_blk_height)
    : m_election_round{ election_round }, m_group_size{ group_size }, m_associated_blk_height{ associated_blk_height } {
}

xtop_logical_version::xtop_logical_version(std::uint16_t const group_size, std::uint64_t const associated_blk_height)
    : m_group_size{ group_size }, m_associated_blk_height{ associated_blk_height } {
}

xelection_round_t const & xtop_logical_version::election_round() const noexcept {
    return m_election_round;
}

std::uint16_t xtop_logical_version::group_size() const noexcept {
    return m_group_size;
}

std::uint64_t xtop_logical_version::associated_blk_height() const noexcept {
    return m_associated_blk_height;
}

void xtop_logical_version::swap(xtop_logical_version & other) noexcept {
    m_election_round.swap(other.m_election_round);
    std::swap(m_group_size, other.m_group_size);
    std::swap(m_associated_blk_height, other.m_associated_blk_height);
}

bool xtop_logical_version::empty() const noexcept {
    return broadcast(associated_blk_height()) && m_election_round.empty();
}

bool xtop_logical_version::has_value() const noexcept {
    return !empty();
}

bool xtop_logical_version::operator==(xtop_logical_version const & other) const noexcept {
    return m_group_size == other.m_group_size && m_associated_blk_height == other.m_associated_blk_height;
}

bool xtop_logical_version::operator!=(xtop_logical_version const & other) const noexcept {
    return !(*this == other);
}

bool xtop_logical_version::operator<(xtop_logical_version const & other) const noexcept {
    if (m_associated_blk_height != other.m_associated_blk_height) {
        return m_associated_blk_height < other.m_associated_blk_height;
    }

    if (m_election_round != other.m_election_round) {
        return m_election_round < other.m_election_round;
    }

    return false;
}

bool xtop_logical_version::operator>(xtop_logical_version const & other) const noexcept {
    return other < *this;
}

bool xtop_logical_version::operator<=(xtop_logical_version const & other) const noexcept {
    return !(other < *this);
}

bool xtop_logical_version::operator>=(xtop_logical_version const & other) const noexcept {
    return !(*this < other);
}

bool xtop_logical_version::contains(xtop_logical_version const & logical_version) const noexcept {
    if (broadcast(associated_blk_height()) || associated_blk_height() == logical_version.associated_blk_height()) {
        return true;
    } else if (associated_blk_height() != logical_version.associated_blk_height()) {
        return false;
    }

    if (election_round().empty() || election_round() == logical_version.election_round()) {
        return true;
    }

    return false;
}

xtop_logical_version::hash_result_type xtop_logical_version::hash() const {
    utl::xxh64_t hasher;

    auto const version_hash = election_round().hash();
    hasher.update(&version_hash, sizeof(version_hash));

    auto const associated_blk_height_hash = associated_blk_height();
    hasher.update(&associated_blk_height_hash, sizeof(associated_blk_height_hash));

    return hasher.get_hash();
}

std::string xtop_logical_version::to_string() const {
    return m_election_round.to_string() + "/" + (m_group_size == std::numeric_limits<std::uint16_t>::max() ? std::string{"ffff"} : std::to_string(m_group_size)) + "/" +
           (m_associated_blk_height == std::numeric_limits<std::uint64_t>::max() ? std::string{"ffffffffffffffff"} : std::to_string(m_associated_blk_height));
}

void xtop_logical_version::from_string(std::string const & /*input*/, std::error_code & /*ec*/) {
    assert(false);
}

xtop_node_address::xtop_node_address(xgroup_address_t const & group_address)
    : m_cluster_address{ group_address } {
}

xtop_node_address::xtop_node_address(xgroup_address_t const & group_address,
                                     xlogic_epoch_t const & group_logic_epoch)
    : m_cluster_address{ group_address }, m_logic_epoch{ group_logic_epoch } {
}

xtop_node_address::xtop_node_address(xgroup_address_t const & group_address,
                                     xelection_round_t const & election_round,
                                     std::uint16_t const group_size,
                                     std::uint64_t const associated_blk_height)
    : xtop_node_address{ group_address, { election_round, group_size, associated_blk_height } } {
}

xtop_node_address::xtop_node_address(xgroup_address_t const & group_address,
                                     xaccount_election_address_t const & account_election_address)
    : m_cluster_address{ group_address }, m_account_election_address{ account_election_address }
{
    if (m_cluster_address.empty()) {
        top::error::throw_error({ xaddress_errc_t::cluster_address_empty });
    }

    if (m_account_election_address.empty()) {
        top::error::throw_error({ xaddress_errc_t::account_address_empty }, m_cluster_address.to_string());
    }
}

xtop_node_address::xtop_node_address(xgroup_address_t const & group_address,
                                     xaccount_election_address_t const & account_election_address,
                                     xlogic_epoch_t const & group_logic_epoch)
    : m_cluster_address{ group_address }, m_account_election_address{ account_election_address }
    , m_logic_epoch{ group_logic_epoch }
{
    if (m_cluster_address.empty()) {
        top::error::throw_error({ xaddress_errc_t::cluster_address_empty });
    }

    if (m_account_election_address.empty()) {
        top::error::throw_error({ xaddress_errc_t::account_address_empty }, m_cluster_address.to_string());
    }

    if (m_logic_epoch.empty()) {
        top::error::throw_error({ xaddress_errc_t::version_empty }, m_cluster_address.to_string() + " " + m_account_election_address.to_string());
    }
}

xtop_node_address::xtop_node_address(xgroup_address_t const & group_address,
                                     xaccount_election_address_t const & account_election_address,
                                     xelection_round_t const & election_round,
                                     std::uint16_t const group_size,
                                     std::uint64_t const associated_blk_height)
    : xtop_node_address{ group_address, account_election_address, {election_round, group_size, associated_blk_height} } {
}

bool
xtop_node_address::operator==(xtop_node_address const & other) const noexcept {
    return m_cluster_address          == other.m_cluster_address &&
           m_account_election_address == other.m_account_election_address &&
           m_logic_epoch            == other.m_logic_epoch;
}

bool
xtop_node_address::operator!=(xtop_node_address const & other) const noexcept {
    return !(*this == other);
}

bool
xtop_node_address::operator<(xtop_node_address const & other) const noexcept {
    if (m_cluster_address != other.m_cluster_address) {
        return m_cluster_address < other.m_cluster_address;
    }

    if (m_account_election_address != other.m_account_election_address) {
        return m_account_election_address < other.m_account_election_address;
    }

    if (m_logic_epoch != other.m_logic_epoch) {
        return m_logic_epoch < other.m_logic_epoch;
    }

    return false;
}

bool
xtop_node_address::operator>(xtop_node_address const & other) const noexcept {
    return other < *this;
}

bool
xtop_node_address::operator<=(xtop_node_address const & other) const noexcept {
    return !(other < *this);
}

bool
xtop_node_address::operator>=(xtop_node_address const & other) const noexcept {
    return !(*this < other);
}

bool
xtop_node_address::empty() const noexcept {
    return m_cluster_address.empty();
}

xaccount_address_t const &
xtop_node_address::account_address() const noexcept {
    return m_account_election_address.account_address();
}

xaccount_election_address_t const &
xtop_node_address::account_election_address() const noexcept {
    return m_account_election_address;
}

xgroup_address_t const &
xtop_node_address::cluster_address() const noexcept {
    return m_cluster_address;
}

xgroup_address_t const &
xtop_node_address::sharding_address() const noexcept {
    return m_cluster_address;
}

xgroup_address_t const & xtop_node_address::group_address() const noexcept {
    return m_cluster_address;
}

xnetwork_id_t
xtop_node_address::network_id() const noexcept {
    return m_cluster_address.network_id();
}

xzone_id_t
xtop_node_address::zone_id() const noexcept {
    return m_cluster_address.zone_id();
}

xcluster_id_t
xtop_node_address::cluster_id() const noexcept {
    return m_cluster_address.cluster_id();
}

xgroup_id_t
xtop_node_address::group_id() const noexcept {
    return m_cluster_address.group_id();
}

xslot_id_t
xtop_node_address::slot_id() const noexcept {
    return m_account_election_address.slot_id();
}

xlogical_version_t const &
xtop_node_address::logical_version() const noexcept {
    return m_logic_epoch;
}

xlogic_epoch_t const & xtop_node_address::logic_epoch() const noexcept {
    return m_logic_epoch;
}

std::uint16_t
xtop_node_address::group_size() const noexcept {
    return m_logic_epoch.group_size();
}

std::uint64_t
xtop_node_address::associated_blk_height() const noexcept {
    return m_logic_epoch.associated_blk_height();
}

xip2_t
xtop_node_address::xip2() const noexcept {
    auto const & sharding_xip = m_cluster_address.xip();
    return {
        sharding_xip.network_id(),
        sharding_xip.zone_id(),
        sharding_xip.cluster_id(),
        sharding_xip.group_id(),
        m_account_election_address.slot_id(),
        group_size(),
        associated_blk_height()
    };
}

xnode_id_t const &
xtop_node_address::node_id() const noexcept {
    return m_account_election_address.node_id();
}

xelection_round_t const & xtop_node_address::election_round() const noexcept {
    return m_logic_epoch.election_round();
}

xnode_type_t
xtop_node_address::type() const noexcept {
    auto const cluster_address_type = m_cluster_address.type();
    // assert(common::has<xnode_type_t::cluster>(cluster_address_type) || common::has<xnode_type_t::zone>(cluster_address_type));

    // if (m_account_election_address.has_value()) {
    //     auto node_type = real_part_type(cluster_address_type);
    //     assert(node_type != xnode_type_t::invalid);
    //     return node_type;
    // }

    return cluster_address_type;
}

bool xtop_node_address::contains(xtop_node_address const & address) const noexcept {
    if (!group_address().contains(address.group_address())) {
        return false;
    }

    if (!logical_version().contains(address.logical_version())) {
        return false;
    }

    return account_address().empty() || account_address() == address.account_address();
}

xtop_node_address::hash_result_type
xtop_node_address::hash() const {
    utl::xxh64_t hasher;

    auto const sharding_address_hash = m_cluster_address.hash();
    hasher.update(&sharding_address_hash, sizeof(sharding_address_hash));

    auto const account_election_address_hash = m_account_election_address.hash();
    hasher.update(&account_election_address_hash, sizeof(account_election_address_hash));

    auto const logical_version_hash = logical_version().hash();
    hasher.update(&logical_version_hash, sizeof(logical_version_hash));

    return hasher.get_hash();
}

void
xtop_node_address::swap(xtop_node_address & other) noexcept {
    m_account_election_address.swap(other.m_account_election_address);
    m_cluster_address.swap(other.m_cluster_address);
    m_logic_epoch.swap(other.m_logic_epoch);
}


std::string
xtop_node_address::to_string() const {
    return m_cluster_address.to_string() + "/" + m_account_election_address.to_string() + "/" + m_logic_epoch.to_string();
}

std::int32_t
xtop_node_address::do_write(base::xstream_t & stream) const {
    try {
        return stream << codec::msgpack_encode(*this);
    } catch (...) {
        xerror("[node address] do_write failed %s", to_string().c_str());
        return -1;
    }
}

std::int32_t
xtop_node_address::do_read(base::xstream_t & stream) {
    try {
        xbyte_buffer_t buffer;
        auto const size = stream >> buffer;
        *this = codec::msgpack_decode<xtop_node_address>(buffer);
        return size;
    } catch (...) {
        xerror("[cluster address] do_read failed");
        return -1;
    }
}

std::int32_t
operator<<(base::xstream_t & stream, xtop_node_address const & o) {
    return o.do_write(stream);
}

std::int32_t
operator>>(base::xstream_t & stream, xtop_node_address & o) {
    return o.do_read(stream);
}

xgroup_address_t
build_committee_sharding_address(xnetwork_id_t const & network_id) {
    return xgroup_address_t{
        network_id,
        xcommittee_zone_id,
        xcommittee_cluster_id,
        xcommittee_group_id
    };
}

xgroup_address_t
build_zec_sharding_address(xnetwork_id_t const & network_id) {
    return xgroup_address_t{
        network_id,
        xzec_zone_id,
        xcommittee_cluster_id,
        xcommittee_group_id
    };
}

xgroup_address_t
build_edge_sharding_address(xnetwork_id_t const & network_id) {
    return xgroup_address_t{
        network_id,
        xedge_zone_id,
        xdefault_cluster_id,
        xdefault_group_id
    };
}

xgroup_address_t
build_archive_sharding_address(xgroup_id_t const & group_id /* todo delete this */, xnetwork_id_t const & network_id) {
    assert(group_id == common::xarchive_group_id);
    return xgroup_address_t{
        network_id,
        xstorage_zone_id,
        xdefault_cluster_id,
        xarchive_group_id
    };
}

xgroup_address_t
build_exchange_sharding_address(xnetwork_id_t const & network_id) {
    return xgroup_address_t{
        network_id,
        xstorage_zone_id,
        xexchange_cluster_id,
        xexchange_group_id
    };
}

xgroup_address_t build_fullnode_group_address(xnetwork_id_t const & network_id) {
    return xgroup_address_t{
        network_id,
        xfullnode_zone_id,
        xdefault_cluster_id,
        xdefault_group_id
    };
}

xgroup_address_t
build_evm_group_address(xnetwork_id_t const & network_id, common::xnode_type_t const & type) {
    if (common::has<common::xnode_type_t::evm_validator>(type)) {
        return xgroup_address_t{network_id, xevm_zone_id, xdefault_cluster_id, xvalidator_group_id_begin};
    }
    return xgroup_address_t{
        network_id, 
        xevm_zone_id, 
        xdefault_cluster_id, 
        xdefault_group_id};
}

xgroup_address_t build_relay_group_address(xnetwork_id_t const & network_id) {
    return xgroup_address_t{
        network_id, 
        xrelay_zone_id, 
        xdefault_cluster_id, 
        xdefault_group_id};
}

xgroup_address_t
build_consensus_sharding_address(xgroup_id_t const & group_id,
                                 xnetwork_id_t const & network_id) {
    return xgroup_address_t{
        network_id,
        xconsensus_zone_id,
        xdefault_cluster_id,
        group_id
    };
}

xgroup_address_t
build_network_broadcast_sharding_address(xnetwork_id_t const & network_id) {
    return xgroup_address_t{
        network_id
    };
}

xgroup_address_t
build_platform_broadcast_sharding_address() {
    return xgroup_address_t{
    };
}

xgroup_address_t
build_frozen_sharding_address(xnetwork_id_t const & network_id, xcluster_id_t const & cluster_id, xgroup_id_t const & group_id) {
    return xgroup_address_t{
        network_id,
        xfrozen_zone_id,
        cluster_id,
        group_id
    };
}

xgroup_address_t build_group_address(xnetwork_id_t const & network_id, xnode_type_t const node_type) {
    switch (node_type) {
    case xnode_type_t::rec:
        return build_committee_sharding_address(network_id);

    case xnode_type_t::zec:
        return build_zec_sharding_address(network_id);

    case xnode_type_t::edge:
        return build_edge_sharding_address(network_id);

    default:
        assert(false);
        return {};
    }
}

NS_END2

std::ostream &
operator<<(std::ostream & o, top::common::xnode_address_t const & addr) {
    return o << addr.to_string();
}

NS_BEG1(std)

std::size_t
hash<top::common::xnode_address_t>::operator()(top::common::xnode_address_t const & vnode_address) const {
    return static_cast<std::size_t>(vnode_address.hash());
}

std::size_t
hash<top::common::xgroup_address_t>::operator()(top::common::xgroup_address_t const & cluster_address) const {
    return static_cast<std::size_t>(cluster_address.hash());
}

NS_END1
