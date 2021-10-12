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
}

void
xtop_network_driver::spread_rumor(xbyte_buffer_t const & rumor) const {
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
}


void
xtop_network_driver::forward(xnetwork_message_t const & network_message) const {
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
