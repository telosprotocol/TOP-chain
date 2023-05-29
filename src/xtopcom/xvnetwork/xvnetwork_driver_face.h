// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file xvnetwork_driver_face.h
 * @brief interface definitions for xvnetwork_driver class
 */

#pragma once

#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"
#include "xcommon/xaddress.h"
#include "xcommon/xip.h"
#include "xcommon/xmessage_category.h"
#include "xdata/xnode_info.h"
#include "xcommon/xnode_type.h"
#include "xvnetwork/xmessage_ready_callback.h"
#include "xvnetwork/xvhost_face_fwd.h"

#include <map>
#include <memory>

NS_BEG2(top, vnetwork)

class xtop_vnetwork_driver_face : public xbasic_runnable_t<xtop_vnetwork_driver_face> {
public:
    xtop_vnetwork_driver_face() = default;
    xtop_vnetwork_driver_face(xtop_vnetwork_driver_face const &) = delete;
    xtop_vnetwork_driver_face & operator=(xtop_vnetwork_driver_face const &) = delete;
    xtop_vnetwork_driver_face(xtop_vnetwork_driver_face &&) = delete;
    xtop_vnetwork_driver_face & operator=(xtop_vnetwork_driver_face &&) = delete;
    ~xtop_vnetwork_driver_face() override = default;

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
     * @param ec Record error code.
     */
    virtual void send_to(xvnode_address_t const & to, xmessage_t const & message, std::error_code & ec) = 0;

    /**
     * @brief Send message to other via xip.
     * 
     * @param to The destination xip address to send
     * @param message The message to send to
     * @param ec Error code when something goes wrong.
     */
    virtual void send_to(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) = 0;

    /**
     * @brief Broadcast message to other via xip.
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
    virtual common::xaccount_address_t const & account_address() const noexcept = 0;

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
     * @param election_round The election round of validator group
     * @return std::map<common::xslot_id_t, data::xnode_info_t> 
     */
    virtual std::map<common::xslot_id_t, data::xnode_info_t> children_info2(common::xgroup_id_t const & gid, common::xelection_round_t const & election_round) const = 0;

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
    virtual std::vector<common::xnode_address_t> archive_addresses(common::xnode_type_t node_type) const = 0;

    // virtual std::vector<common::xnode_address_t> archive_addresses(std::error_code & ec) const = 0;
    // virtual std::vector<common::xnode_address_t> exchange_addresses(std::error_code & ec) const = 0;

    /// @brief Get the fullnodes' addresses
    /// @param ec Store the error code when getting the data.
    /// @return The fullnodes' addresses.
    virtual std::vector<common::xnode_address_t> fullnode_addresses(std::error_code & ec) const = 0;

    /// @brief Get the relays' addresses
    /// @param ec Store the error code when getting the data.
    /// @return The relays' addresses.
    virtual std::vector<common::xnode_address_t> relay_addresses(std::error_code & ec) const = 0;

    /**
     * @brief Get table ids belonging to this zone
     * 
     * @return std::vector<std::uint16_t> 
     */
    virtual std::vector<std::uint16_t> table_ids() const = 0;

    virtual common::xelection_round_t const & joined_election_round() const = 0;
};
using xvnetwork_driver_face_t = xtop_vnetwork_driver_face;
using xvnetwork_driver_face_ptr_t = std::shared_ptr<xvnetwork_driver_face_t>;

NS_END2
