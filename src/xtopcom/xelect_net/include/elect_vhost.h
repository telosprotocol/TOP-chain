// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <mutex>
#include <chrono>
#include <memory>

#include "xnetwork/xnetwork_driver_face.h"
#include "xnetwork/xnetwork_driver_fwd.h"
#include "xcommon/xmessage_id.h"
#include "xbasic/xsimple_message.hpp"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xrunnable.h"
#include "xnetwork/xmessage_transmission_property.h"
#include "xnetwork/xnetwork_message_ready_callback.h"
#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xdht_host_face.h"
#include "xnetwork/xp2p/xdht_host.h"
#include "xcommon/xsharding_info.h"
#include "xdata/xdata_common.h"

#include "xpbase/base/top_utils.h"
#include "xbase/xbase.h"
#include "xelect_net/include/elect_netcard.h"
#include "xvnetwork/xaddress.h"

namespace top {
    
namespace kadmlia {
    class RoutingTable;
}


namespace elect {

using xelect_message_t = top::xsimple_message_t<top::common::xmessage_id_t>;

class EcVHost : public std::enable_shared_from_this<EcVHost>, public network::xnetwork_driver_face_t  {
public:
    EcVHost() {}
    EcVHost(const uint32_t& xnetwork_id, const EcNetcardPtr& ec_netcard, common::xnode_id_t const & node_id);
    virtual ~EcVHost() {}
public:
    /**
     * @brief Get the host node id
     * 
     * @return common::xnode_id_t const&  the host id
     */
    virtual common::xnode_id_t const & host_node_id() const noexcept;
    /**
     * @brief get host node
     * 
     * @return network::xnode_t 
     */
    virtual network::xnode_t host_node() const noexcept;
    /**
     * @brief Send message to specified endpoint with specified transmission property.
     * 
     * @param node_id The target address message sent to
     * @param bytes_message bytes_message The serialized message, upper tier module passed in, to be sent
     * @param transmission_property transmission_property Message transmission property.
     */
    virtual void send_to(
            common::xnode_id_t const & node_id,
            xbyte_buffer_t const & bytes_message,
            network::xtransmission_property_t const & transmission_property) const;
    /**
     * @brief Spread the rumor
     * 
     * @param rumor The rumor to be spread
     */
    virtual void spread_rumor(xbyte_buffer_t const & rumor) const;
    /**
     * @brief Spread the rumor
     * 
     * @param shardInfo dest shard info
     * @param rumor the rumor to be spread
     */
    virtual void spread_rumor(
            const common::xsharding_info_t & shardInfo,
            xbyte_buffer_t const & rumor) const;
    /**
     * @brief broadcast message
     * 
     * @param shardInfo the dest shard info
     * @param node_type node type
     * @param byte_msg binary message to be broadcast
     */
    virtual void forward_broadcast(
            const common::xsharding_info_t & shardInfo,
            common::xnode_type_t node_type,
            xbyte_buffer_t const & byte_msg) const;
    /**
     * @brief Register the data ready notify handler.
     *        Do not register the handler multiple times.
     *        In debug mode, assert will participate in;
     *        In release mode, the later handler registered in will replace the older one.
     * 
     * @param cb The callback handles data ready notify.
     */
    virtual void register_message_ready_notify(network::xnetwork_message_ready_callback_t cb) noexcept;
    /**
     * @brief Unregister the data ready notify handler.
     * 
     */
    virtual void unregister_message_ready_notify();
    /**
     * @brief not implement yet
     * 
     * @param seeds TODO
     * @return true TODO
     * @return false TODO
     */
    virtual bool p2p_bootstrap(std::vector<network::xdht_node_t> const & seeds) const;
    /**
     * @brief not implement yet
     * 
     * @param to TODO
     * @param verification_data TODO
     * @param transmission_property TODO
     */
    virtual void direct_send_to(
            network::xnode_t const & to,
            xbyte_buffer_t verification_data,
            network::xtransmission_property_t const & transmission_property);
    /**
     * @brief not implement
     * 
     * @return std::vector<common::xnode_id_t> TODO
     */
    virtual std::vector<common::xnode_id_t> neighbors() const;
    /**
     * @brief not implement
     * 
     * @return std::size_t TODO
     */
    virtual std::size_t neighbor_size_upper_limit() const noexcept;
    /**
     * @brief not implement
     * 
     * @return network::p2p::xdht_host_face_t const& TODO
     */
    network::p2p::xdht_host_face_t const & dht_host() const noexcept;
    /**
     * @brief start the network driver
     * 
     */
    virtual void start() {};
    /**
     * @brief stop the network driver
     * 
     */
    virtual void stop() {};

protected:
    bool SyncMessageWhenStart(
            const vnetwork::xvnode_address_t & send_address,
            const vnetwork::xvnode_address_t & recv_address,
            const common::xmessage_id_t& message_type) const;
 

private:
    uint32_t xnetwork_id_ { 1 };
    EcNetcardPtr ec_netcard_ { nullptr };
    common::xnode_id_t m_node_id_;


    DISALLOW_COPY_AND_ASSIGN(EcVHost);
};

typedef std::shared_ptr<EcVHost> EcVHostPtr;

}  // namespace elect

}  // namespace top
