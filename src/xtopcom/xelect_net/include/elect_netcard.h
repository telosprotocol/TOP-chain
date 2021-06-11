// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <mutex>
#include <chrono>
#include <memory>

#include "xcommon/xmessage_id.h"
#include "xbasic/xsimple_message.hpp"
#include "xnetwork/xnetwork_message_ready_callback.h"

#include "xbase/xpacket.h"
#include "xpbase/base/top_utils.h"
#include "xtransport/proto/transport.pb.h"
#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/top_timer.h"
#include "xbase/xbase.h"

namespace top {
    
namespace kadmlia {
    class RoutingTable;
}


namespace elect {
using xelect_message_t = top::xsimple_message_t<top::common::xmessage_id_t>;

class EcNetcard: public std::enable_shared_from_this<EcNetcard> {
public:
    EcNetcard() = default;
    virtual ~EcNetcard();
    EcNetcard(const EcNetcard&) = delete;
    EcNetcard& operator=(const EcNetcard&) = delete;
    EcNetcard& operator=(EcNetcard&&) = delete;
    EcNetcard(EcNetcard&&) = delete;

public:
    /**
     * @brief init EcNetcard and register message type and callback
     * 
     */
    void Init();
    /**
     * @brief send message of broadcast,multicast,point-to-point
     * 
     * @param send_kad_key sender kademlia id(kademlia id is the id of kademlia p2p network)
     * @param recv_kad_key receiver kademlia id(kademlia id is the id of kademlia p2p network)
     * @param message message to be send
     * @param is_broadcast true for broadcast, false for point-to-point
     * @return int 
     */
    int send(
        const base::KadmliaKeyPtr& send_kad_key,
        const base::KadmliaKeyPtr& recv_kad_key,
        const elect::xelect_message_t& message,
        bool is_broadcast) const;
    /**
     * @brief register message callback for networkid
     * 
     * @param cb callback to handle message
     * @param xnetwork_id network id
     */
    void register_message_ready_notify(network::xnetwork_message_ready_callback_t cb, const uint32_t& xnetwork_id) noexcept;
    /**
     * @brief unregister message callback for networkid
     * 
     * @param xnetwork_id network id
     */
    void unregister_message_ready_notify(const uint32_t& xnetwork_id);
protected:
    int GossipWithHeaderBlock(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, uint32_t chain_data_hash, uint32_t chain_msgid) const;
    int GossipOldRootBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, uint32_t chain_data_hash, uint32_t chain_msgid) const;
    // int GossipOldLayerBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, uint32_t chain_data_hash, uint32_t chain_msgid) const;
    int GossipDispatchBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, uint32_t chain_data_hash, uint32_t chain_msgid) const;

private:
    void HandleRumorMessage(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) const;
    size_t rumor_callback_size() const;
    network::xnetwork_message_ready_callback_t get_rumor_callback(const uint32_t& xnetwork_id) const;
    std::vector<network::xnetwork_message_ready_callback_t> getall_rumor_callback() const;
    void UnInit();

    static std::mutex register_msg_handler_mutex_;
    static bool rumor_msg_handler_registered_;

    mutable std::mutex rumor_callback_map_mutex_;
    std::map<uint32_t, network::xnetwork_message_ready_callback_t> rumor_callback_map_;
};

typedef std::shared_ptr<EcNetcard> EcNetcardPtr;

}  // namespace elect

}  // namespace top
