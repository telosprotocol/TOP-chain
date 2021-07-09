#include "xelect_net/include/elect_netcard.h"

#include <memory>
#include <cinttypes>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/xwrouter.h"
#include "xelect_net/proto/elect_net.pb.h"
#include "xelect_net/include/elect_uitils.h"
#include "xgossip/include/gossip_utils.h"
#include "xmetrics/xmetrics.h"
#include "xtransport/udp_transport/transport_util.h"

#include "xvnetwork/xvnetwork_message.h"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"

static const uint32_t kBroadcastTypeNodesSize = 10;

namespace top {

using namespace vnetwork;
using namespace kadmlia;

namespace elect {

std::mutex EcNetcard::register_msg_handler_mutex_;
bool EcNetcard::rumor_msg_handler_registered_ = false;

EcNetcard::~EcNetcard() {
    UnInit();
    TOP_INFO("EcNetcard::UnInit");
}

void EcNetcard::Init() {
    assert(!rumor_msg_handler_registered_);
    std::unique_lock<std::mutex> lock(register_msg_handler_mutex_);
    if (rumor_msg_handler_registered_) {
        return;
    }
    wrouter::WrouterRegisterMessageHandler(kElectVhostRumorP2PMessage, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleRumorMessage(message, packet);
    });

    wrouter::WrouterRegisterMessageHandler(kElectVhostRumorGossipMessage, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleRumorMessage(message, packet);
    });
    rumor_msg_handler_registered_ = true;
    TOP_INFO("EcNetcard::Init");

    return;
}

void EcNetcard::UnInit() {
    std::unique_lock<std::mutex> lock(register_msg_handler_mutex_);
    rumor_msg_handler_registered_ = false;
    wrouter::WrouterUnregisterMessageHandler(kElectVhostRumorP2PMessage);
    wrouter::WrouterUnregisterMessageHandler(kElectVhostRumorGossipMessage);
}

void EcNetcard::register_message_ready_notify(network::xnetwork_message_ready_callback_t cb, const uint32_t& xnetwork_id) noexcept {
    std::unique_lock<std::mutex> lock(rumor_callback_map_mutex_);
    assert(rumor_callback_map_.find(xnetwork_id) == rumor_callback_map_.end());
    rumor_callback_map_[xnetwork_id] = cb;
    TOP_INFO("EcNetcard register rumor_callback for xnetwork_id:%u", xnetwork_id);
}

void EcNetcard::unregister_message_ready_notify(const uint32_t& xnetwork_id) {
    std::unique_lock<std::mutex> lock(rumor_callback_map_mutex_);
    auto ifind = rumor_callback_map_.find(xnetwork_id);
    if (ifind == rumor_callback_map_.end()) {
        return;
    }
    rumor_callback_map_.erase(ifind);
}

size_t EcNetcard::rumor_callback_size()  const {
    std::unique_lock<std::mutex> lock(rumor_callback_map_mutex_);
    return rumor_callback_map_.size();
}

network::xnetwork_message_ready_callback_t EcNetcard::get_rumor_callback(const uint32_t& xnetwork_id)  const {
    std::unique_lock<std::mutex> lock(rumor_callback_map_mutex_);
    auto ifind = rumor_callback_map_.find(xnetwork_id);
    if (ifind == rumor_callback_map_.end()) {
        return nullptr;
    }
    return ifind->second;
}

std::vector<network::xnetwork_message_ready_callback_t> EcNetcard::getall_rumor_callback() const {
    std::vector<network::xnetwork_message_ready_callback_t> cb_vec;
    std::unique_lock<std::mutex> lock(rumor_callback_map_mutex_);
    for (const auto& item : rumor_callback_map_) {
        if (item.second) {
            cb_vec.push_back(item.second);
        }
    }
    return cb_vec;
}

