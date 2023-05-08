// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <mutex>
#include <chrono>
#include <memory>

#include "xbase/xbase.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xrunnable.h"
#include "xbasic/xsimple_message.hpp"
#include "xcommon/xaddress.h"
#include "xcommon/xmessage_id.h"
#include "xcommon/xsharding_info.h"
#include "xelect_net/include/elect_netcard.h"
#include "xelect_net/include/elect_vhost_face.h"
#include "xnetwork/xnetwork_message_ready_callback.h"

namespace top {
    
namespace kadmlia {
    class RoutingTable;
}


namespace elect {

using xelect_message_t = top::xsimple_message_t<top::common::xmessage_id_t>;

class EcVHost : public std::enable_shared_from_this<EcVHost>, public elect::xnetwork_driver_face_t  {
public:
    EcVHost() = default;
    EcVHost(EcVHost const &) = delete;
    EcVHost & operator=(EcVHost const &) = delete;
    EcVHost(EcVHost &&) = delete;
    EcVHost & operator=(EcVHost &&) = delete;
    ~EcVHost() override = default;

    EcVHost(uint32_t xnetwork_id, const EcNetcardPtr & ec_netcard, common::xaccount_address_t const & node_id);

public:
    void send_to(common::xip2_t const & src, common::xip2_t const & dst, xbyte_buffer_t const & byte_message, std::error_code & ec) const override;
    void send_to_through_root(common::xip2_t const & src, common::xnode_id_t const & dst_node_id, xbyte_buffer_t const & byte_message, std::error_code & ec) const override;
    void spread_rumor(common::xip2_t const & src, common::xip2_t const & dst, xbyte_buffer_t const & byte_message, std::error_code & ec) const override;
    void broadcast(common::xip2_t const & src, xbyte_buffer_t const & byte_message, std::error_code & ec) const override;

public:
    /**
     * @brief Get the host node id
     * 
     * @return common::xnode_id_t const&  the host id
     */
    common::xaccount_address_t const & account_address() const noexcept override;
    /**
     * @brief get host node
     * 
     * @return network::xnode_t 
     */
#if 0
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
            xbyte_buffer_t const & bytes_message) const;
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
#endif
    /**
     * @brief Register the data ready notify handler.
     *        Do not register the handler multiple times.
     *        In debug mode, assert will participate in;
     *        In release mode, the later handler registered in will replace the older one.
     * 
     * @param cb The callback handles data ready notify.
     */
    void register_message_ready_notify(network::xnetwork_message_ready_callback_t cb) noexcept override;
    /**
     * @brief Unregister the data ready notify handler.
     * 
     */
    void unregister_message_ready_notify() override;
#if 0
    virtual bool p2p_bootstrap(std::vector<network::xdht_node_t> const & seeds) const;

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
     * @brief start the network driver
     * 
     */
#endif
    void start() override {}
    /**
     * @brief stop the network driver
     * 
     */
    virtual void stop() {};

protected:
    bool SyncMessageWhenStart(common::xnode_address_t const & send_address,
                              common::xnode_address_t const & recv_address,
                              common::xmessage_id_t const & message_type) const;
 

private:
    uint32_t xnetwork_id_ { 1 };
    EcNetcardPtr ec_netcard_ { nullptr };
    common::xaccount_address_t m_node_id_;
};

typedef std::shared_ptr<EcVHost> EcVHostPtr;

}  // namespace elect

}  // namespace top
