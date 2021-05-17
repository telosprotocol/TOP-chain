// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/message_handler/xwrouter_xid_handler.h"

#include "xbase/xutl.h"
#include "xgossip/include/gossip_bloomfilter.h"
#include "xgossip/include/gossip_bloomfilter_layer.h"
#include "xgossip/include/gossip_filter.h"
#include "xgossip/include/gossip_utils.h"
#include "xkad/gossip/rumor_filter.h"
#include "xkad/routing_table/client_node_manager.h"
#include "xkad/routing_table/dynamic_xip_manager.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#include "xpbase/base/redis_client.h"
#include "xpbase/base/redis_utils.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/uint64_bloomfilter.h"
#include "xpbase/base/xip_parser.h"
#include "xtransport/udp_transport/transport_util.h"
#include "xtransport/utils/transport_utils.h"
#include "xwrouter/message_handler/wrouter_message_handler.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "xwrouter/register_routing_table.h"

#include <algorithm>

namespace top {

using namespace kadmlia;
using namespace gossip;

namespace wrouter {

WrouterXidHandler::WrouterXidHandler(transport::TransportPtr transport_ptr,
                                     std::shared_ptr<gossip::GossipInterface> bloom_gossip_ptr,
                                     std::shared_ptr<gossip::GossipInterface> bloom_layer_gossip_ptr,
                                     std::shared_ptr<gossip::GossipInterface> gossip_rrs_ptr)

