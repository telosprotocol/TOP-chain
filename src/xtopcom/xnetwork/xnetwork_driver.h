// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash_list.hpp"
#include "xbasic/xthreading/xthreadsafe_queue.hpp"
#include "xbasic/xtimer_driver_fwd.h"
#include "xbasic/xmemory.hpp"
#include "xnetwork/xnetwork_message.h"
#include "xnetwork/xnetwork_driver_face.h"
#include "xnetwork/xnetwork_driver_fwd.h"
#include "xnetwork/xnetwork_message_ready_callback.h"
#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xdht_host_face.h"
//#include "xnetwork/xrrs/xrumor_manager.h"
#include "xnetwork/xsocket_face.h"


#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

NS_BEG2(top, network)

class xtop_network_driver final : public xnetwork_driver_face_t
                                , public std::enable_shared_from_this<xtop_network_driver>
{
private:
    threading::xthreadsafe_queue<std::pair<xnode_endpoint_t, xbyte_buffer_t>, std::vector<std::pair<xnode_endpoint_t, xbyte_buffer_t>>> m_data_queue{ 500000 };

    std::mutex m_callback_mutex{};
    xnetwork_message_ready_callback_t m_callback{};

    // std::uint64_t: message hash
    using message_cache_container_t = xhash_list_t<std::uint64_t, xhash_list_t<xnode_endpoint_t, std::time_t>>;

    mutable std::mutex m_message_cache_container_mutex{};
    std::unique_ptr<message_cache_container_t> m_message_cache_container{ top::make_unique<message_cache_container_t>() };
    static constexpr std::size_t m_message_cache_container_capacity_upper_limit{ 10000 };

    mutable std::mutex m_to_be_verified_peers_mutex{};
    mutable std::unordered_map<common::xnode_id_t, xnode_endpoint_t> m_to_be_verified_peers{};  // new coming nodes, cached for future verification.

    std::unique_ptr<p2p::xdht_host_face_t> m_dht_host;
    //std::shared_ptr<rrs::xrumor_manager_t> m_rumor_manager{};
    std::shared_ptr<xsocket_face_t> m_socket;

public:

    xtop_network_driver(xtop_network_driver const &)             = delete;
    xtop_network_driver & operator=(xtop_network_driver const &) = delete;
    xtop_network_driver(xtop_network_driver &&)                  = default;
    xtop_network_driver & operator=(xtop_network_driver &&)      = default;
    ~xtop_network_driver() override                              = default;

    xtop_network_driver(std::shared_ptr<xtimer_driver_t> timer_driver,
                        std::unique_ptr<p2p::xdht_host_face_t> dht_host,
                        std::shared_ptr<xsocket_face_t> app_socket);

    xtop_network_driver(std::shared_ptr<xtimer_driver_t> timer_driver,
                        std::unique_ptr<p2p::xdht_host_face_t> dht_host,
                        std::shared_ptr<xsocket_face_t> app_socket,
                        std::size_t const max_neighbor_count);

    void
    start() override;

    void
    stop() override;

    common::xnode_id_t const &
    host_node_id() const noexcept override;

    xnode_t
    host_node() const noexcept override;

    void
    send_to(common::xnode_id_t const & peer,
            xbyte_buffer_t const & message,
            xtransmission_property_t const & transmission_property) const override;

    void
    spread_rumor(xbyte_buffer_t const & rumor) const override;

    void
    register_message_ready_notify(xnetwork_message_ready_callback_t callback) noexcept override;

    void
    unregister_message_ready_notify() override;

    bool
    p2p_bootstrap(std::vector<xdht_node_t> const & seeds) const override;

    void
    direct_send_to(xnode_t const & to,
                   xbyte_buffer_t verification_data,
                   xtransmission_property_t const & transmission_property = {}) override;

    std::vector<common::xnode_id_t>
    neighbors() const override;

    std::size_t
    neighbor_size_upper_limit() const noexcept override;

    p2p::xdht_host_face_t const &
    dht_host() const noexcept override;

private:
    void
    on_socket_data_ready(xnode_endpoint_t const & sender_endpoint, xbyte_buffer_t const & bytes_message);

    void
    forward(xnetwork_message_t const & network_message) const;

    void
    send_to(xnode_endpoint_t const & peer,
            xbyte_buffer_t const & socket_message_payload,
            xdeliver_property_t const & deliver_property) const;

    void
    handle_socket_data();

    void
    do_handle_socket_data(xnode_endpoint_t const & sender_endpoint,
                          xbyte_buffer_t const & bytes_message);
};

NS_END2
