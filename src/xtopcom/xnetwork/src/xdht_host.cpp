// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xnetwork/xp2p/xdht_host.h"

#include <algorithm>
#include <cassert>
#include <iterator>

NS_BEG3(top, network, p2p)

xtop_dht_host::xtop_dht_host(common::xnode_id_t node_id,
                             std::shared_ptr<xsocket_face_t> dht_socket,
                             observer_ptr<xtimer_driver_t> timer_driver)
    : m_routing_table{ std::make_shared<xrouting_table_t>(std::move(node_id),
                                                          std::move(dht_socket),
                                                          timer_driver) }
{
}

void
xtop_dht_host::start() {
    assert(m_routing_table);
    m_routing_table->start();
}

void
xtop_dht_host::stop() {
    assert(m_routing_table);
    m_routing_table->stop();
}

bool
xtop_dht_host::running() const noexcept {
    assert(m_routing_table);
    return m_routing_table->running();
}

void
xtop_dht_host::running(bool const value) noexcept {
    assert(m_routing_table);
    m_routing_table->running(value);
}

void
// std::vector<xbootstrap_result_future_t>
xtop_dht_host::bootstrap(std::vector<xdht_node_t> const & seeds) const {
    assert(m_routing_table);
    // std::vector<xbootstrap_result_future_t> result;
    for (auto const & seed : seeds) {
        if (local_endpoint() == seed.endpoint()) {
            continue;
        }
        // result.push_back(m_routing_table->ping(seed));
        m_routing_table->ping(seed);
        xdbg("[dht] routing table ping %s", seed.id().to_string().c_str());
    }

    // return result;
}

common::xnode_id_t const &
xtop_dht_host::host_node_id() const noexcept {
    assert(m_routing_table);
    return m_routing_table->host_node_id();
}

xdht_node_t
xtop_dht_host::host_dht_node() const {
    assert(m_routing_table);
    return m_routing_table->host_dht_node();
}


std::vector<std::shared_ptr<xnode_entry_t>>
xtop_dht_host::nearest_node_entries(common::xnode_id_t const & target) const {
    assert(m_routing_table);
    return m_routing_table->nearest_node_entries(target);
}

std::vector<std::shared_ptr<xnode_entry_t>>
xtop_dht_host::all_node_entries() const {
    assert(m_routing_table);
    auto const & all_known_nodes = m_routing_table->all_known_nodes();
    std::vector<std::shared_ptr<xnode_entry_t>> ret;
    ret.reserve(all_known_nodes.size());
    std::transform(std::begin(all_known_nodes),
                   std::end(all_known_nodes),
                   std::back_inserter(ret),
                   [](std::pair<common::xnode_id_t const, std::shared_ptr<xnode_entry_t>> const & node_pair) {
        return top::get<std::shared_ptr<xnode_entry_t>>(node_pair);
    });

    return ret;
}

xnode_endpoint_t
xtop_dht_host::local_endpoint() const {
    assert(m_routing_table);
    return m_routing_table->endpoint(m_routing_table->host_node_id());
}

xnode_endpoint_t
xtop_dht_host::endpoint(common::xnode_id_t const & node_id) const {
    assert(m_routing_table);
    return m_routing_table->endpoint(node_id);
}

std::shared_ptr<xnode_entry_t>
xtop_dht_host::node_entry(common::xnode_id_t const & target) const {
    assert(m_routing_table);
    return m_routing_table->node_entry(target);
}

std::size_t
xtop_dht_host::neighbor_size_upper_limit() const noexcept {
    assert(m_routing_table);
    return m_routing_table->size_upper_limit();
}

