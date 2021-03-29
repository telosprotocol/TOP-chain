#include "xelect_net/include/elect_netcard.h"

#include <memory>
#include <cinttypes>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xkad/routing_table/routing_table.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/xwrouter.h"
#include "xelect_net/proto/elect_net.pb.h"
#include "xelect_net/include/elect_uitils.h"
#include "xgossip/include/gossip_utils.h"
#include "xmetrics/xmetrics.h"
#include "xtransport/udp_transport/transport_util.h"

#include "xvnetwork/xvnetwork_message.h"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"

#define MSG_CATEGORY_ID 0x0002
#define BOOT_MSG_ID  0x00000001
#define BEACON_BOOTSTRAP ((static_cast<std::uint32_t>(static_cast<std::uint16_t>(MSG_CATEGORY_ID)) << 16) | (0x0000FFFF & static_cast<std::uint32_t>(BOOT_MSG_ID)))

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

#ifdef DEBUG
int EcNetcard::test_send(const std::string& des_node_id, bool broadcast, bool root) {
    // TODO(smaug)
    return kVhostSendSuccess;
}
#endif

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

    xdbg("send data chain_src: [%d][%d][%d][%d][%d][%d][%d]",
            send_kad_key->xnetwork_id(),
            send_kad_key->zone_id(),
            send_kad_key->cluster_id(),
            send_kad_key->group_id(),
            send_kad_key->node_id(),
            send_kad_key->xip_type(),
            send_kad_key->process_id());

    xdbg("send data chain_dst: [%d][%d][%d][%d][%d][%d][%d]",
            recv_kad_key->xnetwork_id(),
            recv_kad_key->zone_id(),
            recv_kad_key->cluster_id(),
            recv_kad_key->group_id(),
            recv_kad_key->node_id(),
            recv_kad_key->xip_type(),
            recv_kad_key->process_id());

    kadmlia::RoutingTablePtr routing_table;
    if (recv_kad_key->xnetwork_id() != kRoot &&
            send_kad_key->xnetwork_id() != kRoot &&
            !is_broadcast) {
        routing_table = wrouter::GetRoutingTable(send_kad_key->GetServiceType(), false);
    }

    if (!routing_table || routing_table->nodes_size() == 0) {
        xdbg("get routing table with root.");
        routing_table = wrouter::GetRoutingTable(kRoot, true);
    }

    if (!routing_table) {
        TOP_WARN("send chain msg failed, routing table empty");
        return kVhostSendSrcInvalid;
    }

    transport::protobuf::RoutingMessage pbft_message;
    pbft_message.set_broadcast(is_broadcast);

    //routing_table->SetFreqMessage(pbft_message);
    pbft_message.set_priority(enum_xpacket_priority_type_routine);
    pbft_message.set_is_root(false);
    if (recv_kad_key->xnetwork_id() == kRoot) {
        pbft_message.set_is_root(true);
    }

    // for now
    if (static_cast<uint32_t>(message.id()) == BEACON_BOOTSTRAP) {
        pbft_message.set_is_root(true);
        pbft_message.set_broadcast(true);
        auto kroot_routing_table = wrouter::GetRoutingTable(kRoot, true);
        // not very important, or no need, but just set
        pbft_message.set_des_node_id(kroot_routing_table->get_local_node_info()->id());
        xinfo("beacon bootstrap message, set is_root true, message.id: %d", message.id());
    }

    pbft_message.set_src_node_id(send_kad_key->Get());
    pbft_message.set_des_node_id(recv_kad_key->Get());
    pbft_message.set_type(kElectVhostRumorGossipMessage);
    pbft_message.set_id(CallbackManager::MessageId());

    elect::protobuf::PbftMessage vhost_msg;
    vhost_msg.set_cb_type(static_cast<uint32_t>(message.id()));
    vhost_msg.set_data((char*)message.payload().data(), message.payload().size());
    std::string vdata = vhost_msg.SerializeAsString();
    pbft_message.set_data(vdata);

    uint32_t chain_data_hash = base::xhash32_t::digest(
            std::string((char*)message.payload().data(), message.payload().size()));

    /*
    std::string dump_info = base::StringUtil::str_fmt("alarm elect_vhost_original_send local_node_id:%s "
            "chain_hash:%u chain_msgid:%u chain_msg_size:%u send_timestamp:%llu "
            "src_node_id:%s dest_node_id:%s is_root:%d broadcast:%d",
            HexEncode(global_xid->Get()).c_str(),
            chain_data_hash,
            message.id(),
            message.payload().size(),
            GetCurrentTimeMsec(),
            HexEncode(pbft_message.src_node_id()).c_str(),
            HexEncode(pbft_message.des_node_id()).c_str(),
            pbft_message.is_root(),
            pbft_message.broadcast());
    TOP_KINFO("%s", dump_info.c_str());
    */
    // replace with xmetrics
    XMETRICS_PACKET_INFO("p2p_electvhost_send",
            "local_gid", HexEncode(global_xid->Get()),
            "chain_hash", chain_data_hash,
            "chain_msgid", static_cast<uint64_t>(message.id()),
            "chain_msg_size", message.payload().size(),
            "send_timestamp", GetCurrentTimeMsec(),
            "src_node_id", HexEncode(pbft_message.src_node_id()),
            "dest_node_id", HexEncode(pbft_message.des_node_id()),
            "is_root", pbft_message.is_root(),
            "broadcast", pbft_message.broadcast());

    // point to point
    if (!pbft_message.has_broadcast() || !pbft_message.broadcast()) {
        pbft_message.set_type(kElectVhostRumorP2PMessage);
        pbft_message.set_data(vdata);
#ifndef NDEBUG
        std::string debug_info = base::StringUtil::str_fmt(
                "[ec_net] %s chain message point2point [chain_msg_type:%u] [hash:%u] [src:%s] [des:%s]",
                transport::FormatMsgid(pbft_message).c_str(),
                message.id(),
                chain_data_hash,
                HexEncode(pbft_message.src_node_id()).c_str(),
                HexEncode(pbft_message.des_node_id()).c_str());
        pbft_message.set_debug(debug_info);
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("begin", pbft_message);
#endif // NDEBUG
        if (wrouter::Wrouter::Instance()->send(pbft_message) != 0) {
            TOP_WARN("chain message point2point failed");
            return kVHostSendWrouterFailed;
        }
        return kVhostSendSuccess;
    }

    // broadcast to all use gossip-blooomfilter
    if (pbft_message.is_root()) {
        auto broadcast_type = gossip::kGossipBloomfilterZone;
        auto kroot_routing_table = wrouter::GetRoutingTable(kRoot, true);
        // TODO(smaug)
        if (kroot_routing_table->nodes_size() < kBroadcastTypeNodesSize) {
            broadcast_type = gossip::kGossipBloomfilter;
        }
        // TODO(smaug) for now
        broadcast_type = gossip::kGossipBloomfilter;
        return GossipWithHeaderBlock(
                pbft_message,
                broadcast_type,
                chain_data_hash,
                static_cast<uint32_t>(message.id()));
    }

    return GossipWithHeaderBlock(
            pbft_message,
            gossip::kGossipBloomfilterAndLayered,
            chain_data_hash,
            static_cast<uint32_t>(message.id()));
}

