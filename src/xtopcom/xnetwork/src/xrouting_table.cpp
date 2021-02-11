// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xuint.hpp"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xdiscover_datagram_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xdiscover_message_codec.hpp"
#include "xnetwork/xp2p/xrouting_table.h"
#include "xutility/xhash.h"

#include <algorithm>
#include <cassert>
#include <exception>
#include <map>
#include <random>

NS_BEG3(top, network, p2p)

constexpr std::chrono::milliseconds xtop_routing_table::m_buckets_refresh_interval;
constexpr std::chrono::milliseconds xtop_routing_table::m_buckets_discover_interval;
constexpr std::chrono::milliseconds xtop_routing_table::m_request_timout;
constexpr std::chrono::milliseconds xtop_routing_table::m_eviction_check_interval;
constexpr std::chrono::milliseconds xtop_routing_table::m_eviction_timeout;
constexpr std::chrono::milliseconds xtop_routing_table::m_node_id_discover_pings_check_interval;

xtop_routing_table::xtop_routing_table(common::xnode_id_t id,
                                       std::shared_ptr<xsocket_face_t> socket,
                                       observer_ptr<xtimer_driver_t> timer_driver)
    : m_timer_driver{ timer_driver }
    , m_host_node_id{ std::move(id) }, m_socket{ std::move(socket) }
{
    assert(m_socket);
    assert(timer_driver);
}

