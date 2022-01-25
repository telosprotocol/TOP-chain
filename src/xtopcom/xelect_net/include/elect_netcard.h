// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xbase/xpacket.h"
#include "xbasic/xsimple_message.hpp"
#include "xcommon/xmessage_id.h"
#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/routing_utils.h"
#include "xnetwork/xnetwork_message_ready_callback.h"
#include "xpbase/base/top_timer.h"
#include "xpbase/base/top_utils.h"
#include "xtransport/proto/transport.pb.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <system_error>

namespace top {

namespace kadmlia {
class RoutingTable;
}

namespace elect {
using xelect_message_t = top::xsimple_message_t<top::common::xmessage_id_t>;

class EcNetcard : public std::enable_shared_from_this<EcNetcard> {
public:
    EcNetcard() = default;
    virtual ~EcNetcard();
    EcNetcard(const EcNetcard &) = delete;
    EcNetcard & operator=(const EcNetcard &) = delete;
    EcNetcard & operator=(EcNetcard &&) = delete;
    EcNetcard(EcNetcard &&) = delete;

public:
    /**
     * @brief init EcNetcard and register message type and callback
     *
     */
    void Init();

    void send_to(base::KadmliaKeyPtr const & send_kad_key, base::KadmliaKeyPtr const & recv_kad_key, xbyte_buffer_t const & bytes_message, std::error_code & ec) const;

    void spread_rumor(base::KadmliaKeyPtr const & send_kad_key, base::KadmliaKeyPtr const & recv_kad_key, xbyte_buffer_t const & bytes_message, std::error_code & ec) const;

    void broadcast(base::KadmliaKeyPtr const & send_kad_key, base::KadmliaKeyPtr const & recv_kad_key, xbyte_buffer_t const & bytes_message, std::error_code & ec) const;


    /**
     * @brief register message callback for networkid
     *
     * @param cb callback to handle message
     * @param xnetwork_id network id
     */
    void register_message_ready_notify(network::xnetwork_message_ready_callback_t cb, const uint32_t & xnetwork_id) noexcept;
    /**
     * @brief unregister message callback for networkid
     *
     * @param xnetwork_id network id
     */
    void unregister_message_ready_notify(const uint32_t & xnetwork_id);

protected:
    void GossipWithHeaderBlock(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, std::error_code & ec) const;
    void GossipOldRootBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, std::error_code & ec) const;
    // int GossipOldLayerBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type) const;
    void GossipDispatchBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, std::error_code & ec) const;

private:
    void HandleRumorMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) const;
    size_t rumor_callback_size() const;
    network::xnetwork_message_ready_callback_t get_rumor_callback(const uint32_t & xnetwork_id) const;
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
