// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xelection/xcache/xbasic_element.h"
#include "xelection/xcache/xelement_fwd.h"

#include <map>
#include <memory>
#include <mutex>

NS_BEG3(top, election, cache)

class xtop_zone_element final : public xbasic_element_t
                              , public std::enable_shared_from_this<xtop_zone_element> {
private:
    using xbase_t = xbasic_element_t;
    using xcluster_elements_container_t = std::map<common::xcluster_id_t, std::shared_ptr<xcluster_element_t>>;

    std::weak_ptr<xnetwork_element_t> m_network_element{};

    mutable std::mutex m_cluster_elements_mutex{};
    xcluster_elements_container_t m_cluster_elements{};

public:
    xtop_zone_element(xtop_zone_element const &)             = delete;
    xtop_zone_element & operator=(xtop_zone_element const &) = delete;
    xtop_zone_element(xtop_zone_element &&)                  = default;
    xtop_zone_element & operator=(xtop_zone_element &&)      = default;
    ~xtop_zone_element() override                            = default;

    xtop_zone_element(common::xzone_id_t const & zone_id,
                      std::shared_ptr<xnetwork_element_t> const & network_element);

    std::shared_ptr<xnetwork_element_t>
    network_element() const noexcept;

    bool
    exist(common::xcluster_id_t const & cluster_id) const;

    std::shared_ptr<xcluster_element_t>
    cluster_element(common::xcluster_id_t const & cluster_id, std::error_code & ec) const;

    std::shared_ptr<xcluster_element_t>
    cluster_element(common::xcluster_id_t const & cluster_id) const;

    std::shared_ptr<xcluster_element_t>
    add_cluster_element(common::xcluster_id_t const & cluster_id, std::error_code & ec);

    std::shared_ptr<xcluster_element_t>
    add_cluster_element(common::xcluster_id_t const & cluster_id);

    std::map<common::xcluster_id_t, std::shared_ptr<xcluster_element_t>>
    children() const;

    std::map<common::xcluster_id_t, std::shared_ptr<xcluster_element_t>> children(std::error_code & ec) const;

private:
    bool
    exist_with_lock_hold_outside(common::xcluster_id_t const & cluster_id) const;

    xcluster_elements_container_t::const_iterator
    find_with_lock_hold_outside(common::xcluster_id_t const & cluster_id) const;
};
using xzone_element_t = xtop_zone_element;

NS_END3
