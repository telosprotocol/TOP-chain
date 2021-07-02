// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelection/xcache/xgroup_element.h"

#include "xbasic/xerror/xchain_error.h"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xutility.h"
#include "xelection/xcache/xcluster_element.h"
#include "xelection/xdata_accessor_error.h"

#include <cinttypes>

NS_BEG3(top, election, cache)

xtop_group_element::xtop_group_element(common::xversion_t const & version,
                                       common::xgroup_id_t const & group_id,
                                       std::uint16_t const sharding_size,
                                       std::uint64_t const associated_election_blk_height,
                                       std::shared_ptr<xcluster_element_t> const & cluster_element)
  : xbase_t{version, cluster_element->network_id(), cluster_element->zone_id(), cluster_element->cluster_id(), group_id, sharding_size, associated_election_blk_height}
  , m_cluster_element{cluster_element} {}

std::shared_ptr<xcluster_element_t> xtop_group_element::cluster_element() const noexcept {
    return m_cluster_element.lock();
}

bool xtop_group_element::exist(common::xslot_id_t const & slot_id) const {
    XLOCK(m_node_elements_mutex);
    return exist_with_lock_hold_outside(slot_id);
}

std::shared_ptr<xnode_element_t> xtop_group_element::node_element(common::xslot_id_t const & slot_id, std::error_code & ec) const {
    assert(!ec);
    if (broadcast(slot_id)) {
        ec = xdata_accessor_errc_t::slot_id_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": looking for an empty slot id",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()));

        return {};
    }

    assert(!broadcast(slot_id));

    XLOCK(m_node_elements_mutex);
    auto const it = m_node_elements.find(slot_id);
    if (it == std::end(m_node_elements)) {
        ec = xdata_accessor_errc_t::slot_not_exist;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": doesn't have slot %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()),
              static_cast<std::uint16_t>(slot_id.value()));

        return {};
    }

    return top::get<std::shared_ptr<xnode_element_t>>(*it);
}

std::shared_ptr<xnode_element_t> xtop_group_element::node_element(common::xslot_id_t const & slot_id) const {
    std::error_code ec;
    auto ret = node_element(slot_id, ec);
    top::error::throw_error(ec);

    return ret;
}

std::shared_ptr<xnode_element_t> xtop_group_element::node_element(common::xnode_id_t const & node_id, std::error_code & ec) const {
    assert(!ec);
    if (node_id.empty()) {
        ec = xdata_accessor_errc_t::node_id_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": looking for an empty node id",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()));

        return {};
    }

    XLOCK_GUARD(m_node_elements_mutex) {
        for (auto const & node_info : m_node_elements) {
            auto const & node_element = top::get<std::shared_ptr<xnode_element_t>>(node_info);
            if (node_element->node_id() == node_id) {
                return node_element;
            }
        }
    }

    ec = xdata_accessor_errc_t::node_not_found;

    xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": doesn't have node %s",
          ec.category().name(),
          static_cast<std::uint32_t>(network_id().value()),
          static_cast<std::uint16_t>(zone_id().value()),
          static_cast<std::uint16_t>(cluster_id().value()),
          static_cast<std::uint16_t>(group_id().value()),
          node_id.value().c_str());

    return {};
}

std::shared_ptr<xnode_element_t> xtop_group_element::node_element(common::xnode_id_t const & node_id) const {
    std::error_code ec;
    auto ret = node_element(node_id, ec);
    top::error::throw_error(ec);
    return ret;
}

bool xtop_group_element::contains(common::xnode_id_t const & node_id) const noexcept {
    try {
        std::error_code ec;
        return node_element(node_id, ec) != nullptr;
    } catch (error::xtop_error_t const & eh) {
        xwarn("xtop_group_element::contains %s", eh.what());
    } catch (std::exception const & eh) {
        xwarn("xtop_group_element::contains %s", eh.what());
    } catch (...) {
        xerror("xtop_group_element::contains unknown");
    }

    return false;
}