  : WrouterHandler(transport_ptr, bloom_gossip_ptr, bloom_layer_gossip_ptr, gossip_rrs_ptr) {
}

WrouterXidHandler::~WrouterXidHandler() {
}

int32_t WrouterXidHandler::SendPacket(transport::protobuf::RoutingMessage & message) {
    if (message.hop_num() >= kadmlia::kHopToLive) {
        TOP_WARN2("stop SendPacket hop_num(%d) beyond max_hop_num(%d)", message.hop_num(), kadmlia::kHopToLive);
        return enum_xerror_code_fail;
    }

    // root broadcast might have empyt src && dst node id
    if (GossipPacketCheck(message)) {
        return SendGossip(message);
    }

    if (message.des_node_id().empty()) {
        TOP_WARN2("send illegal");
        return enum_xerror_code_fail;
    }

    if (message.src_node_id().empty()) {
        // choose one random(right) id for this message
        uint64_t service_type = ParserServiceType(message.des_node_id());
        RoutingTablePtr routing_table = nullptr;

        if (!message.has_is_root() || !message.is_root()) {
            routing_table = FindRoutingTable(false, service_type, false);
        }

        if (!routing_table || routing_table->nodes_size() == 0) {
            routing_table = FindRoutingTable(true, static_cast<uint64_t>(kRoot), true, message.des_node_id());
        }

        if (!routing_table) {
            TOP_WARN2("FindRoutingTable failed");
            return enum_xerror_code_fail;
        }
        message.set_src_node_id(routing_table->get_local_node_info()->id());
    }

    if (MulticastPacketCheck(message)) {
        return SendMulticast(message);
    }

    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("SendPacket base xid", message);
    return SendGeneral(message);
}

int32_t WrouterXidHandler::RecvPacket(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    int32_t judgeown_code = JudgeOwnPacket(message, packet);

    switch (judgeown_code) {
    case kJudgeOwnError: {
        TOP_WARN2("RecvBaseXid failed");
        return kRecvError;
    }
    case kJudgeOwnYes: {
        return kRecvOwn;
    }
    case kJudgeOwnNoAndContinue: {
        SendPacket(message);
        return kRecvOk;
    }
    case kJudgeOwnYesAndContinue: {
        SendPacket(message);
        return kRecvOwn;
    }
    default:
        break;
    }

    return kRecvOk;
}

uint64_t WrouterXidHandler::ParserServiceType(const std::string & kad_key) {
    auto kad_key_ptr = base::GetKadmliaKey(kad_key);
    return kad_key_ptr->GetServiceType();
}

int32_t WrouterXidHandler::SendGeneral(transport::protobuf::RoutingMessage & message) {
    if (message.des_node_id().empty()) {
        assert(false);
    }

    uint64_t service_type = ParserServiceType(message.des_node_id());
    RoutingTablePtr routing_table = nullptr;
    if (message.has_is_root() && message.is_root()) {
        routing_table = FindRoutingTable(true, static_cast<uint64_t>(kRoot), true, message.des_node_id());
        if (!routing_table) {
            TOP_WARN("kroot routing_table not ready, send failed");
            return enum_xerror_code_fail;
        }
    } else {
        routing_table = FindRoutingTable(false, service_type, false);
        if (!routing_table) {
            TOP_DEBUG("FindRoutingTable failed of service_type: %llu, try crossing network", service_type);

            TOP_DEBUG("crossing network from local_xid: %s to des: %llu", HexEncode(global_xid->Get()).c_str(), service_type);

            std::vector<kadmlia::NodeInfoPtr> des_nodes;
            kadmlia::NodeInfoPtr des_node_ptr;
            if (!wrouter::ServiceNodes::Instance()->GetRootNodes(service_type, message.des_node_id(), des_node_ptr) || !des_node_ptr) {
                TOP_WARN("crossing network failed, can't find des nodes of service_type: %llu", service_type);
                return enum_xerror_code_fail;
            }

            des_nodes.push_back(des_node_ptr);
            TOP_DEBUG("crossing network begin, des_nodes size: %d", des_nodes.size());
            return SendData(message, des_nodes, kBroadcastGeneral, false);
        }
    }
    TOP_DEBUG("sendgeneral using routing_table: %s", HexEncode(routing_table->get_local_node_info()->id()).c_str());

    std::string des_xid = message.des_node_id();
    std::vector<kadmlia::NodeInfoPtr> nodes = GetClosestNodes(routing_table,
                                                              des_xid,
                                                              8,  // choose 8 nodes then use bloomfilter choose kBroadcastGeneral nodes
                                                              false);
    if (nodes.empty()) {
        TOP_WARN2("GetClosestNodes failed[%d][%d]", routing_table->nodes_size(), routing_table->get_local_node_info()->kadmlia_key()->xnetwork_id());
        return enum_xerror_code_fail;
    }
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("SendData", message);
    return SendData(message, nodes, kBroadcastGeneral, false);
}

int32_t WrouterXidHandler::SendMulticast(transport::protobuf::RoutingMessage & message) {
    if (message.is_root()) {
        TOP_WARN2("wsend: send multicast base xid invalid, must not root message");
        return enum_xerror_code_fail;
    }
    if (message.src_node_id().empty() || message.des_node_id().empty()) {
        assert(false);
    }
    auto gossip = message.mutable_gossip();
    if (!gossip->has_msg_hash()) {
        std::string bin_data = message.data();
        if (gossip->has_block()) {
            bin_data = gossip->block();
        }
        if (!gossip->has_block() && gossip->has_header_hash()) {
            bin_data = gossip->header_hash();
        }
        uint32_t msg_hash = base::xhash32_t::digest(message.xid() + std::to_string(message.id()) + bin_data);
        gossip->set_msg_hash(msg_hash);
    }

    uint64_t des_service_type = ParserServiceType(message.des_node_id());
    RoutingTablePtr routing_table = FindRoutingTable(false, des_service_type, false);

    // local does'nt have way to des, using root or find des-nodes first
    if (!routing_table || routing_table->nodes_size() == 0) {
        TOP_DEBUG("crossing network from local_xid: %s to des: %llu %s", HexEncode(global_xid->Get()).c_str(), des_service_type, HexEncode(message.des_node_id()).c_str());

        std::vector<kadmlia::NodeInfoPtr> des_nodes;
        if (!wrouter::ServiceNodes::Instance()->GetRootNodes(des_service_type, des_nodes) || des_nodes.empty()) {
            TOP_WARN("crossing network failed, can't find des nodes of service_type: %llu %s", des_service_type, HexEncode(message.des_node_id()).c_str());
            return enum_xerror_code_fail;
        }

        TOP_DEBUG("crossing network begin, des_nodes size: %d", des_nodes.size());
        return SendData(message, des_nodes, 3, true);
    }

    return GossipBroadcast(message, routing_table);
}

int32_t WrouterXidHandler::SendGossip(transport::protobuf::RoutingMessage & message) {
    if (!message.has_is_root() || !message.is_root()) {
        TOP_WARN2("SendGossip must be root_msg");
        return enum_xerror_code_fail;
    }
    auto gossip = message.mutable_gossip();
    if (!gossip->has_msg_hash()) {
        std::string bin_data = message.data();
        if (gossip->has_block()) {
            bin_data = gossip->block();
        }
        if (!gossip->has_block() && gossip->has_header_hash()) {
            bin_data = gossip->header_hash();
        }
        uint32_t msg_hash = base::xhash32_t::digest(message.xid() + std::to_string(message.id()) + bin_data);
        gossip->set_msg_hash(msg_hash);
    }

    RoutingTablePtr routing_table;
    routing_table = FindRoutingTable(true, static_cast<uint64_t>(kRoot), true);

    TOP_DEBUG("sendgossip routing_table: %s", HexEncode(routing_table->get_local_node_info()->id()).c_str());

    if (!routing_table) {
        TOP_WARN2("FindRoutingTable failed");
        return enum_xerror_code_fail;
    }

    return GossipBroadcast(message, routing_table);
}

int32_t WrouterXidHandler::GossipBroadcast(transport::protobuf::RoutingMessage & message, kadmlia::RoutingTablePtr & routing_table) {
    uint32_t gossip_type = message.gossip().gossip_type();
    if (gossip_type == 0) {
        gossip_type = kGossipBloomfilter;
    }

    auto neighbors = routing_table->GetUnLockNodes();
    if (!neighbors) {
        TOP_WARN2("GetUnLockNodes empty");
        return enum_xerror_code_fail;
    }
    switch (gossip_type) {
    case kGossipBloomfilter:
        bloom_gossip_ptr_->Broadcast(routing_table->get_local_node_info()->hash64(), message, neighbors);
        break;
    case kGossipBloomfilterAndLayered:
        bloom_layer_gossip_ptr_->Broadcast(message, routing_table);
        break;
    case kGossipRRS:
        gossip_rrs_ptr_->Broadcast(routing_table->get_local_node_info()->hash64(), message, neighbors);
        break;
    default:
        TOP_WARN2("invalid gossip_type:%d", gossip_type);
        assert(false);
        break;
    }
    return enum_xcode_successful;
}

int32_t WrouterXidHandler::SendData(transport::protobuf::RoutingMessage & message, const std::vector<kadmlia::NodeInfoPtr> & neighbors, uint32_t next_size, bool broadcast_stride) {
    if (neighbors.empty()) {
        TOP_WARN2("invliad neighbors");
        return enum_xerror_code_fail;
    }

    std::vector<NodeInfoPtr> rest_neighbors;
    if (message.broadcast()) {
        auto gossip_info = message.mutable_gossip();
        gossip_info->set_diff_net(broadcast_stride);
        std::vector<uint64_t> new_bloomfilter_vec;
        for (auto i = 0; i < message.bloomfilter_size(); ++i) {
            new_bloomfilter_vec.push_back(message.bloomfilter(i));
        }

        std::shared_ptr<base::Uint64BloomFilter> new_bloomfilter;
        if (new_bloomfilter_vec.empty()) {
            new_bloomfilter = std::make_shared<base::Uint64BloomFilter>(gossip::kGossipBloomfilterSize, gossip::kGossipBloomfilterHashNum);
            auto tmp_routing_table = FindRoutingTable(true, static_cast<uint64_t>(kRoot), true, message.des_node_id());
            new_bloomfilter->Add(tmp_routing_table->get_local_node_info()->hash64());
        } else {
            new_bloomfilter = std::make_shared<base::Uint64BloomFilter>(new_bloomfilter_vec, gossip::kGossipBloomfilterHashNum);
        }
        for (uint32_t i = 0; i < neighbors.size(); ++i) {
            NodeInfoPtr node_ptr = neighbors[i];

            if (new_bloomfilter->Contain(node_ptr->hash64) && node_ptr->node_id != message.des_node_id()) {
                continue;
            }

            rest_neighbors.push_back(node_ptr);
            new_bloomfilter->Add(node_ptr->hash64);
            if (rest_neighbors.size() >= next_size) {
                break;
            }
        }

        const std::vector<uint64_t> & bloomfilter_vec = new_bloomfilter->Uint64Vector();
        message.clear_bloomfilter();
        for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
            message.add_bloomfilter(bloomfilter_vec[i]);
        }
    }
    base::xpacket_t packet(base::xcontext_t::instance());
    _xip2_header xip2_header;
    memset(&xip2_header, 0, sizeof(xip2_header));
    xip2_header.ver_protocol = kSerializeProtocolProtobuf;
    std::string header((const char *)&xip2_header, sizeof(xip2_header));

