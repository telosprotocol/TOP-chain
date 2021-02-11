// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbasic/xcodec/xmsgpack/xsimple_message_codec.hpp"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xnetwork_message_codec.hpp"
#include "xnetwork/xnetwork_message.h"
#include "xnetwork/xnetwork_driver.h"
#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xdht_host.h"
#include "xnetwork/xudp_socket.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <functional>
#include <iterator>
#include <thread>

NS_BEG2(top, network)

xtop_network_driver::xtop_network_driver(std::shared_ptr<xtimer_driver_t> timer_driver,
                                         std::unique_ptr<p2p::xdht_host_face_t> dht,
                                         std::shared_ptr<xsocket_face_t> app_socket)
    : m_dht_host{ std::move(dht) }
    //, m_rumor_manager{ std::make_shared<rrs::xrumor_manager_t>(this, timer_driver) }
    , m_socket{ std::move(app_socket) }
{
}

xtop_network_driver::xtop_network_driver(std::shared_ptr<xtimer_driver_t> timer_driver,
                                         std::unique_ptr<p2p::xdht_host_face_t> dht,
                                         std::shared_ptr<xsocket_face_t> app_socket,
                                         std::size_t const max_neighbor_count)
    : m_dht_host{ std::move(dht) }
    //, m_rumor_manager{ std::make_shared<rrs::xrumor_manager_t>(this, timer_driver, max_neighbor_count) }
    , m_socket{ std::move(app_socket) }
{
}

