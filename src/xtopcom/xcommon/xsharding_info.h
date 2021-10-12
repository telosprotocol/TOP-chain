// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbasic/xhashable.hpp"
#include "xbasic/xid.hpp"
#include "xcommon/xip.h"

#include <functional>
#include <limits>
#include <type_traits>

NS_BEG2(top, common)

/**
 * @brief The sharding information.  We divide the network by sharding, thus each network
 *        is divided into several zones and each zone is divided into several clusters.
 */
class xtop_sharding_info final
  : public xhashable_t<xtop_sharding_info> {
private:
    xnetwork_id_t m_nid{};
    xzone_id_t m_zid{};
    xcluster_id_t m_cid{};
    xgroup_id_t m_gid{};

public:
    using hash_result_type = xhashable_t<xtop_sharding_info>::hash_result_type;

    xtop_sharding_info() = default;
    xtop_sharding_info(xtop_sharding_info const &) = default;
    xtop_sharding_info & operator=(xtop_sharding_info const &) = default;
    xtop_sharding_info(xtop_sharding_info &&) = default;
    xtop_sharding_info & operator=(xtop_sharding_info &&) = default;
    ~xtop_sharding_info() override = default;

    explicit xtop_sharding_info(xip_t const & xip) noexcept;

    explicit xtop_sharding_info(xnetwork_id_t const & nid) noexcept;

    xtop_sharding_info(xnetwork_id_t const & nid, xzone_id_t const & zid) noexcept;

    xtop_sharding_info(xnetwork_id_t const & nid, xzone_id_t const & zid, xcluster_id_t const & cid) noexcept;

    xtop_sharding_info(xnetwork_id_t const & nid, xzone_id_t const & zid, xcluster_id_t const & cid, xgroup_id_t const & gid) noexcept;

    bool operator==(xtop_sharding_info const & other) const noexcept;

    bool operator!=(xtop_sharding_info const & other) const noexcept;

    bool operator<(xtop_sharding_info const & other) const noexcept;

    bool operator>(xtop_sharding_info const & other) const noexcept;

    bool operator<=(xtop_sharding_info const & other) const noexcept;

    bool operator>=(xtop_sharding_info const & other) const noexcept;

    void swap(xtop_sharding_info & other) noexcept;

    // bool empty() const noexcept;

    xnetwork_id_t const & network_id() const noexcept;

    xzone_id_t const & zone_id() const noexcept;

    xcluster_id_t const & cluster_id() const noexcept;

    xgroup_id_t const & group_id() const noexcept;

    hash_result_type hash() const override;

    std::string to_string() const;

    friend std::int32_t operator<<(base::xstream_t & stream, xtop_sharding_info const & o);

    friend std::int32_t operator>>(base::xstream_t & stream, xtop_sharding_info & o);

private:
    std::int32_t do_write(base::xstream_t & stream) const;

    std::int32_t do_read(base::xstream_t & stream);
};
using xsharding_info_t = xtop_sharding_info;

std::int32_t operator<<(base::xstream_t & stream, xtop_sharding_info const & o);

std::int32_t operator>>(base::xstream_t & stream, xtop_sharding_info & o);

NS_END2

NS_BEG1(std)

void swap(top::common::xtop_sharding_info & lhs, top::common::xtop_sharding_info & rhs) noexcept;

template <>
struct hash<top::common::xsharding_info_t> final {
    std::size_t operator()(top::common::xsharding_info_t const & sharding_info) const noexcept;
};

NS_END1
