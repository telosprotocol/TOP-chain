// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xbasic/xthreading/xthreadsafe_queue.hpp"
#include "xcommon/xnode_id.h"
// #include "xnetwork/xnetwork_driver_face.h"
#include "xelect_net/include/elect_vhost_face.h"
#include "xvnetwork/xbasic_vhost.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xmessage_filter_manager_face.h"
#include "xvnetwork/xvnetwork_message.h"

#include <memory>
#include <mutex>
#if defined DEBUG
#    include <thread>
#endif

NS_BEG2(top, vnetwork)

class xtop_vhost final
  : public xbasic_vhost_t
  , public std::enable_shared_from_this<xtop_vhost> {
private:
    using base_t = xbasic_vhost_t;

    constexpr static std::size_t max_message_queue_size{100000};
    threading::xthreadsafe_queue<xbyte_buffer_t, std::vector<xbyte_buffer_t>> m_message_queue{max_message_queue_size};
    
    std::unique_ptr<xmessage_filter_manager_face_t> m_filter_manager;

#if defined DEBUG
    std::thread::id m_vhost_thread_id{};
#endif
    observer_ptr<elect::xnetwork_driver_face_t> m_network_driver;
    // observer_ptr<network::xnetwork_driver_face_t> m_network_driver;

public:
    xtop_vhost(xtop_vhost const &) = delete;
    xtop_vhost & operator=(xtop_vhost const &) = delete;
    xtop_vhost(xtop_vhost &&) = default;
    xtop_vhost & operator=(xtop_vhost &&) = default;
    ~xtop_vhost() override = default;

    /**
     * @brief Construct the xvhost_t object.
     * @param network_driver The network driver object this vhost used to communicate with other vhosts on the network.
     * @param timer_driver The timer driver that this host used.
     * @param role The request role when the vhost launched or the physical node launched in other words.
     * @param nid The network id
     */
    xtop_vhost(observer_ptr<elect::xnetwork_driver_face_t> const & network_driver,
               observer_ptr<time::xchain_time_face_t> const & chain_timer,
               common::xnetwork_id_t const & nid,
               observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
               std::unique_ptr<xmessage_filter_manager_face_t> message_filter_manager_ptr = nullptr);

    void start() override;

    void stop() override;

    common::xnode_id_t const & host_node_id() const noexcept override final;

    // [Deprecated]
    void send(xmessage_t const & message,
              xvnode_address_t const & src,
              xvnode_address_t const & dst,
              network::xtransmission_property_t const & transmission_property = {}) override;

    //void broadcast(xmessage_t const & message, xvnode_address_t const & src) override;

    // void send(common::xnode_address_t const & src, common::xip2_t const & dst, xmessage_t const & message, std::error_code & ec) override;
    // void broadcast(common::xnode_address_t const & src, common::xip2_t const & dst, xmessage_t const & message, std::error_code & ec) override;

    void send_to_through_frozen(common::xnode_address_t const & src, common::xnode_address_t const & dst, xmessage_t const & message, std::error_code & ec) override;
    void send_to(common::xnode_address_t const & src, common::xnode_address_t const & dst, xmessage_t const & message, std::error_code & ec) override;
    void broadcast(common::xnode_address_t const & src, common::xnode_address_t const & dst, xmessage_t const & message, std::error_code & ec) override;

private:
    void on_network_data_ready(common::xaccount_address_t const & account_address, xbyte_buffer_t const & bytes);

    void do_handle_network_data();
};
using xvhost_t = xtop_vhost;

NS_END2
