// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xbasic_vnode.h"

#include "xbasic/xutility.h"
#include "xvnode/xerror/xerror.h"

NS_BEG2(top, vnode)

xtop_basic_vnode::xtop_basic_vnode(common::xnode_address_t address,
                                   common::xminer_type_t miner_type,
                                   bool genesis,
                                   uint64_t raw_credit_score,
                                   common::xelection_round_t joined_election_round,
                                   observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                   observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor) noexcept
  : m_vhost{vhost}
  , m_election_cache_data_accessor{election_cache_data_accessor}
  , m_address{std::move(address)}
  , m_miner_type{miner_type}
  , m_genesis{genesis}
  , m_raw_credit_score{raw_credit_score}
  , m_joined_election_round{std::move(joined_election_round)} {
}

common::xnode_type_t xtop_basic_vnode::type() const noexcept {
    return common::real_part_type(m_address.type());
}

common::xnode_address_t const & xtop_basic_vnode::address() const noexcept {
    return m_address;
}

common::xminer_type_t xtop_basic_vnode::miner_type() const noexcept {
    return m_miner_type;
}

bool xtop_basic_vnode::genesis() const noexcept {
    return m_genesis;
}

common::xelection_round_t const & xtop_basic_vnode::joined_election_round() const noexcept {
    return m_joined_election_round;
}

