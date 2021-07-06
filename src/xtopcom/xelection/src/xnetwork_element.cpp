// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xutility.h"
#include "xcommon/xip.h"
#include "xelection/xcache/xnetwork_element.h"
#include "xelection/xdata_accessor_error.h"

#include <cinttypes>
#include <cstdint>

NS_BEG3(top, election, cache)

xtop_network_element::xtop_network_element(common::xnetwork_id_t const & network_id)
    : xbase_t{ network_id } {
}

bool
xtop_network_element::exist(common::xzone_id_t const & zone_id) const {
    XLOCK(m_zone_elements_mutex);
    return exist_with_lock_hold_outside(zone_id);
}

std::shared_ptr<xzone_element_t>
xtop_network_element::zone_element(common::xzone_id_t const & zone_id,
                                   std::error_code & ec) const {
    assert(!ec);
    if (common::broadcast(zone_id)) {
        ec = xdata_accessor_errc_t::zone_id_empty;

        xwarn("%s network %" PRIu32 " looking for an empty zone id %" PRIu32,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint32_t>(zone_id.value()));

        return {};
    }

    XLOCK(m_zone_elements_mutex);
    auto const iterator = find_with_lock_hold_outside(zone_id);
    if (iterator == std::end(m_zone_elements)) {
        ec = xdata_accessor_errc_t::zone_not_exist;

        xwarn("%s network %" PRIu32 " doesn't have zone %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id.value()));

        return {};
    } else {
        return top::get<std::shared_ptr<xzone_element_t>>(*iterator);
    }
}

std::shared_ptr<xzone_element_t>
xtop_network_element::zone_element(common::xzone_id_t const & zone_id) const {
    std::error_code ec;
    auto ret = zone_element(zone_id, ec);
    top::error::throw_error(ec);
    return ret;
}

std::map<common::xzone_id_t, std::shared_ptr<xzone_element_t>>
xtop_network_element::children() const {
    std::error_code ec{xdata_accessor_errc_t::success};
    auto ret = children(ec);
    top::error::throw_error(ec);
    return ret;
}

std::map<common::xzone_id_t, std::shared_ptr<xzone_element_t>> xtop_network_element::children(std::error_code & ec) const {
    assert(!ec);

    XLOCK(m_zone_elements_mutex);
    if (m_zone_elements.empty()) {
        ec = xdata_accessor_errc_t::no_children;

        xwarn("%s network %" PRIu32 " has no child", ec.category().name(), static_cast<std::uint32_t>(network_id().value()));
    }
    return m_zone_elements;
}

std::shared_ptr<xzone_element_t>
xtop_network_element::add_zone_element(common::xzone_id_t const & zone_id,
                                       std::error_code & ec) {
    assert(!ec);
    XLOCK(m_zone_elements_mutex);
    auto const iterator = find_with_lock_hold_outside(zone_id);
    if (iterator != std::end(m_zone_elements)) {
        ec = xdata_accessor_errc_t::zone_already_exist;

        xwarn("%s network %" PRIu32 " already has zone %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id.value()));

        return top::get<std::shared_ptr<xzone_element_t>>(*iterator);
    }

    auto zone_element = std::make_shared<xzone_element_t>(zone_id, shared_from_this());
    m_zone_elements.insert({ zone_id, zone_element });
    return zone_element;
}

std::shared_ptr<xzone_element_t>
xtop_network_element::add_zone_element(common::xzone_id_t const & zone_id) {
    std::error_code ec;
    auto ret = add_zone_element(zone_id, ec);
    top::error::throw_error(ec);
    return ret;
}

bool
xtop_network_element::exist_with_lock_hold_outside(common::xzone_id_t const & zone_id) const {
    return find_with_lock_hold_outside(zone_id) != std::end(m_zone_elements);
}

xtop_network_element::xzone_elements_container_t::const_iterator
xtop_network_element::find_with_lock_hold_outside(common::xzone_id_t const & zone_id) const {
    return m_zone_elements.find(zone_id);
}

NS_END3