//xtop_dht_host3::xtop_dht_host3(std::shared_ptr<kadmlia::RoutingTable> routing_table)
//        : m_routing_table{ std::move(routing_table) }
//{
//}
//
//void
//xtop_dht_host3::start() {
//    // TODO(smaug) attention return code
//    m_routing_table->Init();
//}
//
//void
//xtop_dht_host3::stop() {
//    m_routing_table->UnInit();
//    m_routing_table = nullptr;
//}
//
//void
//xtop_dht_host3::bootstrap(std::vector<network::xdht_node_t> const & seeds) const {
//    std::set<std::pair<std::string, uint16_t>> boot_endpoints;
//    for (auto const & seed : seeds) {
//        boot_endpoints.insert(std::make_pair(seed.endpoint().address(), seed.endpoint().port()));
//    }
//    if (boot_endpoints.empty()) {
//        TOP_ERROR("no seed give, bootstrap failed");
//        return;
//    }
//    // TODO(smaug) attention of  return code
//    if (m_routing_table->MultiJoin(boot_endpoints) != kadmlia::kKadSuccess) {
//        TOP_ERROR("MultiJoin failed");
//    }
//    return;
//}
//
//network::common::xnode_id_t const &
//xtop_dht_host3::host_node_id() const noexcept {
//    std::string id;
//    if (m_routing_table) {
//        id = m_routing_table->get_local_node_info()->id();
//    }
//    static network::xtop_node_id host_id(id);
//    return host_id;
//}
//
//network::xdht_node_t
//xtop_dht_host3::host_dht_node() const {
//    std::string id = m_routing_table->get_local_node_info()->id();
//    std::string public_ip = m_routing_table->get_local_node_info()->public_ip();
//    uint16_t public_port = m_routing_table->get_local_node_info()->public_port();
//
//    network::xnode_endpoint_t endpoint{ public_ip, public_port };
//    network::xtop_node_id host_id(id);
//    network::xdht_node_t host_node{host_id, endpoint};
//    return host_node;
//}
//
//std::vector<std::shared_ptr<xnode_entry_t>>
//xtop_dht_host3::nearest_node_entries(common::xnode_id_t const & target) const {
//    std::vector<std::shared_ptr<network::p2p::xnode_entry_t>> near_vec;
//    auto near_node_vec = m_routing_table->GetClosestNodes(
//            static_cast<std::string>(target.value()), 16);
//    for (auto& nodeinfo : near_node_vec) {
//        network::xnode_endpoint_t endpoint{nodeinfo->public_ip, nodeinfo->public_port};
//        network::xtop_node_id host_id(nodeinfo->node_id);
//        network::xdht_node_t host_node{host_id, endpoint};
//        std::shared_ptr<network::p2p::xnode_entry_t> xnptr =
//            //std::make_shared<network::p2p::xnode_entry_t>(network::p2p::xdistance_t(0), host_node,0);
//            std::make_shared<network::p2p::xnode_entry_t>(network::p2p::xdistance_t(0), host_node);
//        near_vec.push_back(xnptr);
//    }
//    return near_vec;
//}
//
//std::vector<std::shared_ptr<xnode_entry_t>>
//xtop_dht_host3::all_node_entries() const {
//    std::vector<std::shared_ptr<network::p2p::xnode_entry_t>> near_vec;
//    auto nodes = m_routing_table->nodes();
//    for (auto& nodeinfo : nodes) {
//        network::xnode_endpoint_t endpoint{nodeinfo->public_ip, nodeinfo->public_port};
//        network::xtop_node_id host_id(nodeinfo->node_id);
//        network::xdht_node_t host_node{host_id, endpoint};
//        std::shared_ptr<network::p2p::xnode_entry_t> xnptr =
//            std::make_shared<network::p2p::xnode_entry_t>(network::p2p::xdistance_t(0), host_node);
//        near_vec.push_back(xnptr);
//    }
//    return near_vec;
//}
//
//network::xnode_endpoint_t
//xtop_dht_host3::local_endpoint() const {
//    std::string public_ip = m_routing_table->get_local_node_info()->public_ip();
//    uint16_t public_port = m_routing_table->get_local_node_info()->public_port();
//    network::xnode_endpoint_t endpoint{public_ip, public_port};
//    return endpoint;
//}
//
//network::xnode_endpoint_t
//xtop_dht_host3::endpoint(network::common::xnode_id_t const & node_id) const {
//    auto node_ptr = m_routing_table->FindLocalNode(static_cast<std::string>(node_id.value()));
//    if (!node_ptr) {
//        network::xnode_endpoint_t endpoint{node_ptr->public_ip, node_ptr->public_port};
//        return endpoint;
//    }
//    return network::xnode_endpoint_t{};
//}
//
//std::shared_ptr<xnode_entry_t>
//xtop_dht_host3::node_entry(common::xnode_id_t const & target) const {
//    auto node_ptr = m_routing_table->FindLocalNode(static_cast<std::string>(target.value()));
//    if (!node_ptr) {
//        return nullptr;
//    }
//    network::xnode_endpoint_t endpoint{node_ptr->public_ip, node_ptr->public_port};
//    network::xtop_node_id host_id(node_ptr->node_id);
//    network::xdht_node_t host_node{host_id, endpoint};
//    auto xnptr = std::make_shared<xnode_entry_t>(xdistance_t(0), host_node);
//    return  xnptr;
//}
//
//std::size_t
//xtop_dht_host3::neighbor_size_upper_limit() const noexcept {
//    return kadmlia::kRoutingMaxNodesSize;
//}
//
//std::vector<kadmlia::NodeInfoPtr>
//xtop_dht_host3::nodes() {
//    std::vector<kadmlia::NodeInfoPtr> nodes = m_routing_table->nodes();
//    return nodes;
//}

NS_END3