void xtop_group_element::set_node_elements(std::map<common::xslot_id_t, data::election::xelection_info_bundle_t> const & election_data) {
    XLOCK(m_node_elements_mutex);
    for (auto const & node_data : election_data) {
        auto const & slot_id = top::get<common::xslot_id_t const>(node_data);
        auto const & election_info_bundle = top::get<data::election::xelection_info_bundle_t>(node_data);
        auto const & election_info = election_info_bundle.election_info();
        auto const & node_id = election_info_bundle.node_id();

        if (common::broadcast(slot_id) || election_info_bundle.empty()) {
            continue;
        }

        xdbg("adding %s %s", node_id.c_str(), common::to_string(node_type_from(zone_id())).c_str());
        m_node_elements.insert({slot_id, std::make_shared<xnode_element_t>(node_id, slot_id, election_info, shared_from_this())});
    }
    assert(sharding_size() == m_node_elements.size());
}

void xtop_group_element::associate_parent_group(std::shared_ptr<xtop_group_element> const & parent_group, std::error_code & ec) {
    assert(!ec);
    if (network_id() != parent_group->network_id()) {
        ec = xdata_accessor_errc_t::network_id_mismatch;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": trying to associate parent group from network %" PRIu32,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()),
              static_cast<std::uint32_t>(parent_group->network_id().value()));

        return;
    }

    if (zone_id() != parent_group->zone_id()) {
        ec = xdata_accessor_errc_t::zone_id_mismatch;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": trying to associate parent group from zone %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()),
              static_cast<std::uint16_t>(parent_group->zone_id().value()));

        return;
    }

    if (cluster_id() != parent_group->cluster_id()) {
        ec = xdata_accessor_errc_t::cluster_id_mismatch;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": trying to associate parent group from cluster %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()),
              static_cast<std::uint16_t>(parent_group->cluster_id().value()));

        return;
    }

    bool did_associate_parent{false};
    bool associates_twice{false};

    XLOCK_GUARD(m_associated_parent_group_mutex) {
        if (m_associated_parent_group.expired()) {
            m_associated_parent_group = parent_group;
            did_associate_parent = true;
        } else if (!m_associated_parent_group.owner_before(parent_group) && !parent_group.owner_before(m_associated_parent_group)) {
            associates_twice = true;
        }
    }

    if (did_associate_parent) {
        std::error_code ec1{xdata_accessor_errc_t::success};
        parent_group->associate_child_group(shared_from_this(), ec1);

        if (ec1 && ec1 != xdata_accessor_errc_t::associate_child_group_twice) {
            ec = xdata_accessor_errc_t::associate_child_group_failed;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": associates child group %" PRIu16 " failed",
                  ec.category().name(),
                  static_cast<std::uint32_t>(parent_group->network_id().value()),
                  static_cast<std::uint16_t>(parent_group->zone_id().value()),
                  static_cast<std::uint16_t>(parent_group->cluster_id().value()),
                  static_cast<std::uint16_t>(parent_group->group_id().value()),
                  static_cast<std::uint16_t>(group_id().value()));

            return;
        }

        return;
    }

    if (associates_twice) {
        ec = xdata_accessor_errc_t::associate_parent_group_twice;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": trying to associate parent group %" PRIu16 " twice",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()),
              static_cast<std::uint16_t>(parent_group->group_id().value()));

        return;
    }

    ec = xdata_accessor_errc_t::associate_to_different_parent_group;

    xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": associates to a different parent group %" PRIu16 " at version %" PRIu64
          " which is not allowed",
          ec.category().name(),
          static_cast<std::uint32_t>(network_id().value()),
          static_cast<std::uint16_t>(zone_id().value()),
          static_cast<std::uint16_t>(cluster_id().value()),
          static_cast<std::uint16_t>(group_id().value()),
          static_cast<std::uint16_t>(parent_group->group_id().value()),
          static_cast<std::uint64_t>(parent_group->version().value()));

    return;
}

void xtop_group_element::associate_parent_group(std::shared_ptr<xtop_group_element> const & parent_group) {
    std::error_code ec;
    associate_parent_group(parent_group, ec);
    top::error::throw_error(ec);
}

std::shared_ptr<xtop_group_element> xtop_group_element::associated_parent_group() const {
    XLOCK(m_associated_parent_group_mutex);
    return m_associated_parent_group.lock();
}