void xtop_basic_vnode::broadcast(common::xip2_t const & broadcast_dst, vnetwork::xmessage_t const & msg, std::error_code & ec) {
    assert(m_vhost != nullptr);

    if (!running()) {
        ec = vnode::error::xerrc_t::vnode_is_not_running;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (!common::broadcast(broadcast_dst.network_id()) && !common::broadcast(broadcast_dst.zone_id()) && !common::broadcast(broadcast_dst.cluster_id()) &&
        !common::broadcast(broadcast_dst.group_id()) && !common::broadcast(broadcast_dst.slot_id())) {
        ec = vnode::error::xerrc_t::invalid_address;
        xwarn("%s %s. dst address is a broadcast address %s", ec.category().name(), ec.message().c_str(), broadcast_dst.to_string().c_str());
        return;
    }

    common::xnode_address_t dst{common::xgroup_address_t{broadcast_dst.xip()}, common::xlogic_epoch_t{broadcast_dst.size(), broadcast_dst.height()}};
    m_vhost->broadcast(address(), dst, msg, ec);
    if (ec) {
        xwarn("vnode::broadcast failed %s", ec.message().c_str());
    }
}

void xtop_basic_vnode::send_to(common::xip2_t const & unicast_dst, vnetwork::xmessage_t const & msg, std::error_code & ec) {
    assert(!ec);

    if (!running()) {
        ec = vnode::error::xerrc_t::vnode_is_not_running;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (common::broadcast(unicast_dst.network_id()) || common::broadcast(unicast_dst.zone_id()) || common::broadcast(unicast_dst.cluster_id()) ||
        common::broadcast(unicast_dst.group_id()) || common::broadcast(unicast_dst.slot_id())) {
        ec = vnode::error::xerrc_t::invalid_address;
        xwarn("%s %s. dst address is a broadcast address %s", ec.category().name(), ec.message().c_str(), unicast_dst.to_string().c_str());
        return;
    }

    auto dst_account_address = m_election_cache_data_accessor->account_address_from(unicast_dst, ec);
    if (ec) {
        xwarn("ec category: %s ec msg: %s", ec.category().name(), ec.message().c_str());
        return;
    }

    common::xnode_address_t to{common::xgroup_address_t{unicast_dst.xip()},
                               common::xaccount_election_address_t{dst_account_address, unicast_dst.slot_id()},
                               common::xlogic_epoch_t{unicast_dst.size(), unicast_dst.height()}};
    if (address().group_address() == to.group_address() && address().account_address() == to.account_address()) {
        ec = error::xerrc_t::invalid_address;
        return;
    }

    m_vhost->send_to(address(), to, msg, ec);
    if (ec) {
        xwarn("vnode::send_to failed %s", ec.message().c_str());
    }
}

common::xrotation_status_t xtop_basic_vnode::status() const noexcept {
    return rotation_status(m_vhost->last_logic_time());
}

std::vector<common::xip2_t> xtop_basic_vnode::neighbors_xip2(std::error_code & ec) const {
    assert(m_election_cache_data_accessor != nullptr);
    assert(!ec);

    {
        std::lock_guard<std::mutex> lock{m_neighbors_xip2_mutex};
        if (!m_neighbors_xip2.empty()) {
            return m_neighbors_xip2;
        }
    }

    auto group_element = m_election_cache_data_accessor->group_element(address().group_address(), address().logic_epoch(), ec);
    if (ec) {
        xwarn("vnode::neighbors_xip2: failed. err category %s, err msg %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    auto const & neighbors = group_element->children(ec);
    {
        std::lock_guard<std::mutex> lock{m_neighbors_xip2_mutex};
        if (!m_neighbors_xip2.empty()) {
            return m_neighbors_xip2;
        }

        m_neighbors_xip2.reserve(neighbors.size());
        std::transform(std::begin(neighbors),
                       std::end(neighbors),
                       std::back_inserter(m_neighbors_xip2),
                       [](std::pair<common::xslot_id_t const, std::shared_ptr<election::cache::xnode_element_t>> const & neighbor) {
                           auto const & node_element = top::get<std::shared_ptr<election::cache::xnode_element_t>>(neighbor);
#if !defined(NDEBUG)
                           auto const & slot_id = top::get<common::xslot_id_t const>(neighbor);
                           assert(slot_id == node_element->slot_id());
#endif

                           return node_element->xip2();
                       });

        return m_neighbors_xip2;
    }
}

std::vector<common::xip2_t> xtop_basic_vnode::associated_parent_nodes_xip2(std::error_code & ec) const {
    assert(!ec);
    assert(m_election_cache_data_accessor != nullptr);

    {
        std::lock_guard<std::mutex> lock{m_associated_parent_xip2_mutex};
        if (!m_associated_parent_xip2.empty()) {
            return m_associated_parent_xip2;
        }
    }

    auto const & parent_group_address = m_election_cache_data_accessor->parent_address(address().group_address(), address().logic_epoch(), ec);
    if (ec) {
        xwarn("xbasic_vnode_t::associated_parent_nodes_xip2 failed. err category %s; err msg %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    auto const & parent_nodes_address = m_election_cache_data_accessor->group_nodes(parent_group_address.group_address(), parent_group_address.logic_epoch(), ec);
    if (ec) {
        xwarn("xbasic_vnode_t::associated_parent_nodes_xip2 failed. err category %s; err msg %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    {
        std::lock_guard<std::mutex> lock{m_associated_parent_xip2_mutex};
        if (!m_associated_parent_xip2.empty()) {
            return m_associated_parent_xip2;
        }

        m_associated_parent_xip2.reserve(parent_nodes_address.size());
        std::transform(std::begin(parent_nodes_address),
                       std::end(parent_nodes_address),
                       std::back_inserter(m_associated_parent_xip2),
                       [](std::pair<common::xslot_id_t const, data::xnode_info_t> const & node_data) {
                           auto const & node_datum = top::get<data::xnode_info_t>(node_data);
                           return node_datum.address.xip2();
                       });
        return m_associated_parent_xip2;
    }
}

std::vector<common::xip2_t> xtop_basic_vnode::associated_child_nodes_xip2(common::xip2_t const & child_group_xip2, std::error_code & ec) const {
    assert(m_election_cache_data_accessor != nullptr);
    assert(!ec);

    auto const & child_group_xip = child_group_xip2.xip();
    {
        std::lock_guard<std::mutex> lock{m_associated_child_xip2_mutex};
        auto const it = m_associated_child_xip2.find(child_group_xip);
        if (it != std::end(m_associated_child_xip2)) {
            return top::get<std::vector<common::xip2_t>>(*it);
        }
    }

    auto const & child_group_addresses = m_election_cache_data_accessor->child_addresses(address().group_address(), address().logic_epoch(), ec);
    if (ec) {
        xwarn("xbasic_vnode_t::associated_child_nodes_xip2 failed. err category %s; err msg %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    common::xgroup_address_t const child_group_address{child_group_xip};
    auto const it = std::find_if(std::begin(child_group_addresses),
                                 std::end(child_group_addresses),
                                [&child_group_address, &child_group_xip2](common::xnode_address_t const & address) {
                                    return address.group_address().xip() == child_group_address.xip() && address.associated_blk_height() == child_group_xip2.height();
                                });
    if (it == std::end(child_group_addresses)) {
        ec = error::xerrc_t::invalid_address;
        return {};
    }

    auto const & child_nodes_address =
        m_election_cache_data_accessor->group_nodes(child_group_address, common::xlogic_epoch_t{child_group_xip2.size(), child_group_xip2.height()}, ec);
    if (ec) {
        xwarn("xbasic_vnode_t::associated_child_nodes_xip2 failed. err category %s; err msg %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    {
        std::lock_guard<std::mutex> lock{m_associated_child_xip2_mutex};
        auto const it = m_associated_child_xip2.find(child_group_xip);
        if (it != std::end(m_associated_child_xip2)) {
            return top::get<std::vector<common::xip2_t>>(*it);
        }

        m_associated_child_xip2[child_group_xip].reserve(child_nodes_address.size());
        std::transform(std::begin(child_nodes_address),
                       std::end(child_nodes_address),
                       std::back_inserter(m_associated_child_xip2[child_group_xip]),
                       [](std::pair<common::xslot_id_t const, data::xnode_info_t> const & node_data) {
                           auto const & node_datum = top::get<data::xnode_info_t>(node_data);
                           return node_datum.address.xip2();
                       });
        return m_associated_child_xip2[child_group_xip];
    }
}

//std::vector<common::xip2_t> xtop_basic_vnode::associated_nodes_xip2(common::xip_t const & group_xip, std::error_code & ec) const {
//    assert(m_election_cache_data_accessor != nullptr);
//    assert(!ec);
//    assert(common::broadcast(group_xip.slot_id()));
//
//    if (!common::has<common::xnode_type_t::auditor>(type()) && !common::has<common::xnode_type_t::validator>(type())) {
//        ec = error::xerrc::invalid_address;
//        return {};
//    }
//
//    auto current_group_xip = address().xip2().xip().group_xip();
//    if (current_group_xip.network_id() != group_xip.network_id()) {
//        ec = error::xerrc::invalid_address;
//        return {};
//    }
//
//    if (current_group_xip == group_xip) {
//        ec = error::xerrc::invalid_address;
//        return {};
//    }
//
//    auto group_element = m_election_cache_data_accessor->group_element(address().group_address(), address().logic_epoch(), ec);
//    if (ec) {
//        xwarn("vnode::associated_nodes_xip2 failed. err category %s, err msg %s", ec.category().name(), ec.message().c_str());
//        return {};
//    }
//
//    {
//        std::lock_guard<std::mutex> lock{m_associated_nodes_xip2_mutex};
//        auto const it = m_associated_nodes_xip2.find(group_xip);
//        if (it != std::end(m_associated_nodes_xip2)) {
//            return top::get<std::vector<common::xip2_t>>(*it);
//        }
//    }
//
//    if (common::has<common::xnode_type_t::auditor>(type())) {
//        auto child_groups = group_element->associated_child_groups(ec);
//        if (ec) {
//            xwarn("vnode::associated_nodes_xip2 failed. err category %s, err msg %s", ec.category().name(), ec.message().c_str());
//            return {};
//        }
//
//        auto const it = std::find_if(std::begin(child_groups), std::end(child_groups), [group_xip](std::shared_ptr<election::cache::xgroup_element_t> const & child_group) {
//            return child_group->xip2().xip().group_xip() == group_xip;
//        });
//
//        if (it == std::end(child_groups)) {
//            ec = error::xerrc::invalid_address;
//            xwarn("vnode::associated_nodes_xip2 failed. vnode address %s, input group xip %s", address().xip2().to_string().c_str(), group_xip.to_string().c_str());
//            return {};
//        }
//
//        auto const & child_nodes = (*it)->children(ec);
//        {
//            std::lock_guard<std::mutex> lock{m_associated_nodes_xip2_mutex};
//            // make sure the followed std::transform executes only once.
//            auto const it = m_associated_nodes_xip2.find(group_xip);
//            if (it != std::end(m_associated_nodes_xip2)) {
//                return top::get<std::vector<common::xip2_t>>(*it);
//            }
//
//            std::transform(std::begin(child_nodes),
//                           std::end(child_nodes),
//                           std::back_inserter(m_associated_nodes_xip2[group_xip]),
//                           [](std::pair<common::xslot_id_t const, std::shared_ptr<election::cache::xnode_element_t>> const & node) {
//                               auto const & slot_id = top::get<common::xslot_id_t const>(node);
//                               auto const & node_element = top::get<std::shared_ptr<election::cache::xnode_element_t>>(node);
//
//                               assert(slot_id == node_element->slot_id());
//
//                               return node_element->xip2();
//                           });
//
//            return m_associated_nodes_xip2[group_xip];
//        }
//    } else {
//        assert(common::has<common::xnode_type_t::validator>(type()));
//        auto associated_parent = group_element->associated_parent_group(ec);
//        if (ec) {
//            xwarn("vnode::associated_nodes_xip2 failed. vnode address %s, input group xip %s, err category %s, err msg %s",
//                  address().xip2().to_string().c_str(),
//                  group_xip.to_string().c_str(),
//                  ec.category().name(),
//                  ec.message().c_str());
//
//            return {};
//        }
//        assert(associated_parent != nullptr);
//
//        auto child_groups = associated_parent->associated_child_groups(ec);
//        if (ec) {
//            xwarn("vnode::associated_nodes_xip2 failed. err category %s, err msg %s", ec.category().name(), ec.message().c_str());
//            return {};
//        }
//
//        if (associated_parent->address().xip2().xip().group_xip() == group_xip) {
//            // ec = error::xerrc::invalid_address;
//            // xwarn("vnode::associated_nodes_xip2 failed. vnode address %s, input group xip %s parent group xip %s",
//            //      address().xip2().to_string().c_str(),
//            //      group_xip.to_string().c_str(),
//            //      address().to_string().c_str());
//
//            // return {};
//
//            auto const & child_nodes = associated_parent->children(ec);
//            {
//                std::lock_guard<std::mutex> lock{m_associated_nodes_xip2_mutex};
//                // make sure the followed std::transform executes only once.
//                auto const it = m_associated_nodes_xip2.find(group_xip);
//                if (it != std::end(m_associated_nodes_xip2)) {
//                    return top::get<std::vector<common::xip2_t>>(*it);
//                }
//
//                std::transform(std::begin(child_nodes),
//                               std::end(child_nodes),
//                               std::back_inserter(m_associated_nodes_xip2[group_xip]),
//                               [](std::pair<common::xslot_id_t const, std::shared_ptr<election::cache::xnode_element_t>> const & node) {
//                                   auto const & slot_id = top::get<common::xslot_id_t const>(node);
//                                   auto const & node_element = top::get<std::shared_ptr<election::cache::xnode_element_t>>(node);
//
//                                   assert(slot_id == node_element->slot_id());
//
//                                   return node_element->xip2();
//                               });
//
//                return m_associated_nodes_xip2[group_xip];
//            }
//        } else {
//            auto const it = std::find_if(std::begin(child_groups), std::end(child_groups), [group_xip](std::shared_ptr<election::cache::xgroup_element_t> const & child_group) {
//                return child_group->xip2().xip().group_xip() == group_xip;
//            });
//
//            if (it == std::end(child_groups)) {
//                ec = error::xerrc::invalid_address;
//                xwarn("vnode::associated_nodes_xip2 failed. vnode address %s, input group xip %s", address().xip2().to_string().c_str(), group_xip.to_string().c_str());
//                return {};
//            }
//
//            auto const & child_nodes = (*it)->children(ec);
//            {
//                std::lock_guard<std::mutex> lock{m_associated_nodes_xip2_mutex};
//                // make sure the followed std::transform executes only once.
//                auto const it = m_associated_nodes_xip2.find(group_xip);
//                if (it != std::end(m_associated_nodes_xip2)) {
//                    return top::get<std::vector<common::xip2_t>>(*it);
//                }
//
//                std::transform(std::begin(child_nodes),
//                               std::end(child_nodes),
//                               std::back_inserter(m_associated_nodes_xip2[group_xip]),
//                               [](std::pair<common::xslot_id_t const, std::shared_ptr<election::cache::xnode_element_t>> const & node) {
//                                   auto const & slot_id = top::get<common::xslot_id_t const>(node);
//                                   auto const & node_element = top::get<std::shared_ptr<election::cache::xnode_element_t>>(node);
//
//                                   assert(slot_id == node_element->slot_id());
//
//                                   return node_element->xip2();
//                               });
//
//                return m_associated_nodes_xip2[group_xip];
//            }
//        }
//
//        ec = error::xerrc::invalid_address;
//        return {};
//    }
//}
//
//std::vector<common::xip2_t> xtop_basic_vnode::nonassociated_nodes_xip2(common::xip_t const & group_xip, std::error_code & ec) const {
//    assert(!ec);
//    assert(m_election_cache_data_accessor != nullptr);
//
//    auto const & vnode_group_xip = address().xip2().xip().group_xip();
//    if (vnode_group_xip == group_xip) {
//        ec = error::xerrc::invalid_address;
//        xwarn("vnode::nonassociated_nodes_xip2 vnode group address %s is same as input", address().to_string().c_str());
//        return {};
//    }
//
//    auto group_element = m_election_cache_data_accessor->group_element_by_logic_time(common::xgroup_address_t{group_xip}, m_vhost->last_logic_time(), ec);
//    if (ec) {
//        xwarn("vnode::nonassociated_nodes_xip2 failed. err category %s err msg %s", ec.category().name(), ec.message().c_str());
//        return {};
//    }
//    assert(group_element != nullptr);
//
//    std::error_code ec1, ec2;
//    auto const associated_child_groups = group_element->associated_child_groups(ec1);
//    auto const associated_parent_group = group_element->associated_parent_group(ec2);
//    std::vector<std::shared_ptr<election::cache::xgroup_element_t>> associated_neighbors_group;
//    if (associated_parent_group != nullptr) {
//        if (associated_parent_group->address().xip2().xip().group_xip() == vnode_group_xip) {
//            ec = error::xerrc::invalid_address;
//            xwarn("vnode::nonassociated_nodes_xip2 failed. group xip %s is associated with this vnode %s", group_xip.to_string().c_str(), address().to_string().c_str());
//            return {};
//        }
//
//        std::error_code ec3;
//        associated_neighbors_group = associated_parent_group->associated_child_groups(ec3);
//    }
//    // skip checking ec1 & ec2, useless.
//
//    for (auto const & child_group_element : associated_child_groups) {
//        if (child_group_element->address().xip2().xip().group_xip() == vnode_group_xip) {
//            ec = error::xerrc::invalid_address;
//            xwarn("vnode::nonassociated_nodes_xip2 failed. group xip %s is associated with this vnode %s", group_xip.to_string().c_str(), address().to_string().c_str());
//            return {};
//        }
//    }
//
//    for (auto const & neighbor_group_element : associated_neighbors_group) {
//        if (neighbor_group_element->address().xip2().xip().group_xip() == vnode_group_xip) {
//            ec = error::xerrc::invalid_address;
//            xwarn("vnode::nonassociated_nodes_xip2 failed. group xip %s is associated with this vnode %s", group_xip.to_string().c_str(), address().to_string().c_str());
//            return {};
//        }
//    }
//
//    auto const & child_nodes = group_element->children();
//    std::vector<common::xip2_t> nodes;
//    nodes.reserve(child_nodes.size());
//    std::transform(std::begin(child_nodes),
//                   std::end(child_nodes),
//                   std::back_inserter(nodes),
//                   [](std::pair<common::xslot_id_t const, std::shared_ptr<election::cache::xnode_element_t>> const & node_element_data) {
//                       return top::get<std::shared_ptr<election::cache::xnode_element_t>>(node_element_data)->xip2();
//                   });
//
//    return nodes;
//}

uint64_t xtop_basic_vnode::raw_credit_score() const noexcept {
    return m_raw_credit_score;
}

NS_END2
