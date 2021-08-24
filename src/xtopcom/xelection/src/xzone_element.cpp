// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xutility.h"
#include "xcommon/xip.h"
#include "xelection/xcache/xcluster_element.h"
#include "xelection/xcache/xnetwork_element.h"
#include "xelection/xcache/xzone_element.h"
#include "xelection/xdata_accessor_error.h"

#include <cinttypes>
#include <cstdint>

NS_BEG3(top, election, cache)

xtop_zone_element::xtop_zone_element(common::xzone_id_t const & zone_id,
                                     std::shared_ptr<xnetwork_element_t> const & network_element)
    : xbase_t{
        network_element->network_id(),
        zone_id
    }, m_network_element{ network_element } {
}

std::shared_ptr<xnetwork_element_t>
xtop_zone_element::network_element() const noexcept {
    return m_network_element.lock();
}

bool
xtop_zone_element::exist(common::xcluster_id_t const & cluster_id) const {
    XLOCK(m_cluster_elements_mutex);
    return exist_with_lock_hold_outside(cluster_id);
}

std::shared_ptr<xcluster_element_t>
xtop_zone_element::cluster_element(common::xcluster_id_t const & cluster_id,
                                   std::error_code & ec) const {
    assert(!ec);
    if (common::broadcast(cluster_id)) {
        ec = xdata_accessor_errc_t::cluster_id_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " looking for an empty cluster id",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()));

        return {};
    }

    XLOCK(m_cluster_elements_mutex);
    auto const iterator = find_with_lock_hold_outside(cluster_id);
    if (iterator == std::end(m_cluster_elements)) {
        ec = xdata_accessor_errc_t::cluster_not_exist;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " doesn't have cluster %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id.value()));

        return {};
    }

    return top::get<std::shared_ptr<xcluster_element_t>>(*iterator);
}

std::shared_ptr<xcluster_element_t>
xtop_zone_element::cluster_element(common::xcluster_id_t const & cluster_id) const {
    std::error_code ec;
    auto ret = cluster_element(cluster_id, ec);
    top::error::throw_error(ec);
    return ret;
}

std::shared_ptr<xcluster_element_t>
xtop_zone_element::add_cluster_element(common::xcluster_id_t const & cluster_id,
                                       std::error_code & ec) {
    assert(!ec);
    XLOCK(m_cluster_elements_mutex);
    auto const iterator = find_with_lock_hold_outside(cluster_id);
    if (iterator != std::end(m_cluster_elements)) {
        ec = xdata_accessor_errc_t::cluster_already_exist;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " already has cluster %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id.value()));

        return top::get<std::shared_ptr<xcluster_element_t>>(*iterator);
    }

    auto cluster_element = std::make_shared<xcluster_element_t>(cluster_id, shared_from_this());
    m_cluster_elements.insert({ cluster_id, cluster_element });
    return cluster_element;
}

std::shared_ptr<xcluster_element_t>
xtop_zone_element::add_cluster_element(common::xcluster_id_t const & cluster_id) {
    std::error_code ec;
    auto ret = add_cluster_element(cluster_id, ec);
    top::error::throw_error(ec);
    return ret;
}

std::map<common::xcluster_id_t, std::shared_ptr<xcluster_element_t>>
xtop_zone_element::children() const {
    std::error_code ec{xdata_accessor_errc_t::success};
    auto ret = children(ec);
    top::error::throw_error(ec);
    return ret;
}

std::map<common::xcluster_id_t, std::shared_ptr<xcluster_element_t>> xtop_zone_element::children(std::error_code & ec) const {
    assert(!ec);
    XLOCK(m_cluster_elements_mutex);
    if (m_cluster_elements.empty()) {
        ec = xdata_accessor_errc_t::no_children;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " has no child",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()));
    }
    return m_cluster_elements;
}

bool
xtop_zone_element::exist_with_lock_hold_outside(common::xcluster_id_t const & cluster_id) const {
    return find_with_lock_hold_outside(cluster_id) != std::end(m_cluster_elements);
}

xtop_zone_element::xcluster_elements_container_t::const_iterator
xtop_zone_element::find_with_lock_hold_outside(common::xcluster_id_t const & cluster_id) const {
    return m_cluster_elements.find(cluster_id);
}

NS_END3