int EcNetcard::GossipWithHeaderBlock(
        transport::protobuf::RoutingMessage& pbft_message,
        uint32_t block_gossip_type,
        uint32_t chain_data_hash,
        uint32_t chain_msgid) const {
    xdbg("elect_vhost broadcast using broadcast_type:%u", block_gossip_type);
    bool large_packet = false;
    if (pbft_message.data().size() > 1500) {
        large_packet = true;
    }
    uint32_t vhash = base::xhash32_t::digest(pbft_message.data());
    std::string header_hash = std::to_string(vhash);

    // broadcast block
    auto gossip_block = pbft_message.mutable_gossip();
    gossip_block->set_neighber_count(3);
    //gossip_block->set_stop_times(gossip::kGossipSendoutMaxTimes);
    gossip_block->set_gossip_type(block_gossip_type);
    gossip_block->set_max_hop_num(20);
    gossip_block->set_evil_rate(0);
    if (pbft_message.is_root()) {
        gossip_block->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    } else {
        //gossip_block->set_switch_layer_hop_num(0);
        gossip_block->set_ign_bloomfilter_level(0);
    }
    gossip_block->set_left_overlap(2); // must smaller than 20
    gossip_block->set_right_overlap(2); // must smaller than 20
    gossip_block->set_block(pbft_message.data());
    gossip_block->set_header_hash(header_hash);
    pbft_message.clear_data();

#ifndef NDEBUG
    std::string debug_info_block = base::StringUtil::str_fmt(
            "[ec_net] %s chain message block [chain_msg_type:%u] [is_root:%d] [broadcast:%d] "
            "[hash:%u] [header_hash:%s] [src:%s] [des:%s]",
            transport::FormatMsgid(pbft_message).c_str(),
            chain_msgid,
            pbft_message.is_root(),
            pbft_message.broadcast(),
            chain_data_hash,
            HexEncode(header_hash).c_str(),
            HexEncode(pbft_message.src_node_id()).c_str(),
            HexEncode(pbft_message.des_node_id()).c_str());
    pbft_message.set_debug(debug_info_block);
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("begin", pbft_message);
#endif // NDEBUG

    if (pbft_message.is_root()) {
        // send to self
        base::xpacket_t self_packet;
        HandleRumorMessage(pbft_message, self_packet);
        TOP_DEBUG("kroot broadcast send to self");
    }
    if (wrouter::Wrouter::Instance()->send(pbft_message) != 0) {
        TOP_WARN("chain message block [is_root: %d] broadcast failed", pbft_message.is_root());
        return kVHostSendWrouterFailed;
    }

    /*
    if (!pbft_message.is_root() && !large_packet) {
        xinfo("not root, not large packet, no need broadcast header");
        return kVhostSendSuccess;
    }
    */

    if (pbft_message.is_root() || !large_packet) {
        xdbg("not root, not large packet, no need broadcast header");
        return kVhostSendSuccess;
    }

    // large multicast packet continue
    transport::protobuf::RoutingMessage pbft_message_header(pbft_message);
    pbft_message_header.clear_bloomfilter();
    pbft_message_header.set_hop_num(0);
    // different from block id
    pbft_message_header.set_id(CallbackManager::MessageId());
    auto gossip_header = pbft_message_header.mutable_gossip();
    gossip_header->set_neighber_count(4);
    //gossip_header->set_stop_times(gossip::kGossipSendoutMaxTimes);
    gossip_header->set_gossip_type(gossip::kGossipBloomfilter);
    gossip_header->set_max_hop_num(10);
    gossip_header->set_evil_rate(0);
    gossip_header->set_switch_layer_hop_num(gossip::kGossipSwitchLayerCount);
    gossip_header->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    gossip_header->set_header_hash(header_hash);
    gossip_header->clear_msg_hash();
    gossip_header->clear_block();

#ifndef NDEBUG
    std::string debug_info_header = base::StringUtil::str_fmt(
            "[ec_net] %s chain message block [chain_msg_type:%u] [is_root:%d] [broadcast:%d] [hash:%u] [header_hash:%s] [src:%s] [des:%s]",
            transport::FormatMsgid(pbft_message_header).c_str(),
            chain_msgid,
            pbft_message_header.is_root(),
            pbft_message_header.broadcast(),
            chain_data_hash,
            HexEncode(header_hash).c_str(),
            HexEncode(pbft_message_header.src_node_id()).c_str(),
            HexEncode(pbft_message_header.des_node_id()).c_str());
    pbft_message_header.set_debug(debug_info_header);
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("begin", pbft_message_header);
#endif // NDEBUG
    if (wrouter::Wrouter::Instance()->send(pbft_message_header) != 0) {
        TOP_WARN("chain message header [is_root: %d] broadcast failed", pbft_message_header.is_root());
        return kVHostSendWrouterFailed;
    }
    return kVhostSendSuccess;
}


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
    elect::protobuf::PbftMessage vhost_msg;
    if (!vhost_msg.ParseFromString(data)) {
        TOP_ERROR("%s elect::protobuf::PbftMessage ParseFromString failed!", transport::FormatMsgid(message).c_str());
        return;
    }

    elect::xelect_message_t msg(
        { vhost_msg.data().c_str(), vhost_msg.data().c_str() + vhost_msg.data().size() },
            static_cast<common::xmessage_id_t>(vhost_msg.cb_type()));

    uint32_t chain_hash = base::xhash32_t::digest(
            std::string((char*)msg.payload().data(), msg.payload().size()));
    /*
    std::string dump_info = base::StringUtil::str_fmt("alarm elect_vhost_final_recv local_node_id:%s chain_hash:%u "
            "chain_msgid:%u packet_size:%u chain_msg_size:%u "
            "hop_num:%u recv_timestamp:%llu src_node_id:%s dest_node_id:%s is_root:%d broadcast:%d",
            HexEncode(global_xid->Get()).c_str(),
            chain_hash,
            msg.id(),
            packet.get_size(),
            msg.payload().size(),
            message.hop_num(),
            GetCurrentTimeMsec(),
            HexEncode(message.src_node_id()).c_str(),
            HexEncode(message.des_node_id()).c_str(),
            message.is_root(),
            message.broadcast());
    TOP_KINFO("%s", dump_info.c_str());
    */
    // replace with xmetrics
    XMETRICS_PACKET_INFO("p2p_electvhost_recv",
            "local_gid", HexEncode(global_xid->Get()),
            "chain_hash", chain_hash,
            "chain_msgid", static_cast<uint64_t>(msg.id()),
            "packet_size", packet.get_size(),
            "chain_msg_size", msg.payload().size(),
            "hop_num", message.hop_num(),
            "recv_timestamp", GetCurrentTimeMsec(),
            "src_node_id", HexEncode(message.src_node_id()),
            "dest_node_id", HexEncode(message.des_node_id()),
            "is_root", message.is_root(),
            "broadcast", message.broadcast());