int EcNetcard::send(
        const base::KadmliaKeyPtr& send_kad_key,
        const base::KadmliaKeyPtr& recv_kad_key,
        const elect::xelect_message_t& message,
        bool is_broadcast) const {
    if (!send_kad_key) {
        TOP_ERROR("invalid chain src address.");
        return kVhostSendSrcInvalid;
    }
    if (!recv_kad_key) {
        TOP_ERROR("invalid chain dst address.[%d][%d][%d][%d]",
                recv_kad_key->xnetwork_id(),
                recv_kad_key->zone_id(),
                recv_kad_key->cluster_id(),
                recv_kad_key->group_id());
        assert(false);
        return kVhostSendDstInvalid;
    }

    xdbg("send data chain_src: [%d][%d][%d][%d][%d]",
            send_kad_key->xnetwork_id(),
            send_kad_key->zone_id(),
            send_kad_key->cluster_id(),
            send_kad_key->group_id(),
            recv_kad_key->slot_id());

    xdbg("send data chain_dst: [%d][%d][%d][%d][%d]",
            recv_kad_key->xnetwork_id(),
            recv_kad_key->zone_id(),
            recv_kad_key->cluster_id(),
            recv_kad_key->group_id(),
            recv_kad_key->slot_id());

    transport::protobuf::RoutingMessage pbft_message;
    pbft_message.set_broadcast(is_broadcast);
    pbft_message.set_priority(enum_xpacket_priority_type_routine);
    pbft_message.set_is_root(false);
    if (recv_kad_key->xnetwork_id() == kRoot) {
        pbft_message.set_is_root(true);
    }

    pbft_message.set_src_node_id(send_kad_key->Get());
    pbft_message.set_des_node_id(recv_kad_key->Get());
    pbft_message.set_type(kElectVhostRumorGossipMessage);
    pbft_message.set_id(CallbackManager::MessageId());

    elect::protobuf::VhostMessage vhost_msg;
    vhost_msg.set_cb_type(static_cast<uint32_t>(message.id()));
    vhost_msg.set_data((char*)message.payload().data(), message.payload().size());
    std::string vdata = vhost_msg.SerializeAsString();
    pbft_message.set_data(vdata);

    uint32_t chain_data_hash = base::xhash32_t::digest(
            std::string((char*)message.payload().data(), message.payload().size()));

    // XMETRICS_PACKET_INFO("p2p_electvhost_send",
    //         "local_gid", HexEncode(global_xid->Get()),
    //         "chain_hash", chain_data_hash,
    //         "chain_msgid", static_cast<uint64_t>(message.id()),
    //         "chain_msg_size", message.payload().size(),
    //         "send_timestamp", GetCurrentTimeMsec(),
    //         "src_node_id", HexEncode(pbft_message.src_node_id()),
    //         "dest_node_id", HexEncode(pbft_message.des_node_id()),
    //         "is_root", pbft_message.is_root(),
    //         "broadcast", pbft_message.broadcast());

    // point to point
    if (!pbft_message.has_broadcast() || !pbft_message.broadcast()) {
        pbft_message.set_type(kElectVhostRumorP2PMessage);
        pbft_message.set_data(vdata);
        if (wrouter::Wrouter::Instance()->send(pbft_message) != 0) {
            TOP_WARN("chain message point2point failed");
            return kVHostSendWrouterFailed;
        }
        return kVhostSendSuccess;
    }

    // broadcast to all use gossip-blooomfilter
    if (pbft_message.is_root()) {
        return GossipOldRootBroadcast(pbft_message, gossip::kGossipBloomfilter, chain_data_hash, static_cast<uint32_t>(message.id()));
        // return GossipWithHeaderBlock(pbft_message, gossip::kGossipRRS, chain_data_hash, static_cast<uint32_t>(message.id()));
    }

    return GossipDispatchBroadcast(pbft_message, gossip::kGossipDispatcher, chain_data_hash, static_cast<uint32_t>(message.id()));
}

int EcNetcard::GossipWithHeaderBlock(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, uint32_t chain_data_hash, uint32_t chain_msgid) const {
    xdbg("elect_vhost broadcast using broadcast_type:%u", block_gossip_type);
    uint32_t vhash = base::xhash32_t::digest(pbft_message.data());
    std::string header_hash = std::to_string(vhash);

    // broadcast block
    auto gossip_block = pbft_message.mutable_gossip();

    //! This two param is determined with N and affect the node broadcast logical
    //! Thus should be hard-code in gossip_rrs.cc(gossip_utils.h) rather than set in message.
    // gossip_block->set_neighber_count(6);
    // gossip_block->set_switch_layer_hop_num(4);

    gossip_block->set_gossip_type(block_gossip_type);
    gossip_block->set_max_hop_num(20);

    // gossip_block->set_stop_times(gossip::kGossipSendoutMaxTimes);
    // gossip_block->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);

    gossip_block->set_block(pbft_message.data());
    gossip_block->set_header_hash(header_hash);
    pbft_message.clear_data();

    // send to self
    base::xpacket_t self_packet;
    HandleRumorMessage(pbft_message, self_packet);
    TOP_DEBUG("kroot broadcast send to self");

    if (wrouter::Wrouter::Instance()->send(pbft_message) != 0) {
        TOP_WARN("chain message block [is_root: %d] broadcast failed", pbft_message.is_root());
        return kVHostSendWrouterFailed;
    }

    return kVhostSendSuccess;
}

