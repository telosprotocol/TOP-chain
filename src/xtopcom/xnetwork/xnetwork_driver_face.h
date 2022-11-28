// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xrunnable.h"
#include "xnetwork/xnetwork_message_ready_callback.h"
#include "xnetwork/xnode.h"
#include "xcommon/xsharding_info.h"

#include <vector>

NS_BEG2(top, network)

class xtop_network_driver_face : public xbasic_runnable_t<xtop_network_driver_face>
{
public:

    xtop_network_driver_face()                                             = default;
    xtop_network_driver_face(xtop_network_driver_face const &)             = delete;
    xtop_network_driver_face & operator=(xtop_network_driver_face const &) = delete;
    xtop_network_driver_face(xtop_network_driver_face &&)                  = default;
    xtop_network_driver_face & operator=(xtop_network_driver_face &&)      = default;
    ~xtop_network_driver_face() override                                   = default;

    /**
     * \brief Get the host node id.
     * \return The host node id.
     */
    virtual
    common::xnode_id_t const &
    host_node_id() const noexcept = 0;

    virtual
    xnode_t
    host_node() const noexcept = 0;

    /**
     * \brief Send message to specified endpoint with specified transmission property.
     * \param node_id The target address message sent to.
     * \param bytes_message The serialized message, upper tier module passed in, to be sent.
     * \param transmission_property Message transmission property.
     */
    virtual
    void
    send_to(common::xnode_id_t const & node_id,
            xbyte_buffer_t const & bytes_message) const = 0;

    /**
     * \brief Spread the rumor.
     * \param rumor The rumor to be spread.
     */
    virtual
    void
    spread_rumor(xbyte_buffer_t const & rumor) const = 0;

    virtual
    void
    spread_rumor(const common::xsharding_info_t & shardInfo, xbyte_buffer_t const & byte_msg) const {};

    virtual
    void
    forward_broadcast(const common::xsharding_info_t & shardInfo, common::xnode_type_t node_type, xbyte_buffer_t const & byte_msg) const{};

    /**
     * \brief Register the data ready notify handler.
     *        Do not register the handler multiple times.
     *        In debug mode, assert will participate in;
     *        In release mode, the later handler registered in will replace the older one.
     * \param callback The callback handles data ready notify.
     */
    virtual
    void
    register_message_ready_notify(xnetwork_message_ready_callback_t callback) noexcept = 0;

    /**
     * \brief Un-register the data ready notify handler.
     *        Do not un-register multiple times.
     *        In debug mode, assert will be triggered.
     *        In release mode, it just reset the handler to nullptr.
     */
    virtual
    void
    unregister_message_ready_notify() = 0;

    virtual
    bool
    p2p_bootstrap(std::vector<xdht_node_t> const & seeds) const = 0;

    virtual
    std::vector<common::xnode_id_t>
    neighbors() const = 0;

    virtual
    std::size_t
    neighbor_size_upper_limit() const noexcept = 0;
};

using xnetwork_driver_face_t = xtop_network_driver_face;

NS_END2