#ifdef ENABLE_METRICS

    if (message.type() == kElectVhostRumorP2PMessage) {
        XMETRICS_FLOW_COUNT("p2p_transport_p2pchain_afterfilter_packet_recv", 1);
        XMETRICS_FLOW_COUNT("p2p_transport_p2pchain_afterfilter_bandwidth_recv", packet.get_body().size());

        XMETRICS_FLOW_COUNT("p2p_transport_p2pchain_afterfilter_real_packet_recv", 1);
        XMETRICS_FLOW_COUNT("p2p_transport_p2pchain_afterfilter_real_bandwidth_recv", vhost_msg.data().size());
    } else if (message.type() == kElectVhostRumorGossipMessage) {
        XMETRICS_FLOW_COUNT("p2p_transport_gossipchain_afterfilter_packet_recv", 1);
        XMETRICS_FLOW_COUNT("p2p_transport_gossipchain_afterfilter_bandwidth_recv", packet.get_body().size());

        if (message.is_root()) {
            XMETRICS_FLOW_COUNT("p2p_transport_gossipchain_root_afterfilter_packet_recv", 1);
            XMETRICS_FLOW_COUNT("p2p_transport_gossipchain_root_afterfilter_bandwidth_recv", packet.get_body().size());
        }

        XMETRICS_FLOW_COUNT("p2p_transport_gossipchain_afterfilter_real_packet_recv", 1);
        XMETRICS_FLOW_COUNT("p2p_transport_gossipchain_afterfilter_real_bandwidth_recv", vhost_msg.data().size());
    }
#endif

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



}  // namespace elect

}  // namespace top