std::shared_ptr<xtop_group_element> xtop_group_element::associated_parent_group(std::error_code & ec) const {
    assert(!ec);
    std::shared_ptr<xtop_group_element> group_element;
    XLOCK_GUARD(m_associated_parent_group_mutex) { group_element = m_associated_parent_group.lock(); }

    if (group_element == nullptr) {
        ec = xdata_accessor_errc_t::associated_group_not_exist;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " don't have associated parent group",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()));
    }

    return group_element;
}

void xtop_group_element::associate_child_group(std::shared_ptr<xtop_group_element> const & child_group, std::error_code & ec) {
    assert(!ec);
    assert(network_id() == child_group->network_id());
    assert(zone_id() == child_group->zone_id());
    assert(cluster_id() == child_group->cluster_id());

    bool did_associate_child_group{false};

    XLOCK(m_associated_child_groups_mutex);
    bool found_child_group{false};
    // clean up expired associated children
    for (auto & associated_child_store : m_associated_child_groups) {
        auto & associated_child_group_container = top::get<xassociated_group_container_t>(associated_child_store);
        for (auto it = std::begin(associated_child_group_container); it != std::end(associated_child_group_container);) {
            auto const & associated_child_group = top::get<std::weak_ptr<xgroup_element_t>>(*it).lock();
            if (associated_child_group == nullptr) {
                it = associated_child_group_container.erase(it);
            } else {
                if (!associated_child_group.owner_before(child_group) && !child_group.owner_before(associated_child_group)) {
                    found_child_group = true;
                }

                ++it;
            }
        }
    }

    if (!found_child_group) {
        m_associated_child_groups[child_group->group_id()][child_group->start_time()] = child_group;
    } else {
        ec = xdata_accessor_errc_t::associate_child_group_twice;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " is already associated with child group %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()),
              static_cast<std::uint16_t>(child_group->group_id().value()));
    }
}

std::vector<std::shared_ptr<xtop_group_element>> xtop_group_element::associated_child_groups(common::xlogic_time_t const logic_time, std::error_code & ec) const {
    assert(!ec);
    std::vector<std::shared_ptr<xtop_group_element>> ret;
    XLOCK_GUARD(m_associated_child_groups_mutex) {
        if (m_associated_child_groups.empty()) {
            ec = xdata_accessor_errc_t::associated_group_not_exist;
            xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " doesn't have any associated child groups",
                  ec.category().name(),
                  static_cast<std::uint32_t>(network_id().value()),
                  static_cast<std::uint16_t>(zone_id().value()),
                  static_cast<std::uint16_t>(cluster_id().value()),
                  static_cast<std::uint16_t>(group_id().value()));
            break;
        }

        for (auto const & associated_child_group : m_associated_child_groups) {
            auto const & associated_children = top::get<xassociated_group_container_t>(associated_child_group);
            for (auto const & associated_child_info : associated_children) {
                auto candidate_child = top::get<std::weak_ptr<xtop_group_element>>(associated_child_info).lock();
                if (candidate_child != nullptr && candidate_child->enabled(logic_time)) {
                    ret.push_back(std::move(candidate_child));
                }
            }
        }
    }

    if (ret.empty()) {
        ec = xdata_accessor_errc_t::associated_group_not_exist;
        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " doesn't have any associated child groups at logic time %" PRIu64,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()),
              static_cast<std::uint64_t>(logic_time));
    }

    return ret;
}

bool xtop_group_element::enabled(common::xlogic_time_t const logic_time) const noexcept {
    return start_time() <= logic_time;
}

std::map<common::xslot_id_t, std::shared_ptr<xnode_element_t>> xtop_group_element::children() const {
    std::error_code ec{xdata_accessor_errc_t::success};
    auto ret = children(ec);
    top::error::throw_error(ec);
    return ret;
}

std::map<common::xslot_id_t, std::shared_ptr<xnode_element_t>> xtop_group_element::children(std::error_code & ec) const {
    assert(!ec);

    XLOCK(m_node_elements_mutex);
    if (m_node_elements.empty()) {
        ec = xdata_accessor_errc_t::no_children;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " has no child",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id().value()));
    }

    return m_node_elements;
}

bool xtop_group_element::exist_with_lock_hold_outside(common::xslot_id_t const & slot_id) const noexcept {
    return m_node_elements.find(slot_id) != std::end(m_node_elements);
}

NS_END3
