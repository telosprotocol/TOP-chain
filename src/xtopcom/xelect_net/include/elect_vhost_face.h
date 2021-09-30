// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xrunnable.h"
// #include "xnetwork/xmessage_transmission_property.h"
#include "xnetwork/xnetwork_message_ready_callback.h"
// #include "xnetwork/xnode.h"
// #include "xnetwork/xp2p/xdht_host_face.h"
#include "xcommon/xsharding_info.h"
#include "xdata/xdata_common.h"

#include <system_error>
#include <vector>
NS_BEG2(top, elect)

class xtop_network_driver_face : public xbasic_runnable_t<xtop_network_driver_face> {
public:
    xtop_network_driver_face() = default;
    xtop_network_driver_face(xtop_network_driver_face const &) = delete;
    xtop_network_driver_face & operator=(xtop_network_driver_face const &) = delete;
    xtop_network_driver_face(xtop_network_driver_face &&) = default;
    xtop_network_driver_face & operator=(xtop_network_driver_face &&) = default;
    ~xtop_network_driver_face() override = default;

    virtual void send_to(common::xip2_t const & src, common::xip2_t const & dst, xbyte_buffer_t const & byte_message, std::error_code & ec) const = 0;
    virtual void send_to_through_root(common::xip2_t const & src, common::xnode_id_t const & dst_node_id, xbyte_buffer_t const & byte_message, std::error_code & ec) const = 0;
    virtual void spread_rumor(common::xip2_t const & src, common::xip2_t const & dst, xbyte_buffer_t const & byte_message, std::error_code & ec) const = 0;
    virtual void broadcast(common::xip2_t const & src, xbyte_buffer_t const & byte_message, std::error_code & ec) const = 0;

    /**
     * \brief Get the host node id.
     * \return The host node id.
     */
    virtual common::xnode_id_t const & host_node_id() const noexcept = 0;

    /**
     * \brief Register the data ready notify handler.
     *        Do not register the handler multiple times.
     *        In debug mode, assert will participate in;
     *        In release mode, the later handler registered in will replace the older one.
     * \param callback The callback handles data ready notify.
     */
    virtual void register_message_ready_notify(top::network::xnetwork_message_ready_callback_t callback) noexcept = 0;

    /**
     * \brief Un-register the data ready notify handler.
     *        Do not un-register multiple times.
     *        In debug mode, assert will be triggered.
     *        In release mode, it just reset the handler to nullptr.
     */
    virtual void unregister_message_ready_notify() = 0;
};

using xnetwork_driver_face_t = xtop_network_driver_face;

NS_END2
