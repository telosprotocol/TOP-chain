// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbasic/xutility.h"
#include "xelection/xcache/xcluster_element.h"
#include "xelection/xcache/xdata_accessor.h"
#include "xelection/xcache/xgroup_element.h"
#include "xelection/xcache/xnode_element.h"
#include "xelection/xdata_accessor_error.h"

#include <array>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

NS_BEG3(top, election, cache)

std::shared_ptr<xgroup_element_t>
xtop_data_accessor::group_element(common::xnetwork_id_t const & network_id,
                                  common::xzone_id_t const & zone_id,
                                  common::xcluster_id_t const & cluster_id,
                                  common::xgroup_id_t const & group_id,
                                  common::xversion_t const & version,
                                  std::error_code & ec) const {
    assert(!ec);
    assert(m_logic_timer != nullptr);

    auto cluster_element = this->cluster_element(network_id, zone_id, cluster_id, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    if (version.empty()) {
        return cluster_element->group_element_by_logic_time(group_id, m_logic_timer->logic_time(), ec);
    } else {
        return cluster_element->group_element(group_id, version, ec);
    }
}

std::shared_ptr<xgroup_element_t>
xtop_data_accessor::group_element_by_height(common::xnetwork_id_t const & network_id,
                                            common::xzone_id_t const & zone_id,
                                            common::xcluster_id_t const & cluster_id,
                                            common::xgroup_id_t const & group_id,
                                            uint64_t const  election_blk_height,
                                            std::error_code & ec) const {
    assert(!ec);
    assert(m_logic_timer != nullptr);

    auto cluster_element = this->cluster_element(network_id, zone_id, cluster_id, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    return cluster_element->group_element_by_height(group_id, election_blk_height, ec);
}

std::shared_ptr<xgroup_element_t>
xtop_data_accessor::group_element_by_logic_time(common::xnetwork_id_t const & network_id,
                                                common::xzone_id_t const & zone_id,
                                                common::xcluster_id_t const & cluster_id,
                                                common::xgroup_id_t const & group_id,
                                                common::xlogic_time_t const logic_time,
                                                std::error_code & ec) const {
    assert(!ec);
    assert(m_logic_timer != nullptr);

    auto cluster_element = this->cluster_element(network_id, zone_id, cluster_id, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    return cluster_element->group_element_by_logic_time(group_id, logic_time, ec);

}

std::shared_ptr<xcluster_element_t>
xtop_data_accessor::cluster_element(common::xnetwork_id_t const & network_id,
                                    common::xzone_id_t const & zone_id,
                                    common::xcluster_id_t const & cluster_id,
                                    std::error_code & ec) const {
    assert(!ec);
    assert(m_network_element != nullptr);

    if (network_id != m_network_element->network_id()) {
        ec = xdata_accessor_errc_t::network_id_mismatch;

        xwarn("%s network %" PRIu32 ": looking for network %s", ec.category().name(), static_cast<std::uint32_t>(m_network_element->network_id().value()), network_id.to_string().c_str());

        return {};
    }

    auto const zone_element = m_network_element->zone_element(zone_id, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(zone_element != nullptr);

    auto const cluster_element = zone_element->cluster_element(cluster_id, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(cluster_element);

    return cluster_element;
}


NS_END3
