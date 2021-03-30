// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xelection/xcache/xbasic_element.h"
#include "xelection/xcache/xzone_element.h"

#include <memory>
#include <mutex>
#include <map>

NS_BEG3(top, election, cache)

class xtop_network_element final : public xbasic_element_t
                                 , public std::enable_shared_from_this<xtop_network_element> {
private:
    using xbase_t = xbasic_element_t;
    using xzone_elements_container_t = std::map<common::xzone_id_t, std::shared_ptr<xzone_element_t>>;
    mutable std::mutex m_zone_elements_mutex{};
    xzone_elements_container_t m_zone_elements{};

public:
    xtop_network_element(xtop_network_element const &)             = delete;
    xtop_network_element & operator=(xtop_network_element const &) = delete;
    xtop_network_element(xtop_network_element &&)                  = default;
    xtop_network_element & operator=(xtop_network_element &&)      = default;
    ~xtop_network_element() override                               = default;

    explicit
    xtop_network_element(common::xnetwork_id_t const & network_id);

    bool
    exist(common::xzone_id_t const & zone_id) const;

    std::shared_ptr<xzone_element_t>
    zone_element(common::xzone_id_t const & zone_id, std::error_code & ec) const;

    std::shared_ptr<xzone_element_t>
    zone_element(common::xzone_id_t const & zone_id) const;

    std::shared_ptr<xzone_element_t>
    add_zone_element(common::xzone_id_t const & zone_id, std::error_code & ec);

    std::shared_ptr<xzone_element_t>
    add_zone_element(common::xzone_id_t const & zone_id);

    std::map<common::xzone_id_t, std::shared_ptr<xzone_element_t>>
    children() const;

    std::map<common::xzone_id_t, std::shared_ptr<xzone_element_t>> children(std::error_code & ec) const;

private:
    bool
    exist_with_lock_hold_outside(common::xzone_id_t const & zone_id) const;

    xzone_elements_container_t::const_iterator
    find_with_lock_hold_outside(common::xzone_id_t const & zone_id) const;
};
using xnetwork_element_t = xtop_network_element;

NS_END3
