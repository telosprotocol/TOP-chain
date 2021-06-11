// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file xvnetwork_driver_face.h
 * @brief interface definitions for xvnetwork_driver class
 */

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"
#include "xcommon/xaddress.h"
#include "xcommon/xip.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xmessage_category.h"
#include "xcommon/xnode_info.h"
#include "xcommon/xrole_type.h"
#include "xdata/xnode_info.h"
#include "xcommon/xnode_type.h"
#include "xnetwork/xmessage_transmission_property.h"
#include "xvnetwork/xmessage_ready_callback.h"
#include "xvnetwork/xvhost_face_fwd.h"

#include <map>
#include <memory>
#include <set>

NS_BEG2(top, vnetwork)

class xtop_vnetwork_driver_face : public xbasic_runnable_t<xtop_vnetwork_driver_face> {
public:
    xtop_vnetwork_driver_face() = default;
    xtop_vnetwork_driver_face(xtop_vnetwork_driver_face const &) = delete;
    xtop_vnetwork_driver_face & operator=(xtop_vnetwork_driver_face const &) = delete;
    xtop_vnetwork_driver_face(xtop_vnetwork_driver_face &&) = default;
    xtop_vnetwork_driver_face & operator=(xtop_vnetwork_driver_face &&) = default;
    virtual ~xtop_vnetwork_driver_face() = default;

    /**
     * @brief Register the message for a specified message category.
     * 
     * @param message_category Message category for the callback registered.
     * @param cb The message callback.
     */
    virtual void register_message_ready_notify(common::xmessage_category_t const message_category, xvnetwork_message_ready_callback_t cb) = 0;

    /**
     * @brief Un-register the message notify for a message category.
     * 
     * @param message_category Message category to un-register.
     */
    virtual void unregister_message_ready_notify(common::xmessage_category_t const message_category) = 0;

    /**
     * @brief Get network id of vnetwork dirver
     * 
     * @return common::xnetwork_id_t 
     */
    virtual common::xnetwork_id_t network_id() const noexcept = 0;

    /**
     * @brief Get node address of vnetwork driver
     * 
     * @return xvnode_address_t 
     */
    virtual xvnode_address_t address() const = 0;

    /**
     * @brief Send message to other vnetwork_driver through the underlying module vhost
     * 
     * @param to The destination address to send
     * @param message The message to send
     * @param transmission_property The transport properties of the message
     */
    virtual void send_to(xvnode_address_t const & to, xmessage_t const & message, network::xtransmission_property_t const & transmission_property = {}) = 0;

    /**
     * @brief Broadcast message to vnetwork_driver within the cluster through the underlying module vhost
     * 
     * @param message The message to broadcast
     */ 
    virtual void broadcast(xmessage_t const & message) = 0;

    /**
     * @brief Forward a broadcast to dst cluster address through the underlying module vhost
     * 
     * @param message The message to broadcast
     * @param dst The destination to broadcast the message, must be a cluster address.
     */
    virtual void forward_broadcast_message(xmessage_t const & message, xvnode_address_t const & dst) = 0;

    /**
     * @brief 
     * 
     * @param dst 
     * @param message The message to broadcast
     */
    virtual void broadcast_to(xvnode_address_t const & dst, xmessage_t const & message) = 0;

    /**
     * @brief 
     * 
     * @param to The destination xip address to send
     * @param message The message to send to
     * @param ec Error code when something goes wrong.
     */
    virtual void send_to(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) = 0;

    /**
     * @brief 
     * 
     * @param to The destination xip address to broadcast
     * @param message The message to broadcast
     * @param ec Error code when something goes wrong.
     */
    virtual void broadcast(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) = 0;

    /**
     * @brief Get the node id of vhost
     * 
     * @return common::xnode_id_t const& 
     */
    virtual common::xnode_id_t const & host_node_id() const noexcept = 0;

    /**
     * @brief Get the parent group's address
     * 
     * @return xvnode_address_t 
     */
    virtual xvnode_address_t parent_group_address() const = 0;

    /**
     * @brief Get the neighbors' node info
     * 
     * @return std::map<common::xslot_id_t, data::xnode_info_t> 
     */
    virtual std::map<common::xslot_id_t, data::xnode_info_t> neighbors_info2() const = 0;

    /**
     * @brief Get the parents' node info
     * 
     * @return std::map<common::xslot_id_t, data::xnode_info_t> 
     */
    virtual std::map<common::xslot_id_t, data::xnode_info_t> parents_info2() const = 0;

    /**
     * @brief Get the children' node info (validator group)
     * 
     * @param gid The validator group id 
     * @param version The version of validator group
     * @return std::map<common::xslot_id_t, data::xnode_info_t> 
     */
    virtual std::map<common::xslot_id_t, data::xnode_info_t> children_info2(common::xgroup_id_t const & gid, common::xversion_t const & version) const = 0;

    /**
     * @brief Get the underlying vhost ptr
     * 
     * @return observer_ptr<xvhost_face_t> 
     */
    virtual observer_ptr<xvhost_face_t> virtual_host() const noexcept = 0;

    /**
     * @brief Get the working type of this virtual node.
     * 
     * @return The virtual node type.
     */
    virtual common::xnode_type_t type() const noexcept = 0;

    /**
     * @brief Get the archive's addresses
     * 
     * @return std::vector<xvnode_address_t> 
     */
    virtual std::vector<xvnode_address_t> archive_addresses(common::xnode_type_t node_type) const = 0;

    /**
     * @brief Get table ids belonging to this zone
     * 
     * @return std::vector<std::uint16_t> 
     */
    virtual std::vector<std::uint16_t> table_ids() const = 0;
};
using xvnetwork_driver_face_t = xtop_vnetwork_driver_face;
using xvnetwork_driver_face_ptr_t = std::shared_ptr<xvnetwork_driver_face_t>;

NS_END2
