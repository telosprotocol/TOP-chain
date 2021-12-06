// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelection/xcache/xdata_accessor.h"

#include "xbase/xlog.h"
#include "xbasic/xutility.h"
#include "xelection/xcache/xcluster_element.h"
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

xtop_data_accessor::xtop_data_accessor(common::xnetwork_id_t const & network_id, observer_ptr<time::xchain_time_face_t> const & logic_timer)
  : m_network_element{std::make_shared<xnetwork_element_t>(network_id)}, m_logic_timer{logic_timer} {
    assert(m_logic_timer != nullptr);
}

common::xnetwork_id_t xtop_data_accessor::network_id() const noexcept {
    assert(m_network_element);
    return m_network_element->network_id();
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_zone(common::xzone_id_t const & zone_id,
                                                                                                        data::election::xelection_result_store_t const & election_result_store,
                                                                                                        std::uint64_t const associated_blk_height,
                                                                                                        std::error_code & ec) {
    assert(!ec);
    assert(m_network_element);

    if (broadcast(zone_id)) {
        ec = xdata_accessor_errc_t::zone_id_empty;

        xwarn("%s network %" PRIu32 ": update zone election data failed due to empty zone id",
              ec.category().name(),
              static_cast<std::uint32_t>(m_network_element->network_id().value()));

        return {};
    }

    auto zone_element = m_network_element->add_zone_element(zone_id, ec);
    if (ec && ec != xdata_accessor_errc_t::zone_already_exist) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    ec = xdata_accessor_errc_t::success;  // clear xdata_accessor_errc_t::zone_already_exist if any

    auto const zone_type = common::node_type_from(zone_id);
    if (zone_type == common::xnode_type_t::invalid) {
        ec = xdata_accessor_errc_t::invalid_node_type;

        xwarn("%s network %" PRIu32 ": update zone %" PRIu16 " failed due to unknown zone type %" PRIu32,
              ec.category().name(),
              static_cast<std::uint32_t>(m_network_element->network_id().value()),
              static_cast<std::uint16_t>(m_network_element->zone_id().value()),
              static_cast<std::uint32_t>(zone_type));

        assert(false);

        return {};
    }

    switch (zone_type) {
    case common::xnode_type_t::committee:
        return update_committee_zone(zone_element, election_result_store, associated_blk_height, ec);

    case common::xnode_type_t::consensus:
        return update_consensus_zone(zone_element, election_result_store, associated_blk_height, ec);

    case common::xnode_type_t::edge:
        return update_edge_zone(zone_element, election_result_store, associated_blk_height, ec);

    case common::xnode_type_t::storage:
        return update_storage_zone(zone_element, election_result_store, associated_blk_height, ec);

    case common::xnode_type_t::zec:
        return update_zec_zone(zone_element, election_result_store, associated_blk_height, ec);

    case common::xnode_type_t::frozen:
        return update_frozen_zone(zone_element, election_result_store, associated_blk_height, ec);

    default:
        ec = xdata_accessor_errc_t::invalid_node_type;

        xerror("%s network %" PRIu32 " receives zone data with unrecognized zone id %" PRIu16,
               ec.category().name(),
               static_cast<std::uint32_t>(m_network_element->network_id().value()),
               static_cast<std::uint32_t>(zone_id.value()));

        return {};
    }
}

std::map<common::xslot_id_t, data::xnode_info_t> xtop_data_accessor::sharding_nodes(common::xgroup_address_t const & address,
                                                                                    common::xelection_round_t const & election_round,
                                                                                    std::error_code & ec) const {
    xdbg("sharding nodes %s version %s this %p", address.to_string().c_str(), election_round.to_string().c_str(), this);

    assert(!ec);
    assert(m_network_element != nullptr);

    if (address.empty()) {
        ec = xdata_accessor_errc_t::address_empty;

        xwarn("%s address empty", ec.category().name());

        return {};
    }

    auto group_element = this->group_element(address.network_id(), address.zone_id(), address.cluster_id(), address.group_id(), election_round, ec);

    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(group_element != nullptr);

    std::map<common::xslot_id_t, data::xnode_info_t> result;
    auto node_elements_info = group_element->children(ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    assert(!node_elements_info.empty());

    for (auto const & node_element_info : node_elements_info) {
        auto const & slot_id = top::get<common::xslot_id_t const>(node_element_info);
        auto const & node_element = top::get<std::shared_ptr<xnode_element_t>>(node_element_info);
        auto const & election_info = node_element->election_info();

        assert(slot_id == node_element->address().slot_id());

        data::xnode_info_t node_info;
        node_info.election_info = election_info;
        node_info.address = node_element->address();

        result.insert({slot_id, std::move(node_info)});
    }

    return result;
}

std::map<common::xslot_id_t, data::xnode_info_t> xtop_data_accessor::group_nodes(common::xgroup_address_t const & group_address,
                                                                                 common::xlogic_epoch_t const & group_logic_epoch,
                                                                                 std::error_code & ec) const {
    xdbg("group nodes %s logic epoch %s this %p", group_address.to_string().c_str(), group_logic_epoch.to_string().c_str(), this);

    assert(!ec);
    assert(m_network_element != nullptr);

    if (group_address.empty()) {
        ec = xdata_accessor_errc_t::address_empty;

        xwarn("%s address empty", ec.category().name());

        return {};
    }

    auto group_element = this->group_element(group_address.network_id(), group_address.zone_id(), group_address.cluster_id(), group_address.group_id(), group_logic_epoch, ec);

    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(group_element != nullptr);

    std::map<common::xslot_id_t, data::xnode_info_t> result;
    auto node_elements_info = group_element->children(ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    assert(!node_elements_info.empty());

    for (auto const & node_element_info : node_elements_info) {
        auto const & slot_id = top::get<common::xslot_id_t const>(node_element_info);
        auto const & node_element = top::get<std::shared_ptr<xnode_element_t>>(node_element_info);
        auto const & election_info = node_element->election_info();

        assert(slot_id == node_element->address().slot_id());

        data::xnode_info_t node_info;
        node_info.election_info = election_info;
        node_info.address = node_element->address();

        result.insert({slot_id, std::move(node_info)});
    }

    return result;
}

common::xnode_address_t xtop_data_accessor::parent_address(common::xgroup_address_t const & child_address, common::xelection_round_t const & child_version, std::error_code & ec) const
    noexcept {
    assert(!ec);
    auto const parent_element = parent_group_element(child_address, child_version, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    return parent_element->address();
}

common::xnode_address_t xtop_data_accessor::parent_address(common::xgroup_address_t const & child_address,
                                                           common::xlogic_epoch_t const & child_logic_epoch,
                                                           std::error_code & ec) const noexcept {
    assert(!ec);
    auto const parent_element = parent_group_element(child_address, child_logic_epoch, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    return parent_element->address();
}

std::vector<common::xnode_address_t> xtop_data_accessor::child_addresses(common::xgroup_address_t const & parent_group_address,
                                                                         common::xlogic_epoch_t const & parent_logic_epoch,
                                                                         std::error_code & ec) const noexcept {
    assert(!ec);
    auto const parent_element = group_element(parent_group_address, parent_logic_epoch, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    auto const & child_group_elments = parent_element->associated_child_groups(ec);
    if (ec) {
        xwarn("xdata_accessor_t::child_addresses failed; %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    std::vector<common::xnode_address_t> ret;
    std::transform(std::begin(child_group_elments),
                   std::end(child_group_elments),
                   std::back_inserter(ret),
                   [](std::shared_ptr<election::cache::xgroup_element_t> const & child_group) {
                       return child_group->address();
                   });
    return ret;
}

std::shared_ptr<xnode_element_t> xtop_data_accessor::node_element(common::xgroup_address_t const & address,
                                                     common::xlogic_epoch_t const & logic_epoch,
                                                     common::xslot_id_t const & slot_id,
                                                     std::error_code & ec) const {
    assert(!ec);
    auto const group_element = this->group_element(address.network_id(), address.zone_id(), address.cluster_id(), address.group_id(), logic_epoch, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(group_element != nullptr);

    return group_element->node_element(slot_id, ec);
}

common::xaccount_address_t xtop_data_accessor::account_address_from(common::xip2_t const & xip2, std::error_code & ec) const {
    assert(!ec);
    auto group_element = this->group_element_by_height(xip2.network_id(), xip2.zone_id(), xip2.cluster_id(), xip2.group_id(), xip2.height(), ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(group_element != nullptr);

    auto node_element = group_element->node_element(xip2.slot_id(), ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    assert(node_element != nullptr);
    return node_element->node_id();
}

std::shared_ptr<xgroup_element_t> xtop_data_accessor::group_element(common::xgroup_address_t const & group_address,
                                                                    common::xelection_round_t const & version,
                                                                    std::error_code & ec) const {
    assert(!ec);
    assert(m_network_element != nullptr);

    if (group_address.empty()) {
        ec = xdata_accessor_errc_t::address_empty;

        xwarn("%s child address empty", ec.category().name());

        return {};
    }

    xdbg("looking for %s version %s", group_address.to_string().c_str(), version.to_string().c_str());
    return group_element(group_address.network_id(), group_address.zone_id(), group_address.cluster_id(), group_address.group_id(), version, ec);
}

std::shared_ptr<xgroup_element_t> xtop_data_accessor::group_element(common::xgroup_address_t const & group_address,
                                                                    common::xlogic_epoch_t const & logic_epoch,
                                                                    std::error_code & ec) const {
    assert(!ec);
    assert(m_network_element != nullptr);

    if (group_address.empty()) {
        ec = xdata_accessor_errc_t::address_empty;

        xwarn("%s child address empty", ec.category().name());

        return {};
    }

    xdbg("looking for %s version %s", group_address.to_string().c_str(), logic_epoch.to_string().c_str());
    return group_element(group_address.network_id(), group_address.zone_id(), group_address.cluster_id(), group_address.group_id(), logic_epoch, ec);
}

std::shared_ptr<xgroup_element_t> xtop_data_accessor::group_element_by_height(common::xgroup_address_t const & group_address,
                                                                              uint64_t const election_blk_height,
                                                                              std::error_code & ec) const {
    assert(!ec);
    assert(m_network_element != nullptr);

    if (group_address.empty()) {
        ec = xdata_accessor_errc_t::address_empty;

        xwarn("%s child address empty", ec.category().name());

        return {};
    }

    xdbg("looking for %s", group_address.to_string().c_str());
    return group_element_by_height(group_address.network_id(), group_address.zone_id(), group_address.cluster_id(), group_address.group_id(), election_blk_height, ec);
}

std::shared_ptr<xgroup_element_t> xtop_data_accessor::group_element_by_logic_time(common::xgroup_address_t const & group_address,
                                                                                  common::xlogic_time_t const logic_time,
                                                                                  std::error_code & ec) const {
    assert(!ec);
    assert(m_network_element != nullptr);

    if (group_address.empty()) {
        ec = xdata_accessor_errc_t::address_empty;

        xwarn("%s child address empty", ec.category().name());

        return {};
    }

    return group_element_by_logic_time(group_address.network_id(), group_address.zone_id(), group_address.cluster_id(), group_address.group_id(), logic_time, ec);
}

std::shared_ptr<xgroup_element_t> xtop_data_accessor::parent_group_element(common::xgroup_address_t const & child_sharding_address,
                                                                           common::xelection_round_t const & child_sharding_version,
                                                                           std::error_code & ec) const {
    assert(!ec);
    auto group_element = this->group_element(child_sharding_address, child_sharding_version, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(group_element != nullptr);

    auto associated_parent = group_element->associated_parent_group(ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(associated_parent != nullptr);
    return associated_parent;
}

std::shared_ptr<xgroup_element_t> xtop_data_accessor::parent_group_element(common::xgroup_address_t const & child_gropu_address,
                                                                           common::xlogic_epoch_t const & child_logic_epoch,
                                                                           std::error_code & ec) const {
    assert(!ec);
    auto const group_element = child_logic_epoch.empty() ? group_element_by_logic_time(child_gropu_address, m_logic_timer->logic_time(), ec)
                                                         : group_element_by_height(child_gropu_address, child_logic_epoch.associated_blk_height(), ec);

    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(group_element != nullptr);

    auto associated_parent = group_element->associated_parent_group(ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(associated_parent != nullptr);
    return associated_parent;
}

common::xelection_round_t xtop_data_accessor::election_epoch_from(common::xip2_t const & xip2, std::error_code & ec) const {
    assert(!ec);
    auto group_element = this->group_element_by_height(xip2.network_id(), xip2.zone_id(), xip2.cluster_id(), xip2.group_id(), xip2.height(), ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    assert(group_element != nullptr);

    return group_element->election_round();
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                        data::election::xelection_result_store_t const & election_result_store,
                                                                                                        std::uint64_t const associated_blk_height,
                                                                                                        std::error_code & ec) {
    assert(!ec);
    std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> ret;

    auto const zone_type = common::node_type_from(zone_element->zone_id());

    try {
        if (!election_result_store.size()) {
            ec = xdata_accessor_errc_t::election_data_empty;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 " empty xelection_result_store_t",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()));
            return {};
        }

        xdbg("%s network %" PRIu32 " zone %" PRIu16 ": zone type %s",
             ec.category().name(),
             static_cast<std::uint32_t>(m_network_element->network_id().value()),
             static_cast<std::uint16_t>(zone_element->zone_id().value()),
             common::to_string(zone_type).c_str());

        auto const & zone_result = election_result_store.result_of(m_network_element->network_id()).result_of(zone_type);

        for (auto const & zone_data : zone_result.results()) {
            std::error_code ec1;

            auto const & cluster_id = top::get<common::xcluster_id_t const>(zone_data);
            auto const & cluster_result = top::get<data::election::xelection_cluster_result_t>(zone_data);

            auto const & cluster_element = zone_element->add_cluster_element(cluster_id, ec1);
            if (ec1 && ec1 != xdata_accessor_errc_t::cluster_already_exist) {
                ec = xdata_accessor_errc_t::election_data_partially_updated;

                xwarn("%s network %" PRIu32 " zone %" PRIu16 ": failed to update election data at cluster %" PRIu16,
                      ec.category().name(),
                      static_cast<std::uint32_t>(m_network_element->network_id().value()),
                      static_cast<std::uint16_t>(zone_element->zone_id().value()),
                      static_cast<std::uint16_t>(cluster_id.value()));

                continue;
            }

            auto r = update_cluster(zone_element, cluster_element, cluster_result, associated_blk_height, ec);
            ret.insert(std::begin(r), std::end(r));
        }
    } catch (std::out_of_range const &) {
        ec = xdata_accessor_errc_t::election_data_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 ": updating election data failed due to empty input",
              ec.category().name(),
              static_cast<std::uint32_t>(m_network_element->network_id().value()),
              static_cast<std::uint16_t>(zone_element->zone_id().value()));
    } catch (std::exception const & eh) {
        ec = xdata_accessor_errc_t::unknown_std_exception;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 ": update election data failed due to std::exception: %s",
              ec.category().name(),
              static_cast<std::uint32_t>(m_network_element->network_id().value()),
              static_cast<std::uint16_t>(zone_element->zone_id().value()),
              eh.what());
    } catch (...) {
        ec = xdata_accessor_errc_t::unknown_error;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 ": update election data failed due to unknown error",
              ec.category().name(),
              static_cast<std::uint32_t>(m_network_element->network_id().value()),
              static_cast<std::uint16_t>(zone_element->zone_id().value()));
    }

    return ret;
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_committee_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                                  data::election::xelection_result_store_t const & election_result_store,
                                                                                                                  std::uint64_t const associated_blk_height,
                                                                                                                  std::error_code & ec) {
    assert(!ec);
    return update_zone(zone_element, election_result_store, associated_blk_height, ec);
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_consensus_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                                  data::election::xelection_result_store_t const & election_result_store,
                                                                                                                  std::uint64_t const associated_blk_height,
                                                                                                                  std::error_code & ec) {
    assert(!ec);
    std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> ret;

    assert(common::node_type_from(zone_element->zone_id()) == common::xnode_type_t::consensus);
    std::array<common::xnode_type_t, 2> node_types{ {common::xnode_type_t::consensus_auditor, common::xnode_type_t::consensus_validator} };

    if (election_result_store.empty()) {
        ec = xdata_accessor_errc_t::election_data_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " empty xelection_result_store_t",
              ec.category().name(),
              static_cast<std::uint32_t>(m_network_element->network_id().value()),
              static_cast<std::uint16_t>(zone_element->zone_id().value()));

        return {};
    }

    for (auto node_type : node_types) {
        try {
            auto const & zone_result = election_result_store.result_of(m_network_element->network_id()).result_of(node_type);

            for (auto const & zone_data : zone_result.results()) {
                std::error_code ec1{xdata_accessor_errc_t::success};

                auto const & cluster_id = top::get<common::xcluster_id_t const>(zone_data);
                auto const & cluster_result = top::get<data::election::xelection_cluster_result_t>(zone_data);

                auto const & cluster_element = zone_element->add_cluster_element(cluster_id, ec1);
                if (ec1 && ec1 != xdata_accessor_errc_t::cluster_already_exist) {
                    ec = xdata_accessor_errc_t::election_data_partially_updated;

                    xwarn("%s network %" PRIu32 " zone %" PRIu16 ": failed to update election data at cluster %" PRIu16,
                          ec.category().name(),
                          static_cast<std::uint32_t>(m_network_element->network_id().value()),
                          static_cast<std::uint16_t>(zone_element->zone_id().value()),
                          static_cast<std::uint16_t>(cluster_id.value()));

                    continue;
                }
                ec1 = xdata_accessor_errc_t::success;  // clear xdata_accessor_errc_t::cluster_already_exist if any

                auto r = update_cluster(zone_element, cluster_element, cluster_result, associated_blk_height, ec1);
                // should skip checking ec1?
                ret.insert(std::begin(r), std::end(r));
            }
        } catch (std::out_of_range const &) {
            ec = xdata_accessor_errc_t::election_data_empty;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 ": updating election data failed due to empty input",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()));

        } catch (std::exception const & eh) {
            ec = xdata_accessor_errc_t::unknown_std_exception;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 ": update election data failed due to std::exception: %s",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()),
                  eh.what());

        } catch (...) {
            ec = xdata_accessor_errc_t::unknown_error;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 ": update election data failed due to unknown error",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()));
        }
    }

    return ret;
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_edge_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                             data::election::xelection_result_store_t const & election_result_store,
                                                                                                             std::uint64_t const associated_blk_height,
                                                                                                             std::error_code & ec) {
    assert(!ec);
    return update_zone(zone_element, election_result_store, associated_blk_height, ec);
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_storage_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                                data::election::xelection_result_store_t const & election_result_store,
                                                                                                                std::uint64_t const associated_blk_height,
                                                                                                                std::error_code & ec) {
    assert(!ec);
    std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> ret;

    assert(common::node_type_from(zone_element->zone_id()) == common::xnode_type_t::storage);
    std::array<common::xnode_type_t, 2> node_types{ {common::xnode_type_t::storage_archive, common::xnode_type_t::storage_exchange} };

    if (election_result_store.empty()) {
        ec = xdata_accessor_errc_t::election_data_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " empty xelection_result_store_t",
              ec.category().name(),
              static_cast<std::uint32_t>(m_network_element->network_id().value()),
              static_cast<std::uint16_t>(zone_element->zone_id().value()));

        return {};
    }

    for (auto node_type : node_types) {
        try {
            auto const & zone_result = election_result_store.result_of(m_network_element->network_id()).result_of(node_type);

            for (auto const & zone_data : zone_result.results()) {
                std::error_code ec1{ xdata_accessor_errc_t::success };

                auto const & cluster_id = top::get<common::xcluster_id_t const>(zone_data);
                auto const & cluster_result = top::get<data::election::xelection_cluster_result_t>(zone_data);

                auto const & cluster_element = zone_element->add_cluster_element(cluster_id, ec1);
                if (ec1 && ec1 != xdata_accessor_errc_t::cluster_already_exist) {
                    ec = xdata_accessor_errc_t::election_data_partially_updated;

                    xwarn("%s network %" PRIu32 " zone %" PRIu16 ": failed to update election data at cluster %" PRIu16,
                          ec.category().name(),
                          static_cast<std::uint32_t>(m_network_element->network_id().value()),
                          static_cast<std::uint16_t>(zone_element->zone_id().value()),
                          static_cast<std::uint16_t>(cluster_id.value()));

                    continue;
                }

                ec1 = xdata_accessor_errc_t::success;  // clear xdata_accessor_errc_t::cluster_already_exist if any
                auto r = update_cluster(zone_element, cluster_element, cluster_result, associated_blk_height, ec1);
                // should skip checking ec1?
                ret.insert(std::begin(r), std::end(r));
            }
        } catch (std::out_of_range const &) {
            ec = xdata_accessor_errc_t::election_data_empty;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 ": updating election data failed due to empty input",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()));

        } catch (std::exception const & eh) {
            ec = xdata_accessor_errc_t::unknown_std_exception;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 ": update election data failed due to std::exception: %s",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()),
                  eh.what());

        } catch (...) {
            ec = xdata_accessor_errc_t::unknown_error;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 ": update election data failed due to unknown error",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()));
        }

        // special case for exchange, since the result can be empty.
        if (node_type == common::xnode_type_t::storage_exchange && ec == xdata_accessor_errc_t::election_data_empty) {
            ec = xdata_accessor_errc_t::success;
        }
    }

    return ret;
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_zec_zone(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                            data::election::xelection_result_store_t const & election_result_store,
                                                                                                            std::uint64_t const associated_blk_height,
                                                                                                            std::error_code & ec) {
    assert(!ec);
    return update_zone(zone_element, election_result_store, associated_blk_height, ec);
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_frozen_zone(
    std::shared_ptr<xzone_element_t> const & zone_element,
    data::election::xelection_result_store_t const & election_result_store,
    std::uint64_t const associated_blk_height,
    std::error_code & ec) {
    assert(!ec);
    return update_zone(zone_element, election_result_store, associated_blk_height, ec);
}

std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> xtop_data_accessor::update_cluster(std::shared_ptr<xzone_element_t> const & zone_element,
                                                                                                           std::shared_ptr<xcluster_element_t> const & cluster_element,
                                                                                                           data::election::xelection_cluster_result_t const & cluster_result,
                                                                                                           std::uint64_t const associated_blk_height,
                                                                                                           std::error_code & ec) {
    assert(!ec);
    std::unordered_map<common::xgroup_address_t, xgroup_update_result_t> ret;
    std::error_code ec1{xdata_accessor_errc_t::success};

    for (auto const & group_data : cluster_result.results()) {
        ec1 = xdata_accessor_errc_t::success;

        auto const & group_id = top::get<common::xgroup_id_t const>(group_data);
        auto const & group_result = top::get<data::election::xelection_group_result_t>(group_data);

        auto group_update_result = cluster_element->add_group_element(
            group_id, group_result.group_version(), group_result.timestamp(), group_result.start_time(), group_result.size(), associated_blk_height, ec1);
        if (ec1) {
            ec = xdata_accessor_errc_t::election_data_partially_updated;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": failed to update election data at group %" PRIu16,
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()),
                  static_cast<std::uint16_t>(cluster_element->cluster_id().value()),
                  static_cast<std::uint16_t>(group_id.value()));

            continue;
        }

        ret.insert({common::xgroup_address_t{m_network_element->network_id(), zone_element->zone_id(), cluster_element->cluster_id(), group_id}, group_update_result});

        auto const & group_element = group_update_result.added;
        update_group(zone_element, cluster_element, group_element, group_result, ec1);
    }

    for (auto const & group_data : cluster_result.results()) {
        ec1 = xdata_accessor_errc_t::success;

        auto const & group_id = top::get<common::xgroup_id_t const>(group_data);
        auto const & group_result = top::get<data::election::xelection_group_result_t>(group_data);
        auto const & associated_group_id = group_result.associated_group_id();
        auto const & associated_group_version = group_result.associated_group_version();

        if (broadcast(associated_group_id) || associated_group_version.empty()) {
            continue;
        }

        auto const & parent_group_element = cluster_element->group_element(associated_group_id, associated_group_version, ec1);
        if (ec1) {
            ec = xdata_accessor_errc_t::group_association_failed;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " fail to find associated group %" PRIu16 " at version %" PRIu64 " error string %s",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()),
                  static_cast<std::uint16_t>(cluster_element->cluster_id().value()),
                  static_cast<std::uint16_t>(group_id.value()),
                  static_cast<std::uint16_t>(associated_group_id.value()),
                  static_cast<std::uint64_t>(associated_group_version.value()),
                  ec1.message().c_str());

            continue;
        }

        auto const & child_group_element = cluster_element->group_element(group_id, group_result.group_version(), ec1);
        if (ec1) {
            ec = xdata_accessor_errc_t::group_association_failed;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " fail to find associated group %" PRIu16 " at version %" PRIu64 " error string %s",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()),
                  static_cast<std::uint16_t>(cluster_element->cluster_id().value()),
                  static_cast<std::uint16_t>(group_id.value()),
                  static_cast<std::uint16_t>(associated_group_id.value()),
                  static_cast<std::uint64_t>(associated_group_version.value()),
                  ec1.message().c_str());

            continue;
        }

        child_group_element->associate_parent_group(parent_group_element, ec1);
        if (ec1 && ec1 != xdata_accessor_errc_t::associate_parent_group_twice && ec1 != xdata_accessor_errc_t::associate_child_group_twice) {
            ec = xdata_accessor_errc_t::group_association_failed;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " fail to find associated group %" PRIu16 " at version %" PRIu64 " error string %s",
                  ec.category().name(),
                  static_cast<std::uint32_t>(m_network_element->network_id().value()),
                  static_cast<std::uint16_t>(zone_element->zone_id().value()),
                  static_cast<std::uint16_t>(cluster_element->cluster_id().value()),
                  static_cast<std::uint16_t>(group_id.value()),
                  static_cast<std::uint16_t>(associated_group_id.value()),
                  static_cast<std::uint64_t>(associated_group_version.value()),
                  ec1.message().c_str());

            continue;
        }
    }

    return ret;
}

void xtop_data_accessor::update_group(std::shared_ptr<xzone_element_t> const & /*zone_element*/,
                                      std::shared_ptr<xcluster_element_t> const & /*cluster_element*/,
                                      std::shared_ptr<xgroup_element_t> const & group_element,
                                      data::election::xelection_group_result_t const & group_result,
                                      XATTRIBUTE_MAYBE_UNUSED std::error_code & ec) {
    assert(!ec);
    group_element->set_node_elements(group_result.results());
}

NS_END3
