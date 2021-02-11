#pragma once

#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xdistance.hpp"
#include "xnetwork/xp2p/xtime_point.h"

#include <cassert>
#include <functional>
#include <utility>

NS_BEG3(top, network, p2p)

template <typename DistanceT>
class xtop_node_entry final : xdht_node_t
{
private:
    using base_t = xdht_node_t;

    DistanceT m_distance{};
    xtime_point_t m_last_pong_time{};
    // std::uint16_t m_app_port{ 0 };
    // bool m_pending{ true };

public:
    xtop_node_entry()                                    = default;
    xtop_node_entry(xtop_node_entry const &)             = default;
    xtop_node_entry & operator=(xtop_node_entry const &) = default;
    xtop_node_entry(xtop_node_entry &&)                  = default;
    xtop_node_entry & operator=(xtop_node_entry &&)      = default;
    ~xtop_node_entry()                                   = default;

    xtop_node_entry(DistanceT dist, xdht_node_t host_node) noexcept
        : base_t{ std::move(host_node) }, m_distance{ std::move(dist) }
    {
    }

    //xtop_node_entry(DistanceT distance,
    //                xdht_node_t host_node,
    //                std::uint16_t const app_port) noexcept
    //    : base_t{ std::move(host_node) }, m_distance{ std::move(distance) }, m_app_port{ app_port }
    //{
    //    assert(m_app_port != 0);
    //}

    using id_type = base_t::id_type;
    using endpoint_type = base_t::endpoint_type;

    using base_t::id;
    using base_t::endpoint;
    using base_t::empty;

    xdht_node_t
    dht_node() const {
        return { id(), endpoint() };
    }

    void
    swap(xtop_node_entry & other) noexcept {
        base_t::swap(other);
        // std::swap(m_app_port, other.m_app_port);
        std::swap(m_last_pong_time, other.m_last_pong_time);
        m_distance.swap(other.m_distance);
        //std::swap(m_pending, other.m_pending);
    }

    DistanceT const &
    distance() const noexcept {
        return m_distance;
    }

    //std::uint16_t
    //app_port() const noexcept {
    //    assert(m_app_port != 0);
    //    return m_app_port;
    //}

    xtime_point_t const &
    last_pong_time() const noexcept {
        return m_last_pong_time;
    }

    void
    last_pong_time(xtime_point_t const & tp) {
        m_last_pong_time = tp;
    }
};

using xnode_entry_t = xtop_node_entry<xdistance_t>;

NS_END3

NS_BEG1(std)

template <>
struct hash<top::network::p2p::xnode_entry_t> final
{
    std::size_t
    operator()(top::network::p2p::xnode_entry_t const & node_entry) const noexcept;
};

void
swap(top::network::p2p::xnode_entry_t & lhs, top::network::p2p::xnode_entry_t & rhs) noexcept;

NS_END1
