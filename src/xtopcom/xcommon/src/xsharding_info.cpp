// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xsharding_info_codec.hpp"
#include "xcommon/xsharding_info.h"
#include "xutility/xhash.h"

#include <cstring>

NS_BEG2(top, common)

xtop_sharding_info::xtop_sharding_info(xip_t const & xip) noexcept
    : xtop_sharding_info{ xip.network_id(), xip.zone_id(), xip.cluster_id(), xip.group_id() } {
}

xtop_sharding_info::xtop_sharding_info(xnetwork_id_t const & nid) noexcept
    : m_nid{ nid } {
}

xtop_sharding_info::xtop_sharding_info(xnetwork_id_t const & nid,
                                       xzone_id_t const & zid) noexcept
    : m_nid{ nid }, m_zid{ zid } {
}

xtop_sharding_info::xtop_sharding_info(xnetwork_id_t const & nid,
                                       xzone_id_t const & zid,
                                       xcluster_id_t const & cid) noexcept
    : m_nid{ nid }, m_zid{ zid }, m_cid{ cid } {
}

xtop_sharding_info::xtop_sharding_info(xnetwork_id_t const & nid,
                                       xzone_id_t const & zid,
                                       xcluster_id_t const & cid,
                                       xgroup_id_t const & gid) noexcept
    : m_nid{ nid }, m_zid{ zid }, m_cid{ cid }, m_gid{ gid } {
}

bool
xtop_sharding_info::operator==(xtop_sharding_info const & other) const noexcept {
    return m_gid == other.m_gid &&
           m_cid == other.m_cid &&
           m_zid == other.m_zid &&
           m_nid == other.m_nid;
}

bool
xtop_sharding_info::operator!=(xtop_sharding_info const & other) const noexcept {
    return !(*this == other);
}

bool
xtop_sharding_info::operator<(xtop_sharding_info const & other) const noexcept {
    if (m_nid != other.m_nid) {
        return m_nid < other.m_nid;
    }

    if (m_zid != other.m_zid) {
        return m_zid < other.m_zid;
    }

    if (m_cid != other.m_cid) {
        return m_cid < other.m_cid;
    }

    if (m_gid != other.m_gid) {
        return m_gid < other.m_gid;
    }

    return false;
}

bool
xtop_sharding_info::operator>(xtop_sharding_info const & other) const noexcept {
    return other < *this;
}

bool
xtop_sharding_info::operator<=(xtop_sharding_info const & other) const noexcept {
    return !(other < *this);
}

bool
xtop_sharding_info::operator>=(xtop_sharding_info const & other) const noexcept {
    return !(*this < other);
}

void
xtop_sharding_info::swap(xtop_sharding_info & other) noexcept {
    m_nid.swap(other.m_nid);
    m_zid.swap(other.m_zid);
    m_cid.swap(other.m_cid);
    m_gid.swap(other.m_gid);
}

//bool
//xtop_sharding_info::empty() const noexcept {
//    return m_nid.empty();
//}

xnetwork_id_t const &
xtop_sharding_info::network_id() const noexcept {
    return m_nid;
}

xzone_id_t const &
xtop_sharding_info::zone_id() const noexcept {
    return m_zid;
}

xcluster_id_t const &
xtop_sharding_info::cluster_id() const noexcept {
    return m_cid;
}

xgroup_id_t const &
xtop_sharding_info::group_id() const noexcept {
    return m_gid;
}

xtop_sharding_info::hash_result_type
xtop_sharding_info::hash() const {
    utl::xxh64_t hasher;

    {
        auto const value = m_nid.value();
        hasher.update(&value, sizeof(value));
    }

    {
        auto const value = m_zid.value();
        hasher.update(&value, sizeof(value));
    }

    {
        auto const value = m_cid.value();
        hasher.update(&value, sizeof(value));
    }

    {
        auto const value = m_gid.value();
        hasher.update(&value, sizeof(value));
    }

    return hasher.get_hash();
}

std::string
xtop_sharding_info::to_string() const {
    return top::common::to_string(m_nid) + "/" +
           top::common::to_string(m_zid) + "/" +
           top::common::to_string(m_cid) + "/" +
           top::common::to_string(m_gid);
}

std::int32_t
xtop_sharding_info::do_write(base::xstream_t & stream) const {
    try {
        return stream << codec::msgpack_encode(*this);
    } catch (...) {
        xerror("[sharding info] do_write failed %s", to_string().c_str());
        return -1;
    }
}

std::int32_t
xtop_sharding_info::do_read(base::xstream_t & stream) {
    try {
        xbyte_buffer_t buffer;
        auto const size = stream >> buffer;
        *this = codec::msgpack_decode<xtop_sharding_info>(buffer);
        return size;
    } catch (...) {
        xerror("[sharding info] do_read failed");
        return -1;
    }
}

std::int32_t
operator <<(base::xstream_t & stream, xtop_sharding_info const & o) {
    return o.do_write(stream);
}

std::int32_t
operator >>(base::xstream_t & stream, xtop_sharding_info & o) {
    return o.do_read(stream);
}

NS_END2

NS_BEG1(std)

void
swap(top::common::xtop_sharding_info & lhs, top::common::xtop_sharding_info & rhs) noexcept {
    lhs.swap(rhs);
}

std::size_t
hash<top::common::xtop_sharding_info>::operator()(top::common::xsharding_info_t const & sharding_info) const noexcept {
    return static_cast<std::size_t>(sharding_info.hash());
}

NS_END1
