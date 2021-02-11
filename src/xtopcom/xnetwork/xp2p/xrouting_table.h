// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"
#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xdiscover_datagram.h"
#include "xnetwork/xp2p/xdiscover_message.h"
#include "xnetwork/xp2p/xdistance.hpp"
#include "xnetwork/xp2p/xkbucket.hpp"
#include "xnetwork/xp2p/xnode_entry.hpp"
#include "xnetwork/xp2p/xrouting_table_face.h"
#include "xnetwork/xp2p/xtime_point.h"
#include "xnetwork/xsocket_face.h"

#include <atomic>
#include <chrono>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

NS_BEG3(top, network, p2p)

// using xping_result_future_t = std::future<bool>;
// using xping_result_promise_t = std::promise<bool>;
using xping_result_future_t = void;
#define xping_result_promise_t

class xtop_routing_table final : public std::enable_shared_from_this<xtop_routing_table>
                               , public xbasic_runnable_t<xtop_routing_table>
{
private:
    /**
     * \brief Kademlia: address size in bits.
     */
    static constexpr std::size_t m_address_bit_size{ xdistance_t::upper_bound() };
    /**
     * \brief Kademlia: k-bucket number.
     */
    static constexpr std::size_t m_bucket_count{ m_address_bit_size - 1 };
    /**
     * \brief Kademlia: 'k' value of k-bucket.
     */
    static constexpr std::size_t m_k{ 16 };
    /**
     * \brief Kademlia: The quantity of simultaneous lookups.
     */
    static constexpr std::size_t m_alpha{ 3 };
    /**
     * \brief The discover iteration.  It is the log2(m_address_bit_size)
     */
    static constexpr std::size_t m_max_discover_iteration{ 8 };
    /**
     * \brief k-buckets content refresh interval
     */
    static constexpr std::chrono::milliseconds m_buckets_refresh_interval{ 7200 };
    /**
     * \brief k-buckets discover interval
     */
    static constexpr std::chrono::milliseconds m_buckets_discover_interval{ 500 };
    /**
     * \brief request timeout
     */
    static constexpr std::chrono::milliseconds m_request_timout{ 1000 };
    static constexpr std::chrono::milliseconds m_eviction_check_interval{ 500 };               // eviction check interval
    static constexpr std::chrono::milliseconds m_eviction_timeout{ 3000 };                      // eviction candidate timeout
    static constexpr std::chrono::milliseconds m_node_id_discover_pings_check_interval{ 5000 };

public:
    using xshared_node_entry_ptr_t = std::shared_ptr<xnode_entry_t>;
    using xweak_node_entry_ptr_t = std::weak_ptr<xnode_entry_t>;

    using xkbucket_t = top::network::p2p::xkbucket_t<m_k>;

private:
    mutable std::mutex m_all_known_nodes_mutex{};
    /**
     * \brief All the nodes this table knows
     */
    std::unordered_map<common::xnode_id_t, xshared_node_entry_ptr_t> m_all_known_nodes{};

    mutable std::mutex m_kbuckets_mutex{};
    std::array<xkbucket_t, m_address_bit_size> m_kbuckets{};    // k-buckets

    struct xtop_ping_context final {
        xhash256_t ping_hash;
        xtime_point_t ping_send_time;
        common::xnode_id_t replacement_node_id;
        // xping_result_promise_t ping_result_promise;

        // void
        // ping_result_discarded() {
        //     ping_result_promise.set_value(false);
        // }
    };
    using xping_context_t = xtop_ping_context;

    mutable std::mutex m_sent_pings_mutex{};
    std::unordered_map<common::xnode_id_t, xping_context_t> m_sent_pings{};
    // std::unordered_map<xendpoint_t, xping_context_t> m_sent_pings2{};

    struct xtop_node_id_time_point {
        common::xnode_id_t node_id;
        xtime_point_t time_point;
    };
    using xnode_id_time_point_t = xtop_node_id_time_point;

    mutable std::mutex m_find_node_timeouts_mutex{};
    std::list<xnode_id_time_point_t> m_find_node_timeouts{};

    mutable std::mutex m_host_node_endpoint_mutex{};
    xnode_endpoint_t m_host_node_endpoint{};

    observer_ptr<xtimer_driver_t> m_timer_driver;

    common::xnode_id_t m_host_node_id;
    std::shared_ptr<xsocket_face_t> m_socket;
    // std::uint16_t m_host_app_socket_port;

public:

    xtop_routing_table(xtop_routing_table const &)             = delete;
    xtop_routing_table & operator=(xtop_routing_table const &) = delete;
    xtop_routing_table(xtop_routing_table &&)                  = default;
    xtop_routing_table & operator=(xtop_routing_table &&)      = default;
    ~xtop_routing_table()                                      = default;

    xtop_routing_table(common::xnode_id_t node_id,
                       std::shared_ptr<xsocket_face_t> socket,
                       observer_ptr<xtimer_driver_t> timer_driver);

    void
    start() override;

    void
    stop() override;

    enum class xenum_node_relationship
    {
        unknown,
        already_known,
        just_known,
        pending,
    };
    using xnode_relationship_t = xenum_node_relationship;

    /**
     * \brief Add node to _known_ nodes table.  We'll meet several situations here:
     *        1) we already know this id.
     *        2) we don't know this node, add it to known table and ping it.
     *
     *        Thus:
     *        1) if it's first added to the known table, a ping is triggered (node id and node endpoint are both
     *        not empty)
     *        2) if it's a known node, no further action is taken.
     *
     *        We'll see that if the node id and node endpoint are both not empty, the corresponding node can be added
     *        into the _known_ nodes table.
     * \param dht_node The DHT node to add.  Cannot be empty.
     * \param app_port The application socket port.
     */
    // void
    // add_node(xdht_node_t const & dht_node, std::uint16_t const app_port);

    void
    add_node(xdht_node_t const & dht_node);

    xping_result_future_t
    ping(xdht_node_t const & remote_dht_node);

    common::xnode_id_t
    node_id(xnode_endpoint_t const & endpoint) const;

    xnode_endpoint_t
    endpoint(common::xnode_id_t const & node_id) const;

    std::vector<std::shared_ptr<xnode_entry_t>>
    nearest_node_entries(common::xnode_id_t const & target) const;

    std::shared_ptr<xnode_entry_t>
    node_entry(common::xnode_id_t const & node_id) const;

    common::xnode_id_t const &
    host_node_id() const noexcept;

    xdht_node_t
    host_dht_node() const;

    std::unordered_map<common::xnode_id_t, xshared_node_entry_ptr_t>
    all_known_nodes() const;

    std::size_t
    size_upper_limit() const noexcept {
        return m_k * m_bucket_count;
    }

private:
    static
    xdistance_t
    make_distance(common::xnode_id_t const & left, common::xnode_id_t const & right) noexcept;

    /**
     * \brief Make a new node entry object composed with the node id and endpoint.
     *        The node id should be unknown.
     */
    // xshared_node_entry_ptr_t
    // make_node_entry(xdht_node_t const & dht_node,
    //                 std::uint16_t const app_port) const;

    xshared_node_entry_ptr_t
    make_node_entry(xdht_node_t const & dht_node) const;

    void
    make_node_active(common::xnode_id_t const & node_id, xnode_endpoint_t const & endpoint);

    void
    make_node_active(xshared_node_entry_ptr_t node_entry_ptr);

    ///////////////////////////////////////////////////////////////////////////
    // ping staff
    // ping is used:
    //  1) For discovering the target node id.
    //  2) Adding to known nodes table for the first time.
    //  3) When k-bucket is full, check the least seen node's liveness.
    ///////////////////////////////////////////////////////////////////////////

    /**
     * \brief Ping the node entry.
     * \param node_entry_ptr Ping the target node entry.
     *                       While we know the target node id, call this overload.
     * \param replacement_node_id Replacement node id is used when ping eviction
     *                            node.
     */
    void
    ping(xshared_node_entry_ptr_t const & node_entry_ptr,
         common::xnode_id_t const & replacement_node_id = {});

    ///////////////////////////////////////////////////////////////////////////
    // discover staff
    ///////////////////////////////////////////////////////////////////////////

    /**
     * \brief Launch a discover process.
     */
    void
    discover();

    struct xtop_discover_context
    {
        std::unordered_set<xshared_node_entry_ptr_t> tried_entries{};
        std::size_t iteration{ 0 };
    };
    using xdiscover_context_t = xtop_discover_context;

    void
    do_discover(common::xnode_id_t const & node_id,
                std::shared_ptr<xdiscover_context_t> discover_context = std::make_shared<xdiscover_context_t>());

    ///////////////////////////////////////////////////////////////////////////
    // eviction staff
    ///////////////////////////////////////////////////////////////////////////

    void
    do_eviction(xshared_node_entry_ptr_t const & eviction_candidate,
                xshared_node_entry_ptr_t const & new_node);

    void
    do_timeout_check();

    template <typename IteratorT>
    void
    drop_evictions(IteratorT begin, IteratorT end) {
        std::lock_guard<std::mutex> lock{ m_kbuckets_mutex };
        for (auto & it = begin; it != end; ++it)
        {
            auto node_entry_ptr = static_cast<xshared_node_entry_ptr_t>(*it);
            auto & kbucket = kbucket_unsafe(node_entry_ptr->distance());
            kbucket.nodes().remove_if([&node_entry_ptr](std::weak_ptr<xnode_entry_t> const & weak_entry_ptr)
            {
                return weak_entry_ptr.lock() == node_entry_ptr;
            });
        }
    }

    void
    drop_node(xshared_node_entry_ptr_t const & node_entry);

    ///////////////////////////////////////////////////////////////////////////
    // get operations
    ///////////////////////////////////////////////////////////////////////////

    /**
     * \brief Get the entry object from the known table.
     */
    xshared_node_entry_ptr_t
    get_node_entry(common::xnode_id_t const & node_id) const noexcept;

    bool
    known_node_unsafe(common::xnode_id_t const & node_id) const noexcept;

    xkbucket_t &
    kbucket_unsafe(xdistance_t const & distance) noexcept;

    xkbucket_t const &
    kbucket_unsafe(xdistance_t const & distance) const noexcept;

    ///////////////////////////////////////////////////////////////////////////
    // networking staff
    ///////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        typename std::enable_if
        <
            std::is_same<T, xping_node_t>::value ||
            std::is_same<T, xpong_t>::value      ||
            std::is_same<T, xfind_node_t>::value ||
            std::is_same<T, xneighbors_t>::value
        >::type * = nullptr
    >
    void
    send_to(xnode_endpoint_t const & ep, T const & discover_datagram) const {
        xdiscover_message_t const discover_message
        {
            top::codec::xmsgpack_codec_t<T>::encode(discover_datagram),
            m_host_node_id,
            std::is_same<T, xping_node_t>::value
                ? xdiscover_message_type_t::ping_node
                : std::is_same<T, xpong_t>::value
                ? xdiscover_message_type_t::pong
                : std::is_same<T, xfind_node_t>::value
                ? xdiscover_message_type_t::find_node
                : xdiscover_message_type_t::neighbors
        };

        assert(!discover_message.empty());

        send_to(ep, discover_message);
    }

    void
    send_to(xnode_endpoint_t const & endpoint, xdiscover_message_t const & discover_message) const;

    void
    send_to(xnode_endpoint_t const & endpoint, xbyte_buffer_t const & discover_message_bytes) const;

    void
    on_p2p_data_ready(xnode_endpoint_t const & sender,
                      xbyte_buffer_t const & bytes);
};

using xrouting_table_t = xtop_routing_table;
NS_END3