    std::string xbody;
    if (!message.SerializeToString(&xbody)) {
        TOP_WARN2("wrouter message SerializeToString failed");
        return enum_xerror_code_fail;
    }

    std::string xdata = header + xbody;

    auto each_call = [this, &packet, &message, neighbors, &xdata](kadmlia::NodeInfoPtr node_info_ptr) {
        if (!node_info_ptr) {
            TOP_WARN2("kadmlia::NodeInfoPtr null");
            return false;
        }
        packet.reset();
        packet.get_body().push_back((uint8_t *)xdata.data(), xdata.size());
        packet.set_to_ip_addr(node_info_ptr->public_ip);
        packet.set_to_ip_port(node_info_ptr->public_port);
        //        if (kadmlia::kKadSuccess != transport_ptr_->SendData(packet)) {
        if (kadmlia::kKadSuccess != transport_ptr_->SendDataWithProp(packet, node_info_ptr->udp_property)) {
            TOP_WARN2("SendData to  endpoint(%s:%d) failed", node_info_ptr->public_ip.c_str(), node_info_ptr->public_port);
            return false;
        }
        return true;
    };
    TOP_DEBUG("finally get destnode size:%u", rest_neighbors.size());
    for (const auto & item : rest_neighbors) {
        TOP_DEBUG("finally get %s %s:%u", HexEncode(item->node_id).c_str(), item->public_ip.c_str(), item->public_port);
    }

