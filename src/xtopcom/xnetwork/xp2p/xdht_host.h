// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlog.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver_fwd.h"
#include "xbasic/xutility.h"
#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xdht_host_face.h"
#include "xnetwork/xp2p/xrouting_table.h"

#include <cassert>
#include <memory>

NS_BEG3(top, network, p2p)

class xtop_dht_host final : public xdht_host_face_t
{
private:
    using base_t = xdht_host_face_t;

    std::shared_ptr<xrouting_table_t> m_routing_table{};

public:
    xtop_dht_host(xtop_dht_host const &)             = delete;
    xtop_dht_host & operator=(xtop_dht_host const &) = delete;
    xtop_dht_host(xtop_dht_host &&)                  = default;
    xtop_dht_host & operator=(xtop_dht_host &&)      = default;
    ~xtop_dht_host() override                        = default;

    xtop_dht_host(common::xnode_id_t node_id,
                  std::shared_ptr<xsocket_face_t> dht_socket,
                  observer_ptr<xtimer_driver_t> timer_driver);

    void
    start() override;

    void
    stop() override;

    bool
    running() const noexcept override;

    void
    running(bool const value) noexcept override;

    void
    // std::vector<xbootstrap_result_future_t>
    bootstrap(std::vector<xdht_node_t> const & seeds) const override;

    common::xnode_id_t const &
    host_node_id() const noexcept override;

    xdht_node_t
    host_dht_node() const override;

    std::vector<std::shared_ptr<xnode_entry_t>>
    nearest_node_entries(common::xnode_id_t const & target) const override;

    std::vector<std::shared_ptr<xnode_entry_t>>
    all_node_entries() const override;

    xnode_endpoint_t
    local_endpoint() const override;

    xnode_endpoint_t
    endpoint(common::xnode_id_t const & node_id) const override;

    std::shared_ptr<xnode_entry_t>
    node_entry(common::xnode_id_t const & target) const override;

    std::size_t
    neighbor_size_upper_limit() const noexcept override;
};

//template
//<
//    typename RoutingTable,
//    typename std::enable_if<std::is_base_of<xrouting_table_face_t, RoutingTable>::value>::type * = nullptr
//>
//class xtop_dht_host2 final : public xdht_host_face_t
//{
//private:
//    using base_t = xdht_host_face_t;
//
//    std::shared_ptr<xrouting_table_face_t> m_routing_table{};
//
//public:
//    xtop_dht_host2(xtop_dht_host2 const &)             = delete;
//    xtop_dht_host2 & operator=(xtop_dht_host2 const &) = delete;
//    xtop_dht_host2(xtop_dht_host2 &&)                  = default;
//    xtop_dht_host2 & operator=(xtop_dht_host2 &&)      = default;
//    ~xtop_dht_host2() override                         = default;
//
//    explicit
//    xtop_dht_host2(std::shared_ptr<xrouting_table_face_t> routing_table)
//        : m_routing_table{ std::move(routing_table) }
//    {
//        if (m_routing_table == nullptr) {
//            throw std::invalid_argument{ u8"routing table null"};
//        }
//    }
//
//    void
//    start() override {
//        assert(m_routing_table);
//        m_routing_table->start();
//    }
//
//    void
//    stop() override {
//        assert(m_routing_table);
//        m_routing_table->stop();
//    }
//
//    bool
//    running() const noexcept override {
//        return m_routing_table->running();
//    }
//
//    void
//    running(bool const value) noexcept override {
//        m_routing_table->running(value);
//    }
//
//    void
//    bootstrap(std::vector<xdht_node_t> const & seeds) const override {
//        assert(m_routing_table);
//        for (auto const & seed : seeds) {
//            m_routing_table->ping(seed);
//            xdbg("[dht] routing table ping %s", seed.id().to_string().c_str());
//        }
//    }
//
//    common::xnode_id_t const &
//    host_node_id() const noexcept override {
//        assert(m_routing_table);
//        return m_routing_table->host_node_id();
//    }
//
//    xdht_node_t
//    host_dht_node() const override {
//        assert(m_routing_table);
//        return m_routing_table->host_dht_node();
//    }
//
//    std::vector<std::shared_ptr<xnode_entry_t>>
//    nearest_node_entries(common::xnode_id_t const & target) const override {
//        assert(m_routing_table);
//        return m_routing_table->nearest_node_entries(target);
//    }
//
//    std::vector<std::shared_ptr<xnode_entry_t>>
//    all_node_entries() const override {
//        assert(m_routing_table);
//        auto const & all_known_nodes = m_routing_table->all_known_nodes();
//        std::vector<std::shared_ptr<xnode_entry_t>> ret;
//        ret.reserve(all_known_nodes.size());
//
//        for (auto const & node_info : all_known_nodes) {
//            ret.push_back(top::get<std::shared_ptr<xnode_entry_t>>(node_info));
//        }
//
//        return ret;
//    }
//
//    xnode_endpoint_t
//    local_endpoint() const override {
//        assert(m_routing_table);
//        return m_routing_table->endpoint(m_routing_table->host_node_id());
//    }
//
//    xnode_endpoint_t
//    endpoint(common::xnode_id_t const & node_id) const override {
//        assert(m_routing_table);
//        return m_routing_table->endpoint(node_id);
//    }
//
//    std::shared_ptr<xnode_entry_t>
//    node_entry(common::xnode_id_t const & target) const override {
//        assert(m_routing_table);
//        return m_routing_table->node_entry(target);
//    }
//
//    std::size_t
//    neighbor_size_upper_limit() const noexcept override {
//        assert(m_routing_table);
//        return m_routing_table->size_upper_limit();
//    }
//};
//
//class xtop_dht_host3 final : public xdht_host_face_t
//{
//private:
//    std::shared_ptr<kadmlia::RoutingTable> m_routing_table;
//
//public:
//    xtop_dht_host3(xtop_dht_host3 const &)             = delete;
//    xtop_dht_host3 & operator=(xtop_dht_host3 const &) = delete;
//    xtop_dht_host3(xtop_dht_host3 &&)                  = default;
//    xtop_dht_host3 & operator=(xtop_dht_host3 &&)      = default;
//    ~xtop_dht_host3() override                         = default;
//
//    explicit
//    xtop_dht_host3(std::shared_ptr<kadmlia::RoutingTable> routing_table);
//
//    void
//    start() override;
//
//    void
//    stop() override;
//
//    void
//    bootstrap(std::vector<network::xdht_node_t> const & seeds) const override;
//
//    common::xnode_id_t const &
//    host_node_id() const noexcept override;
//
//    xdht_node_t
//    host_dht_node() const override;
//
//    std::vector<std::shared_ptr<xnode_entry_t>>
//    nearest_node_entries(common::xnode_id_t const & target) const override;
//
//    std::vector<std::shared_ptr<xnode_entry_t>>
//    all_node_entries() const override;
//
//    network::xnode_endpoint_t
//    local_endpoint() const override;
//
//    network::xnode_endpoint_t
//    endpoint(network::common::xnode_id_t const & node_id) const override;
//
//    std::shared_ptr<xnode_entry_t>
//    node_entry(network::common::xnode_id_t const & target) const override;
//
//    std::size_t
//    neighbor_size_upper_limit() const noexcept override;
//
//    std::vector<kadmlia::NodeInfoPtr>
//    nodes();
//
//    std::shared_ptr<kadmlia::RoutingTable>
//    routing_table() const noexcept;
//};

using xdht_host_t = xtop_dht_host;
// using xdht_host3_t = xtop_dht_host3;

NS_END3