int EcNetcard::GossipOldRootBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, uint32_t chain_data_hash, uint32_t chain_msgid) const {
    xdbg("elect_vhost broadcast using broadcast_type:%u", block_gossip_type);

    uint32_t vhash = base::xhash32_t::digest(pbft_message.data());
    std::string header_hash = std::to_string(vhash);

    // broadcast block
    auto gossip_block = pbft_message.mutable_gossip();
    gossip_block->set_neighber_count(3);
    // gossip_block->set_stop_times(gossip::kGossipSendoutMaxTimes);
    gossip_block->set_gossip_type(block_gossip_type);
    gossip_block->set_max_hop_num(20);

    gossip_block->set_block(pbft_message.data());
    gossip_block->set_header_hash(header_hash);
    pbft_message.clear_data();

    // send to self
    base::xpacket_t self_packet;
    HandleRumorMessage(pbft_message, self_packet);
    TOP_DEBUG("kroot broadcast send to self");

    if (wrouter::Wrouter::Instance()->send(pbft_message) != 0) {
        TOP_WARN("chain message block [is_root: %d] broadcast failed", pbft_message.is_root());
        return kVHostSendWrouterFailed;
    }

    return kVhostSendSuccess;
}
#if 0
int EcNetcard::GossipOldLayerBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, uint32_t chain_data_hash, uint32_t chain_msgid) const {
    xdbg("elect_vhost broadcast using broadcast_type:%u", block_gossip_type);

    uint32_t vhash = base::xhash32_t::digest(pbft_message.data());
    std::string header_hash = std::to_string(vhash);

    // broadcast block
    auto gossip_block = pbft_message.mutable_gossip();
    // gossip_block->set_stop_times(gossip::kGossipSendoutMaxTimes);
    gossip_block->set_gossip_type(block_gossip_type);
    gossip_block->set_max_hop_num(20);

    gossip_block->set_neighber_count(3);
    // gossip_block->set_switch_layer_hop_num(0);
    gossip_block->set_ign_bloomfilter_level(0);
    // todo end next version delete.

    // gossip_block->set_left_overlap(0);   // must smaller than 20
    // gossip_block->set_right_overlap(0);  // must smaller than 20
    gossip_block->set_block(pbft_message.data());
    gossip_block->set_header_hash(header_hash);
    pbft_message.clear_data();

    if (wrouter::Wrouter::Instance()->send(pbft_message) != 0) {
        TOP_WARN("chain message block [is_root: %d] broadcast failed", pbft_message.is_root());
        return kVHostSendWrouterFailed;
    }

    return kVhostSendSuccess;
}
#endif
int EcNetcard::GossipDispatchBroadcast(transport::protobuf::RoutingMessage & pbft_message, uint32_t block_gossip_type, uint32_t chain_data_hash, uint32_t chain_msgid) const {
    xdbg("elect_vhost broadcast using broadcast_type:%u", block_gossip_type);

    uint32_t vhash = base::xhash32_t::digest(pbft_message.data());
    std::string header_hash = std::to_string(vhash);

    // broadcast block
    auto gossip_block = pbft_message.mutable_gossip();
    // gossip_block->set_stop_times(gossip::kGossipSendoutMaxTimes);
    gossip_block->set_gossip_type(block_gossip_type);
    gossip_block->set_max_hop_num(20);

    gossip_block->set_neighber_count(3);

    gossip_block->set_sit1(0);
    gossip_block->set_sit2(0);
    gossip_block->set_overlap_rate(6);
    gossip_block->set_block(pbft_message.data());
    gossip_block->set_header_hash(header_hash);
    pbft_message.clear_data();

    if (wrouter::Wrouter::Instance()->send(pbft_message) != 0) {
        TOP_WARN("chain message block [is_root: %d] broadcast failed", pbft_message.is_root());
        return kVHostSendWrouterFailed;
    }

    return kVhostSendSuccess;
}