void
xtop_network_driver::start() {
    assert(m_dht_host);
    assert(m_socket);

    m_socket->register_data_ready_notify(std::bind(&xtop_network_driver::on_socket_data_ready,
                                                   shared_from_this(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));

    m_dht_host->start();

    m_socket->start();

    //m_rumor_manager->start();

    assert(!running());
    running(true);
    assert(running());

    auto self = shared_from_this();
    threading::xbackend_thread::spawn([this, self] {
        handle_socket_data();
    });
}

void
xtop_network_driver::stop() {
    assert(running());
    running(false);
    assert(!running());

    //m_rumor_manager->stop();
    m_socket->stop();
    m_dht_host->stop();

    m_socket->unregister_data_ready_notify();
}

common::xnode_id_t const &
xtop_network_driver::host_node_id() const noexcept {
    assert(m_dht_host);
    return m_dht_host->host_node_id();
}

xnode_t
xtop_network_driver::host_node() const noexcept {
    return { host_node_id(), m_socket->local_endpoint() };
}

void
xtop_network_driver::send_to(common::xnode_id_t const & peer,
                             xbyte_buffer_t const & message,
                             xtransmission_property_t const & transmission_property) const {
    if (!running()) {
        xwarn("[network] network driver not run");
        return;
    }

    auto const & spread_property = transmission_property.spread_property;
    auto const & deliver_property = transmission_property.deliver_property;

    if (peer == host_node_id()) {
        // TODO needed?
        return;
    }

    assert(!(peer == host_node_id()));

    if (spread_property.spread_mode == xspread_mode_t::pt2pt &&
        spread_property.ttl_factor == 0) {
        xwarn("[network] pt2pt, but ttl is zero. discard");
        return;
    }

    auto normalized_spread_property = spread_property;
    if (normalized_spread_property.ttl_factor > ttl_factor_upper_bound) {
        normalized_spread_property.ttl_factor = ttl_factor_upper_bound;
    }

    xnetwork_message_t const network_message
    {
        xmessage_t{ message, xmessage_type_t::normal },
        m_dht_host->host_node_id(),
        peer,
        xtransmission_property_t{ deliver_property, normalized_spread_property }
    };

    auto const bytes_message = codec::msgpack_encode<xnetwork_message_t>(network_message);
    assert(!bytes_message.empty());

    XLOCK_GUARD(m_message_cache_container_mutex) {
       while (m_message_cache_container->size() > m_message_cache_container_capacity_upper_limit) {
           m_message_cache_container->pop_front();
       }

       auto hash = network_message.hash();
       auto & endpoints_hash_list = m_message_cache_container->at(hash,
                                                                  xhash_list_update_direction_t::to_back);
       while (endpoints_hash_list.size() >= m_message_cache_container_capacity_upper_limit) {
           endpoints_hash_list.pop_front();
       }
       endpoints_hash_list.push_back({ xnode_endpoint_t{}, std::time(nullptr) });
    }

    assert(m_dht_host);

    XLOCK_GUARD(m_to_be_verified_peers_mutex) {
        auto const it = m_to_be_verified_peers.find(peer);
        if (it != std::end(m_to_be_verified_peers)) {
            printf("%s send msg %" PRIx64 " to %s directly\n", host_node_id().to_string().c_str(), network_message.hash(), peer.to_string().c_str());
            send_to(top::get<xnode_endpoint_t>(*it), bytes_message, deliver_property);
            m_to_be_verified_peers.erase(it);

            return;
        }
    }

    xdbg("[network] %s send msg %" PRIx64 " to %s",
         host_node_id().to_string().c_str(),
         network_message.hash(),
         peer.to_string().c_str());

    auto peer_dht_entry = m_dht_host->node_entry(peer);
    if (peer_dht_entry != nullptr) {
        auto const remote_endpoint = xnode_endpoint_t{ peer_dht_entry->endpoint().address(), m_socket->local_endpoint().port() };
        xdbg("[network] %s send msg %" PRIx64 " to %s directly",
             host_node_id().to_string().c_str(),
             network_message.hash(),
             peer.to_string().c_str());
        send_to(remote_endpoint, bytes_message, deliver_property);
    } else {
        auto nearest_nodes = m_dht_host->nearest_node_entries(peer);

        for (auto const & node_entry : nearest_nodes) {
            auto const & dht_endpoint = node_entry->endpoint();
            auto const remote_endpoint = xnode_endpoint_t{ dht_endpoint.address(), m_socket->local_endpoint().port() };

            xdbg("%s rout msg %" PRIx64 " to %s:%hu",
                   host_node_id().value().c_str(),
                   network_message.hash(),
                   node_entry->id().value().c_str(),
                   remote_endpoint.port());

            send_to(remote_endpoint, bytes_message, deliver_property);
        }
    }
}

void
xtop_network_driver::spread_rumor(xbyte_buffer_t const & rumor) const {
    if (!running()) {
        xwarn("[network] network driver not run");
        return;
    }

    // assert(m_rumor_manager);
    // m_rumor_manager->add_rumor(rumor);
}

void
xtop_network_driver::register_message_ready_notify(xnetwork_message_ready_callback_t callback) noexcept {
    XLOCK_GUARD(m_callback_mutex) {
        assert(m_callback == nullptr);
        m_callback = std::move(callback);
    }
}

void
xtop_network_driver::unregister_message_ready_notify() {
    XLOCK_GUARD(m_callback_mutex) {
        assert(m_callback != nullptr);
        m_callback = nullptr;
    }
}

bool
xtop_network_driver::p2p_bootstrap(std::vector<xdht_node_t> const & seeds) const {
    assert(running());
    assert(m_dht_host);
    m_dht_host->bootstrap(seeds);
    return true;
    // auto result = m_dht_host->bootstrap(seeds);
    // std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    // bool successful = false;
    // for (auto & r : result) {
    //     if (successful) {
    //         continue;
    //     }

    //     if (!successful) {
    //         try {
    //             successful = r.get();
    //         } catch (std::future_error const &) {
    //             xwarn("[network driver] p2p bootstrap may be failed");
    //         }
    //     }
    // }

    // return successful;
}

void
xtop_network_driver::direct_send_to(xnode_t const & to,
                                    xbyte_buffer_t verification_data,
                                    xtransmission_property_t const & transmission_property) {
    if (!running()) {
        xwarn("[network] network driver not run");
        return;
    }

    assert(m_socket);

    xnetwork_message_t const network_message
    {
        xmessage_t{ std::move(verification_data), xmessage_type_t::direct },
        m_dht_host->host_node_id(),
        to.id(),
        transmission_property
    };

    auto bytes_message = codec::msgpack_encode<xnetwork_message_t>(network_message);
    assert(!bytes_message.empty());

    send_to(to.endpoint(), std::move(bytes_message), transmission_property.deliver_property);
}

std::vector<common::xnode_id_t>
xtop_network_driver::neighbors() const {
    assert(m_dht_host);
    auto const & all_dht_node_entries = m_dht_host->all_node_entries();

    std::vector<common::xnode_id_t> ret;
    ret.reserve(all_dht_node_entries.size());

    std::transform(std::begin(all_dht_node_entries),
                   std::end(all_dht_node_entries),
                   std::back_inserter(ret),
                   [](std::shared_ptr<p2p::xnode_entry_t> const & entry_ptr) {
        return entry_ptr->id();
    });

    return ret;
}

std::size_t
xtop_network_driver::neighbor_size_upper_limit() const noexcept {
    assert(m_dht_host);
    return m_dht_host->neighbor_size_upper_limit();
}

p2p::xdht_host_face_t const &
xtop_network_driver::dht_host() const noexcept {
    return *m_dht_host;
}

void
xtop_network_driver::on_socket_data_ready(xnode_endpoint_t const & sender_endpoint,
                                          xbyte_buffer_t const & bytes_message) {
    if (!running()) {
        xwarn("[network] network driver not run");
        return;
    }

    m_data_queue.push({ sender_endpoint, bytes_message });
    // do_handle_socket_data(sender_endpoint, bytes_message);
}

void
xtop_network_driver::handle_socket_data() {
    while (running()) {
        try {
            auto data = m_data_queue.wait_and_pop_all();

            if (m_callback == nullptr) {
                xwarn("[network] callback not registered");
                continue;
            }

            for (auto const & datum : data) {
                auto const & sender_endpoint = top::get<xnode_endpoint_t>(datum);
                auto const & bytes_message = top::get<xbyte_buffer_t>(datum);

                do_handle_socket_data(sender_endpoint, bytes_message);
            }
        } catch (std::exception const & eh) {
            xwarn("[network] caught exception: %s", eh.what());
        }
    }
}

void
xtop_network_driver::do_handle_socket_data(xnode_endpoint_t const & sender_endpoint,
                                           xbyte_buffer_t const & bytes_message) {
    auto network_message = codec::msgpack_decode<xnetwork_message_t>(bytes_message);
    if (network_message.empty()) {
        xerror("[network] decode bytes to xnetwork_message_t failed");
        assert(false);
        return;;
    }

    if (network_message.message().id() == xmessage_type_t::invalid) {
        xerror("[network] invalid network message id");
        assert(false);
        return;;
    }

    auto & transmission_property = network_message.transmission_property();
    auto & spread_property = transmission_property.spread_property;

    auto & ttl_factor = spread_property.ttl_factor;
    if (ttl_factor == 0) {
        xerror("[network] received a message with ttl == 0");
        assert(false);
        return;
    }

    --ttl_factor;

    switch (spread_property.spread_mode) {
        case xspread_mode_t::pt2pt: {
            auto cached_just_now = false;
            XLOCK_GUARD(m_message_cache_container_mutex) {
                while (m_message_cache_container->size() > m_message_cache_container_capacity_upper_limit) {
                    m_message_cache_container->pop_front();
                }

                auto const hash = network_message.hash();
                xdbg("[network] %s recv msg %" PRIx64 " sent to %s", host_node_id().to_string().c_str(), hash, network_message.receiver_id().to_string().c_str());


                auto & endpoints_hash_list = m_message_cache_container->at(hash,
                                                                           xhash_list_update_direction_t::to_back);
                while (endpoints_hash_list.size() >= m_message_cache_container_capacity_upper_limit) {
                    endpoints_hash_list.pop_front();
                }
                endpoints_hash_list.push_back({ sender_endpoint, std::time(nullptr) });

                auto const cached_times = endpoints_hash_list.size();
                cached_just_now = cached_times == 1u;
            }

            auto const hash = network_message.hash();
            if (cached_just_now) {  // received for the first time.
                assert(m_dht_host);
                assert(m_callback);
                assert(spread_property.spread_mode == xspread_mode_t::pt2pt);

                auto const & recv_id = network_message.receiver_id();
                auto const & host_id = m_dht_host->host_node_id();
                if (recv_id == host_id) {
                    //static std::unordered_set<std::uint64_t> received_msg;
                    //if (received_msg.find(hash) != std::end(received_msg)) {
                    //    xdbg("[network] %s received a cached msg %" PRIx64,
                    //         host_id.to_string().c_str(),
                    //         hash);
                    //    // std::printf("[network] %s received a cached msg %" PRIx64 "\n",
                    //    //             host_id.to_string().c_str(),
                    //    //             hash);
                    //    // std::fflush(stdout);
                    //    break;
                    //}
                    //received_msg.insert(hash);

                    xdbg("[network] %s handle msg %" PRIx64 " sent to %s", host_node_id().to_string().c_str(), hash, network_message.receiver_id().to_string().c_str());
                    assert(m_callback);

                    if (network_message.message().type() == xmessage_type_t::direct) {
                        XLOCK_GUARD(m_to_be_verified_peers_mutex) {
                            m_to_be_verified_peers.insert({ network_message.sender_id(), sender_endpoint });
                        }
                    }

                    xdbg("[network] push msg %" PRIx64 " to virtual network", hash);
                    m_callback(network_message.sender_id(), network_message.message().payload());
                } else {
                    if (spread_property.ttl_factor > 0) {
                        forward(network_message);
                    } else {
                        xwarn("[network] %s receives msg %" PRIx64 " sent to %s, ttl reaches zero", host_node_id().to_string().c_str(), hash, recv_id.to_string().c_str());
                    }
                }
            } else {
                xkinfo("[network] %s received a cache msg %" PRIx64 " sent to %s",
                       host_node_id().to_string().c_str(),
                       hash,
                       network_message.receiver_id().to_string().c_str());
            }

            break;
        }

        case xspread_mode_t::broadcast: {
            if (network_message.message().type() != xmessage_type_t::normal) {
                xerror("[network] broadcast message but type is wrong");
                assert(false);
                break;
            }

            std::string const payload = ""; //= m_rumor_manager->rumor_received(network_message.sender_id().hash(), network_message.message().payload());
            if (!payload.empty()) {
                xdbg("[network] broadcast msg %" PRIx64 " pushed to virtual network", network_message.hash());
                //m_callback(network_message.sender_id(), payload);
            }

            xdbg("[network] host %s received a rumor %" PRIx64 " from %s",
                 host_node_id().to_string().c_str(),
                 network_message.message().hash(),
                 network_message.sender_id().to_string().c_str());

            break;
        }

        default:
            assert(false);
            // todo log error
            break;
    }
}


void
xtop_network_driver::forward(xnetwork_message_t const & network_message) const {
    if (!running()) {
        xwarn("[network] network driver not run");
        return;
    }

    assert(network_message.transmission_property().spread_property.ttl_factor > 0);
    assert(!network_message.empty());

    auto const socket_message_payload = codec::msgpack_encode<xnetwork_message_t>(network_message);
    assert(!socket_message_payload.empty());

    std::unordered_set<xnode_endpoint_t> excludes{};
    XLOCK_GUARD(m_message_cache_container_mutex) {
        while (m_message_cache_container->size() > m_message_cache_container_capacity_upper_limit) {
            m_message_cache_container->pop_front();
        }

        auto & endpoints_hash_list = m_message_cache_container->at(network_message.hash(),
                                                                   xhash_list_update_direction_t::to_back);

        excludes.reserve(endpoints_hash_list.size());
        for (auto const & endpoint_info : endpoints_hash_list) {
            excludes.insert(top::get<xnode_endpoint_t const>(endpoint_info));
        }

        while (endpoints_hash_list.size() >= m_message_cache_container_capacity_upper_limit) {
            endpoints_hash_list.pop_front();
        }
        endpoints_hash_list.push_back({ xnode_endpoint_t{}, std::time(nullptr) });
    }

    auto const hash = network_message.hash();
    auto const & peer = network_message.receiver_id();
    auto const bytes_message = codec::msgpack_encode<xnetwork_message_t>(network_message);

    assert(m_dht_host);

    auto peer_dht_entry = m_dht_host->node_entry(peer);
    if (peer_dht_entry != nullptr) {
        auto const remote_endpoint = xnode_endpoint_t{ peer_dht_entry->endpoint().address(), m_socket->local_endpoint().port() };
        send_to(remote_endpoint, bytes_message, network_message.transmission_property().deliver_property);
    } else {
        auto nearest_nodes = m_dht_host->nearest_node_entries(network_message.receiver_id());

        if (nearest_nodes.empty()) {
            xdbg("[network] forwarding msg %" PRIx64 " failed. host %s has no neighbor near to %s", hash, host_node_id().to_string().c_str(), network_message.receiver_id().to_string().c_str());
        }

        for (auto const & node_entry : nearest_nodes) {
            auto const & dht_endpoint = node_entry->endpoint();
            auto remote_endpoint = xnode_endpoint_t{ dht_endpoint.address(), m_socket->local_endpoint().port() };

            if (excludes.find(remote_endpoint) != std::end(excludes)) {
                continue;
            }

            xdbg("[network] host %s forwarding msg %" PRIx64 " to %s:%hu", host_node_id().to_string().c_str(), hash, node_entry->id().value().c_str(), m_socket->local_endpoint().port());

            send_to(remote_endpoint, bytes_message, network_message.transmission_property().deliver_property);
            xdbg("[network] host %s forwarded msg %" PRIx64 " to socket layer", host_node_id().to_string().c_str(), hash);
        }
    }
}

void
xtop_network_driver::send_to(xnode_endpoint_t const & peer,
                             xbyte_buffer_t const & socket_message_payload,
                             xdeliver_property_t const & deliver_property) const {
    assert(m_socket);
    assert(running());
    m_socket->send_to(peer, socket_message_payload, deliver_property);
}

NS_END2