void
xtop_routing_table::start() {
    // start from bottom to top

    assert(m_socket);
    m_socket->register_data_ready_notify(std::bind(&xtop_routing_table::on_p2p_data_ready,
                                                   shared_from_this(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));

    m_socket->start();

    assert(!running());
    running(true);
    assert(running());

    discover();
    do_timeout_check();
}

void
xtop_routing_table::stop() {
    assert(running());
    running(false);
    assert(!running());

    m_socket->stop();
}

void
xtop_routing_table::add_node(xdht_node_t const & dht_node) {
    if (dht_node.empty()) {
        assert(false);
        // todo log error
        return;
    }

    {
        std::lock_guard<std::mutex> lock{ m_all_known_nodes_mutex };
        auto const it = m_all_known_nodes.find(dht_node.id());
        if (it != std::end(m_all_known_nodes)) {
            return;
        }
    }

    // prevent adding itself into _known_ table
    if (dht_node.id() == m_host_node_id) {
        return;
    }

    // first seen, add to known table and ping it.
    auto entry = make_node_entry(dht_node);
    XLOCK_GUARD(m_all_known_nodes_mutex) {
        m_all_known_nodes.insert({ dht_node.id(), entry });
    }

    ping(entry);
}

void
xtop_routing_table::ping(xshared_node_entry_ptr_t const & node_entry_ptr,
                         common::xnode_id_t const & replacement_node_id) {
    assert(node_entry_ptr);
    assert(!node_entry_ptr->empty());
    assert(!node_entry_ptr->id().empty());

    XLOCK_GUARD(m_sent_pings_mutex) {
        auto const it = m_sent_pings.find(node_entry_ptr->id());
        if (it != std::end(m_sent_pings)) {
            return;
        }
    }

    xdiscover_message_t const ping_message{
        codec::msgpack_encode(xping_node_t{ node_entry_ptr->dht_node() }),
        m_host_node_id,
        xdiscover_message_type_t::ping_node
    };
    send_to(node_entry_ptr->endpoint(), ping_message);

    xhash256_t digest{};
    assert(digest.size() == 32);
    uint256_t hash  = utl::xsha3_256_t::digest(ping_message.payload().data(), ping_message.payload().size());
    memcpy(digest.data(), hash.data(), hash.size());

    XLOCK_GUARD(m_sent_pings_mutex) {
        m_sent_pings[node_entry_ptr->id()] = xping_context_t{
            std::move(digest),
            xtime_point_t::clock::now(),
            replacement_node_id,
            // xping_result_promise_t{}
        };
    }
}

xping_result_future_t
xtop_routing_table::ping(xdht_node_t const & remote_dht_node) {
    assert(!remote_dht_node.id().empty());

    xdiscover_message_t const ping_message{
        codec::msgpack_encode(xping_node_t{ remote_dht_node }),
        m_host_node_id,
        xdiscover_message_type_t::ping_node
    };
    send_to(remote_dht_node.endpoint(), ping_message);

    xhash256_t digest{};
    assert(digest.size() == 32);
    uint256_t hash  = utl::xsha3_256_t::digest(ping_message.payload().data(), ping_message.payload().size());
    memcpy(digest.data(), hash.data(), hash.size());

    std::lock_guard<std::mutex> lock{ m_sent_pings_mutex };
    // assert(m_sent_pings.find(remote_dht_node.id()) == std::end(m_sent_pings)); // TODO: assert is seen!!

    m_sent_pings[remote_dht_node.id()] = xping_context_t{
        std::move(digest),
        xtime_point_t::clock::now(),
        common::xnode_id_t{},
        // xping_result_promise_t{}
    };

    // return m_sent_pings[remote_dht_node.id()].ping_result_promise.get_future();
}

void
xtop_routing_table::discover() {
    if (!running()) {
        return;
    }

    assert(m_timer_driver != nullptr);

    auto self = shared_from_this();
    m_timer_driver->schedule(m_buckets_refresh_interval, [this, self](std::error_code const & ec) {
        if (ec && ec == std::errc::operation_canceled) {
            xwarn("[routing table] timer driver stopped");
            return;
        }

        if (!running()) {
            return;
        }

        common::xnode_id_t random_node;
        random_node.random();
        do_discover(random_node);
    });
}

void
xtop_routing_table::do_discover(common::xnode_id_t const & id,
                                std::shared_ptr<xdiscover_context_t> discover_context) {
    assert(discover_context);

    if (discover_context->iteration == m_max_discover_iteration) {
        discover();
        return;
    }

    if (!running()) {
        xkinfo("[routing table] do_discover: routing table doesn't run");
        return;
    }

    xdiscover_message_t const discover_message{ codec::msgpack_encode(xfind_node_t{ id }), m_host_node_id, xdiscover_message_type_t::find_node };
    auto const bytes_message = codec::msgpack_encode(discover_message);
    assert(!bytes_message.empty());

    auto nearest_nodes = nearest_node_entries(id);
    //std::printf("host %s nearest_node_entries size %zu\n", host_node_id().value().c_str(), nearest_nodes.size());
    //std::fflush(stdout);

    std::unordered_set<xshared_node_entry_ptr_t> tried{};
    for (auto const & entry : nearest_nodes) {
        if (tried.size() >= m_alpha) {
            break;
        }

        if (!discover_context->tried_entries.count(entry)) {
            tried.insert(entry);
            {
                std::lock_guard<std::mutex> lock{ m_find_node_timeouts_mutex };
                m_find_node_timeouts.push_back({ entry->id(), xtime_point_t::clock::now() });
            }

            send_to(entry->endpoint(), bytes_message);
        }
    }

    if (tried.empty()) {
        discover();
        return;
    }

    discover_context->tried_entries.insert(std::begin(tried), std::end(tried));
    tried.clear();

    assert(m_timer_driver != nullptr);

    auto self = shared_from_this();
    m_timer_driver->schedule(m_buckets_discover_interval,
                             [this, self, id, discover_context](std::error_code const & ec) {
        if (ec && ec == std::errc::operation_canceled) {
            return;
        }

        if (!running()) {
            xkinfo("[routing table] do_discover: routing table doesn't run");
            return;
        }

        do_discover(id, discover_context);
    });
}

std::vector<xtop_routing_table::xshared_node_entry_ptr_t>
xtop_routing_table::nearest_node_entries(common::xnode_id_t const & target) const {
    std::map<std::size_t, std::list<xshared_node_entry_ptr_t>> found;

    XLOCK_GUARD(m_kbuckets_mutex) {
        for (auto const & kbucket : m_kbuckets) {
            for (auto const & weak_node_ptr : kbucket.nodes()) {
                auto shared_node_ptr = weak_node_ptr.lock();
                if (shared_node_ptr) {
                    found[static_cast<std::size_t>(make_distance(shared_node_ptr->id(), target))].push_back(shared_node_ptr);
                }
            }
        }
    }

    std::vector<xshared_node_entry_ptr_t> ret;
    ret.reserve(m_k);

    for (auto const & nodes : found) {
        for (auto const & node : top::get<std::list<xshared_node_entry_ptr_t>>(nodes)) {
            if (ret.size() >= m_k) {
                goto quit_loop;
            }

            if (node->empty()) {
                continue;
            }

            ret.push_back(node);
        }
    }

quit_loop:

    return ret;
}

common::xnode_id_t const &
xtop_routing_table::host_node_id() const noexcept {
    return m_host_node_id;
}

xdht_node_t
xtop_routing_table::host_dht_node() const {
    assert(!m_host_node_id.empty());
    assert(m_socket);
    assert(!m_socket->local_endpoint().empty());

    return xdht_node_t{ m_host_node_id, m_socket->local_endpoint() };
}


std::unordered_map<common::xnode_id_t, xtop_routing_table::xshared_node_entry_ptr_t>
xtop_routing_table::all_known_nodes() const {
    std::lock_guard<std::mutex> lock{ m_all_known_nodes_mutex };
    return m_all_known_nodes;
}

void
xtop_routing_table::on_p2p_data_ready(xnode_endpoint_t const & sender,
                                      xbyte_buffer_t const & bytes) {
    auto discover_message = codec::msgpack_decode<xdiscover_message_t>(bytes);
    if (discover_message.empty()) {
        return;
    }

    if (discover_message.expired()) {
        assert(false);
        return;
    }

    switch (discover_message.type()) {
        case xdiscover_message_type_t::ping_node: {
            auto ping_node = codec::msgpack_decode<xping_node_t>(discover_message.payload());
            if (ping_node.empty()) {
                xerror("[routing table] received an empty ping message");
                assert(false);
                break;
            }

            xhash256_t digest{};
            assert(digest.size() == 32);
            uint256_t hash = utl::xsha3_256_t::digest(discover_message.payload().data(), discover_message.payload().size());
            memcpy(digest.data(), hash.data(), hash.size());

            // add_node(xdht_node_t{ discover_message.sender_node_id(), sender }, ping_node.app_port());
            add_node(xdht_node_t{ discover_message.sender_node_id(), sender });

            send_to(sender, xpong_t{ sender, std::move(digest) });

            break;
        }

        case xdiscover_message_type_t::pong:
        {
            auto pong = codec::msgpack_decode<xpong_t>(discover_message.payload());
            if (pong.empty()) {
                xerror("[routing table] received an empty pong message");
                return;
            }

            XLOCK_GUARD(m_sent_pings_mutex) {
                auto const it = m_sent_pings.find(discover_message.sender_node_id());

                // validating

                if (it == std::end(m_sent_pings)) {
                    xwarn("[routing table] unknown pong, doesn't have a matched ping");
                    return;
                }

                auto & ping_context = top::get<xping_context_t>(*it);
                if (pong.ping_hash() != ping_context.ping_hash) {
                    xwarn("[routing table] host %s received pong from %s with ping hash mismatched.  discard",
                          host_node_id().to_string().c_str(),
                          discover_message.sender_node_id().to_string().c_str());
                    return;
                }

                // ping_context.ping_result_promise.set_value(true);

                auto const & replacement_node_id = ping_context.replacement_node_id;
                if (!replacement_node_id.empty()) {
                    auto replacement_node_entry = get_node_entry(replacement_node_id);
                    if (replacement_node_entry) {
                        drop_node(replacement_node_entry);
                    }
                }

                XLOCK_GUARD(m_all_known_nodes_mutex) {
                    auto const it2 = m_all_known_nodes.find(discover_message.sender_node_id());
                    if (it2 == std::end(m_all_known_nodes)) {
                        xdht_node_t peer{ discover_message.sender_node_id(), sender };
                        auto entry = make_node_entry(peer);
                        m_all_known_nodes.insert({ entry->id(), entry });
                    }
                }

                m_sent_pings.erase(it);
            }

            XLOCK_GUARD(m_host_node_endpoint_mutex) {
                if (m_host_node_endpoint.empty()) {
                    m_host_node_endpoint = pong.ping_endpoint();
                }
            }

            break;
        }

        case xdiscover_message_type_t::find_node:
        {
            auto find_node = codec::msgpack_decode<xfind_node_t>(discover_message.payload());
            if (find_node.empty()) {
                xerror("[routing table] received an empty find node message");
                return;
            }

            auto nearest_nodes = nearest_node_entries(find_node.target_id());
            std::unordered_set<xdht_node_t> neighbors;
            for (auto const & node : nearest_nodes) {
                neighbors.insert(xdht_node_t{ node->id(), node->endpoint() });
            }

            send_to(sender, xneighbors_t{ std::move(neighbors) });

            break;
        }

        case xdiscover_message_type_t::neighbors:
        {
            auto const neighbors = codec::msgpack_decode<xneighbors_t>(discover_message.payload());
            if (neighbors.empty()) {
                xwarn("[routing table] received an empty neighbors message");
                return;
            }

            auto now = xtime_point_t::clock::now();
            auto expected = false;
            {
                std::lock_guard<std::mutex> lock{ m_find_node_timeouts_mutex };
                auto self = shared_from_this();
                m_find_node_timeouts.remove_if([self, &discover_message, &expected, &now](xnode_id_time_point_t const & nitp) {
                    if (nitp.node_id != discover_message.sender_node_id()) {
                        return false;
                    }

                    expected = now - nitp.time_point < m_request_timout;
                    return true;
                });
            }

            if (!expected) {
                break;
            }

            for (auto const & neighbor : neighbors.neighbors()) {
                add_node(neighbor);
            }

            break;
        }

        default:
            assert(false);
            break;
    }

    make_node_active(discover_message.sender_node_id(), sender);
}

void
xtop_routing_table::send_to(xnode_endpoint_t const & ep, xdiscover_message_t const & discover_message) const {
    assert(!discover_message.empty());
    assert(!codec::msgpack_encode(discover_message).empty());

    send_to(ep, codec::msgpack_encode(discover_message));
}

void
xtop_routing_table::send_to(xnode_endpoint_t const & peer, xbyte_buffer_t const & discover_message_bytes) const {
    if (discover_message_bytes.empty()) {
        assert(false);
        return;
    }

    assert(m_socket);
    m_socket->send_to(peer, discover_message_bytes, xdeliver_property_t{});
}

common::xnode_id_t
xtop_routing_table::node_id(xnode_endpoint_t const & ep) const {
    XLOCK_GUARD(m_host_node_endpoint_mutex) {
        if (m_host_node_endpoint == ep) {
            return m_host_node_id;
        }
    }

    XLOCK_GUARD(m_all_known_nodes_mutex) {
        auto const it = std::find_if(std::begin(m_all_known_nodes),
                                     std::end(m_all_known_nodes),
                                     [&ep](std::pair<common::xnode_id_t const, xshared_node_entry_ptr_t> const & node_info) {
            return top::get<xshared_node_entry_ptr_t>(node_info)->endpoint() == ep;
        });

        if (it != std::end(m_all_known_nodes)) {
            return top::get<common::xnode_id_t const>(*it);
        }
    }

    return {};
}

xnode_endpoint_t
xtop_routing_table::endpoint(common::xnode_id_t const & nid) const {
    if (nid == m_host_node_id) {
        XLOCK_GUARD(m_host_node_endpoint_mutex) {
            return m_host_node_endpoint;
        }
    }

    {
        XLOCK_GUARD(m_all_known_nodes_mutex) {
            auto const it = m_all_known_nodes.find(nid);

            if (it != std::end(m_all_known_nodes)) {
                return top::get<xshared_node_entry_ptr_t>(*it)->endpoint();
            }
        }
    }

    return {};
}

std::shared_ptr<xnode_entry_t>
xtop_routing_table::node_entry(common::xnode_id_t const & nid) const {
    if (nid == m_host_node_id) {
        assert(false);
        // todo log error
        // todo throw invalid argument
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock{ m_all_known_nodes_mutex };
        auto const it = m_all_known_nodes.find(nid);

        if (it != std::end(m_all_known_nodes)) {
            return top::get<xshared_node_entry_ptr_t>(*it);
        }
    }

    return {};
}

xdistance_t
xtop_routing_table::make_distance(common::xnode_id_t const & left, common::xnode_id_t const & right) noexcept {
    if (left == right) {
        return xdistance_t{ 0 };
    }

    xhash256_t lhash;
    xhash256_t rhash;

    uint256_t hash  = utl::xsha3_256_t::digest(left.value().data(), left.value().size());
    memcpy(lhash.data(), hash.data(), hash.size());

    hash  = utl::xsha3_256_t::digest(right.value().data(), right.value().size());
    memcpy(rhash.data(), hash.data(), hash.size());

    std::size_t distance{ 0 };
    xuint256_t result{ lhash ^ rhash };
    while (result) {
        distance++;
        result >>= 1;
    }

    return xdistance_t{ distance };
}

// xtop_routing_table::xshared_node_entry_ptr_t
// xtop_routing_table::make_node_entry(xdht_node_t const & dht_node,
//                                     std::uint16_t const app_port) const {
//     assert(!dht_node.empty());
//     assert(!known_node_unsafe(dht_node.id()));

//     return std::make_shared<xnode_entry_t>(make_distance(m_host_node_id, dht_node.id()),
//                                            dht_node,
//                                            app_port);
// }

xtop_routing_table::xshared_node_entry_ptr_t
xtop_routing_table::make_node_entry(xdht_node_t const & dht_node) const {
    assert(!dht_node.empty());
    assert(!known_node_unsafe(dht_node.id()));

    return std::make_shared<xnode_entry_t>(make_distance(m_host_node_id, dht_node.id()),
                                           dht_node);
}

xtop_routing_table::xshared_node_entry_ptr_t
xtop_routing_table::get_node_entry(common::xnode_id_t const & nid) const noexcept {
    std::lock_guard<std::mutex> lock{ m_all_known_nodes_mutex };
    auto const it = m_all_known_nodes.find(nid);
    if (it != std::end(m_all_known_nodes)) {
        return top::get<xshared_node_entry_ptr_t>(*it);
    }

    return nullptr;
}

bool
xtop_routing_table::known_node_unsafe(common::xnode_id_t const & nid) const noexcept {
    return m_all_known_nodes.find(nid) != std::end(m_all_known_nodes);
}

void
xtop_routing_table::make_node_active(common::xnode_id_t const & nid,
                                     xnode_endpoint_t const &) {
    if (m_host_node_id == nid) {
        return;
    }

    auto const shared_node_entry_ptr = get_node_entry(nid);
    if (shared_node_entry_ptr) {
        make_node_active(std::move(shared_node_entry_ptr));
    } else {
        xwarn("[dht] node entry not found for node %s", nid.to_string().c_str());
    }
}

void
xtop_routing_table::make_node_active(xshared_node_entry_ptr_t shared_node_entry_ptr) {
    assert(shared_node_entry_ptr);
    assert(m_host_node_id != shared_node_entry_ptr->id());

    XLOCK_GUARD(m_kbuckets_mutex) {
        auto & kbucket = kbucket_unsafe(shared_node_entry_ptr->distance());
        auto & nodes = kbucket.nodes();

        auto const it = std::find_if(std::begin(nodes),
                                     std::end(nodes),
                                     [&shared_node_entry_ptr](std::weak_ptr<xnode_entry_t> const & node_entry_weak_ptr) {
            return !shared_node_entry_ptr.owner_before(node_entry_weak_ptr) &&
                !node_entry_weak_ptr.owner_before(shared_node_entry_ptr);
        });
        if (it != std::end(nodes)) {
            nodes.splice(std::end(nodes), nodes, it);
        } else {
            if (nodes.size() < xkbucket_t::k()) {
                nodes.push_back(shared_node_entry_ptr);
            } else {
                auto const eviction_candiate = nodes.front().lock();
                if (eviction_candiate == nullptr) {
                    nodes.pop_front();
                    nodes.push_back(shared_node_entry_ptr);
                } else {
                    do_eviction(eviction_candiate, shared_node_entry_ptr);
                }
            }
        }
    }
}

void
xtop_routing_table::do_eviction(xshared_node_entry_ptr_t const & least_seen_node,
                                xshared_node_entry_ptr_t const & new_node) {
    ping(least_seen_node, new_node->id());
}

static constexpr std::chrono::seconds timeout_check_interval{ 5 };
static constexpr std::chrono::milliseconds ping_ttl{ 30000 };
void
xtop_routing_table::do_timeout_check() {
    if (!running()) {
        return;
    }

    assert(m_timer_driver != nullptr);

    auto self = shared_from_this();
    m_timer_driver->schedule(std::chrono::duration_cast<std::chrono::milliseconds>(timeout_check_interval),
                             [this, self](std::error_code const & ec) {
        std::vector<xshared_node_entry_ptr_t> drop;

        if (ec && ec == std::errc::operation_canceled) {
            xwarn("[routing table] do_timeout_check: timer driver not run");
            return;
        }

        if (!running()) {
            return;
        }

        std::vector<xshared_node_entry_ptr_t> nodes_to_active;
        for (auto it = std::begin(m_sent_pings); it != std::end(m_sent_pings);) {
            auto const & ping_ctx = top::get<xping_context_t>(*it);
            if (xtime_point_t::clock::now() > ping_ctx.ping_send_time + ping_ttl) {
                // ping request timeout
                if (auto node = get_node_entry(top::get<common::xnode_id_t const>(*it))) {
                    drop_node(std::move(node));
                }

                if (!ping_ctx.replacement_node_id.empty()) {
                    auto node = get_node_entry(ping_ctx.replacement_node_id);
                    if (node) {
                        nodes_to_active.push_back(std::move(node));
                    }
                }

                it = m_sent_pings.erase(it);
            } else {
                ++it;
            }
        }

        for (auto const & entry : nodes_to_active) {
            make_node_active(entry->id(), entry->endpoint());
        }

        do_timeout_check();
    });
}

void
xtop_routing_table::drop_node(xshared_node_entry_ptr_t const & entry) {
    XLOCK_GUARD(m_kbuckets_mutex) {
        auto & kbucket = kbucket_unsafe(entry->distance());
        kbucket.nodes().remove_if([&entry](std::weak_ptr<xnode_entry_t> const & weak_entry_ptr) {
            return weak_entry_ptr.lock() == entry;
        });
    }

    XLOCK_GUARD(m_all_known_nodes_mutex) {
        m_all_known_nodes.erase(entry->id());
    }
}

xtop_routing_table::xkbucket_t &
xtop_routing_table::kbucket_unsafe(xdistance_t const & distance) noexcept {
    assert(static_cast<std::size_t>(distance) > 0 && static_cast<std::size_t>(distance) <= m_address_bit_size);
    return m_kbuckets[static_cast<std::size_t>(distance) - 1];
}

xtop_routing_table::xkbucket_t const &
xtop_routing_table::kbucket_unsafe(xdistance_t const & distance) const noexcept {
    assert(static_cast<std::size_t>(distance) > 0 && static_cast<std::size_t>(distance) <= m_address_bit_size);
    return m_kbuckets[static_cast<std::size_t>(distance) - 1];
}

NS_END3