#define IS_BROADCAST(message) (message.broadcast())
#define IS_RRS_GOSSIP_MESSAGE(message) (message.is_root() && message.broadcast() && message.gossip().gossip_type() == 8)
#define IS_RRS_PULLED_MESSAGE(message) message.ack_id() == 181819
#define MESSAGE_BASIC_INFO(message) "src_node_id", (message.src_node_id()), "dst_node_id", (message.des_node_id()), "hop_num", message.hop_num()
#define MESSAGE_RRS_FEATURE(message) "gossip_header_hash", std::stol(message.gossip().header_hash()), "gossip_block_size", message.gossip().block().size()
#define MESSAGE_FEATURE(message) "msg_hash", message.msg_hash(), "msg_size", message.gossip().block().size()
#define IS_ROOT_BROADCAST(message) "is_root", message.is_root(), "is_broadcast", message.broadcast()
#define PACKET_SIZE(packet) "packet_size", packet.get_size()
#define NOW_TIME "timestamp", GetCurrentTimeMsec()

void EcNetcard::HandleRumorMessage(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) const {
    if (rumor_callback_size() == 0) {
        TOP_ERROR("%s message [rumor root] has not set callback function!", transport::FormatMsgid(message).c_str());
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("no callback here", message);
        return;
    }

    std::string data = message.data(); // point2point or broadcast without header and block
    if (message.has_gossip()) {
        // broadcast  with header and block
        auto gossip = message.gossip();
        if (!gossip.has_block() && gossip.has_header_hash()) {
            xdbg("%s HandleRumorMessage header arrive", transport::FormatMsgid(message).c_str());
            return;
        }

        if (gossip.has_block()) {
            // broadcast with header and block (block arrive)
            data = gossip.block();
        }
    }

    xdbg("%s HandleRumorMessage", transport::FormatMsgid(message).c_str());
    elect::protobuf::VhostMessage vhost_msg;
    if (!vhost_msg.ParseFromString(data)) {
        TOP_ERROR("%s elect::protobuf::VhostMessage ParseFromString failed!", transport::FormatMsgid(message).c_str());
        return;
    }

    elect::xelect_message_t msg(
        { vhost_msg.data().c_str(), vhost_msg.data().c_str() + vhost_msg.data().size() },
            static_cast<common::xmessage_id_t>(vhost_msg.cb_type()));

 
    uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + data);
    message.set_msg_hash(msg_hash);

 
    // if (IS_RRS_GOSSIP_MESSAGE(message)) {
    //     XMETRICS_PACKET_INFO("p2pperf_vhostrecv_info",
    //                          MESSAGE_BASIC_INFO(message),
    //                          MESSAGE_RRS_FEATURE(message),
    //                          IS_ROOT_BROADCAST(message),
    //                          "is_pulled",
    //                          IS_RRS_PULLED_MESSAGE(message),
    //                          PACKET_SIZE(packet),
    //                          NOW_TIME);
    // } else {
    if (IS_BROADCAST(message)) {
        XMETRICS_PACKET_INFO(
            "p2pbroadcast_vhostrecv_info", MESSAGE_BASIC_INFO(message), MESSAGE_FEATURE(message), IS_ROOT_BROADCAST(message), "is_pulled", 0, PACKET_SIZE(packet), NOW_TIME);
    }
    // }

    // TODO(smaug) for kRoot message, call all rumor_callback for now
    if (message.is_root()) {
        common::xnode_id_t node_id;
        auto cb_vec = getall_rumor_callback();
        for (const auto& cb : cb_vec) {
            cb(node_id, msg.payload());
        }
        return;
    }

    auto des_kad_key_ptr = base::GetKadmliaKey(message.des_node_id());
    uint32_t xnetwork_id = des_kad_key_ptr->xnetwork_id();
    auto xnet_cb = get_rumor_callback(xnetwork_id);
    if (!xnet_cb) {
        TOP_WARN("no rumor_callback found for xnetwork_id:%u", xnetwork_id);
        return;
    }

    common::xnode_id_t node_id;
    xnet_cb(node_id, msg.payload());
    //TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("end", message);
}

#undef IS_BROADCAST
#undef IS_RRS_GOSSIP_MESSAGE
#undef IS_RRS_PULLED_MESSAGE
#undef MESSAGE_BASIC_INFO
#undef MESSAGE_RRS_FEATURE
#undef MESSAGE_FEATURE
#undef IS_ROOT_BROADCAST
#undef PACKET_SIZE
#undef NOW_TIME

}  // namespace elect

}  // namespace top