    if (message.broadcast()) {
        std::for_each(rest_neighbors.begin(), rest_neighbors.end(), each_call);
    } else {
        std::for_each(neighbors.begin(), neighbors.begin() + 1, each_call);
    }
    return enum_xcode_successful;
}

bool WrouterXidHandler::HandleSystemMessage(transport::protobuf::RoutingMessage & message, kadmlia::RoutingTablePtr & routing_table) {
    static std::vector<int32_t> direct_vec = {
        kKadBootstrapJoinRequest,
        kKadBootstrapJoinResponse,
        kKadFindNodesRequest,
        kKadFindNodesResponse,
        kKadHeartbeatRequest,
        kKadHeartbeatResponse,
        kKadHandshake,
        kKadConnectRequest,

        kNatDetectRequest,
        kNatDetectResponse,
        kNatDetectHandshake2Node,
        kNatDetectHandshake2Boot,
        kNatDetectFinish,
        kUdpNatDetectRequest,
        kUdpNatDetectResponse,
        kUdpNatHeartbeat,

        kGossipBlockSyncAsk,
        kGossipBlockSyncAck,
        kGossipBlockSyncRequest,
        kGossipBlockSyncResponse,
    };
    auto it = std::find(direct_vec.begin(), direct_vec.end(), message.type());
    if (it != direct_vec.end()) {
        return true;
    }

    return false;
}

int32_t WrouterXidHandler::JudgeOwnPacket(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (message.has_broadcast() && message.broadcast()) {
        if (gossip::GossipFilter::Instance()->FilterMessage(message)) {
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter gossip filter kJudgeOwnNoAndContinue", message);
            return kJudgeOwnNoAndContinue;
        }
        // TOP_DEBUG("wrouter gossip filter finished, continue code");

        if (message.is_root()) {
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYesAndContinue", message);
            return kJudgeOwnYesAndContinue;
        }
        if (message.src_node_id().empty() || message.des_node_id().empty()) {
            assert(false);
        }

        uint64_t des_service_type = ParserServiceType(message.des_node_id());

        RoutingTablePtr routing_table = FindRoutingTable(false, des_service_type, false);
        if (routing_table) {
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYesAndContinue", message);
            return kJudgeOwnYesAndContinue;
        }
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnNoAndContinue", message);
        return kJudgeOwnNoAndContinue;
    }

    // usually only bootstrap message will come here
    if (message.des_node_id().empty()) {
        TOP_DEBUG("message type(%d) id(%d) des_node_id empty", message.type(), message.id());
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
        return kJudgeOwnYes;
    }

    // if (message.has_is_root() && message.is_root()) {
    if (message.has_is_root() && message.is_root() && message.type() == kRootMessage) {
        // for root routing table message, just for kRootMessage return yes, other root message also need relay
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
        return kJudgeOwnYes;
    }

    uint64_t service_type = ParserServiceType(message.des_node_id());
    RoutingTablePtr routing_table = nullptr;
    if (message.is_root()) {
        routing_table = FindRoutingTable(true, static_cast<uint64_t>(kRoot), true, message.des_node_id());
    } else {
        routing_table = FindRoutingTable(false, service_type, false);
    }

    if (!routing_table) {
        TOP_WARN2("FindRoutingTable failed, judge own packet: type(%d) failed", message.type());
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnNoAndContinue", message);
        return kJudgeOwnNoAndContinue;
    }

    if (HandleSystemMessage(message, routing_table)) {
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
        return kJudgeOwnYes;
    }

    std::string match_kad_id = routing_table->get_local_node_info()->id();
    if (message.des_node_id().compare(match_kad_id) == 0) {
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
        return kJudgeOwnYes;
    }

    bool closest = false;
    if (routing_table->ClosestToTarget(message.des_node_id(), closest) != kadmlia::kKadSuccess) {
        TOP_WARN2("ClosestToTarget goes wrong");
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnError", message);
        return kJudgeOwnError;
    }
    if (closest) {
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
        return kJudgeOwnYes;
    }

    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnNoAndContinue", message);
    return kJudgeOwnNoAndContinue;
}

bool WrouterXidHandler::MulticastPacketCheck(transport::protobuf::RoutingMessage & message) {
    if (!message.has_broadcast() || !message.broadcast()) {
        return false;
    }

    if (message.has_is_root() && message.is_root()) {
        return false;
    }
    // broadcast to same network or different network
    return true;
}

bool WrouterXidHandler::GossipPacketCheck(transport::protobuf::RoutingMessage & message) {
    if (!message.has_broadcast() || !message.broadcast()) {
        return false;
    }

    if (!message.has_is_root() || !message.is_root()) {
        return false;
    }
    // broadcast to root network(all nodes)
    return true;
}

}  // namespace wrouter

}  // namespace top
