// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/message_handler/xwrouter_xid_handler.h"

#include "xbase/xutl.h"
#include "xgossip/include/gossip_bloomfilter.h"
#include "xgossip/include/gossip_filter.h"
#include "xgossip/include/gossip_utils.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/uint64_bloomfilter.h"
#include "xtransport/udp_transport/transport_util.h"
#include "xtransport/utils/transport_utils.h"
#include "xwrouter/message_handler/wrouter_message_handler.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/multi_routing/service_node_cache.h"

#include <algorithm>

namespace top {

using namespace kadmlia;
using namespace gossip;

namespace wrouter {

WrouterXidHandler::WrouterXidHandler(transport::TransportPtr transport_ptr,
                                     std::shared_ptr<gossip::GossipInterface> bloom_gossip_ptr,
                                     std::shared_ptr<gossip::GossipInterface> bloom_layer_gossip_ptr,
                                     std::shared_ptr<gossip::GossipInterface> gossip_rrs_ptr,
                                     std::shared_ptr<gossip::GossipInterface> gossip_dispatcher_ptr)
  : transport_ptr_(transport_ptr)
  , bloom_gossip_ptr_(bloom_gossip_ptr)
  , bloom_layer_gossip_ptr_(bloom_layer_gossip_ptr)
  , gossip_rrs_ptr_(gossip_rrs_ptr)
  , gossip_dispatcher_ptr_(gossip_dispatcher_ptr) {
}

WrouterXidHandler::~WrouterXidHandler() {
    transport_ptr_ = nullptr;
    bloom_gossip_ptr_ = nullptr;
    bloom_layer_gossip_ptr_ = nullptr;
    gossip_rrs_ptr_ = nullptr;
    gossip_dispatcher_ptr_ = nullptr;
}

kadmlia::ElectRoutingTablePtr WrouterXidHandler::FindElectRoutingTable(base::ServiceType service_type) {
    return MultiRouting::Instance()->GetElectRoutingTable(service_type);
}
kadmlia::RootRoutingTablePtr WrouterXidHandler::FindRootRoutingTable() {
    return MultiRouting::Instance()->GetRootRoutingTable();
}

int32_t WrouterXidHandler::SendPacket(transport::protobuf::RoutingMessage & message) {
    assert(message.has_msg_hash());
    if (message.hop_num() >= kadmlia::kHopToLive) {
        xwarn("stop SendPacket hop_num(%d) beyond max_hop_num(%d)", message.hop_num(), kadmlia::kHopToLive);
        return enum_xerror_code_fail;
    }

    // root broadcast might have empyt src && dst node id
    if (GossipPacketCheck(message)) {
        return SendGossip(message);
    }

    if (message.des_node_id().empty()) {
        xwarn("send illegal");
        return enum_xerror_code_fail;
    }

    if (message.src_node_id().empty()) {
        // assert(false);
        // choose one random(right) id for this message
        base::ServiceType service_type = ParserServiceType(message.des_node_id());
        if (service_type == base::ServiceType{kRoot} && message.has_is_root() && message.is_root()) {
            auto root_routing_table = FindRootRoutingTable();
            message.set_src_node_id(root_routing_table->get_local_node_info()->kad_key());
        } else {
            auto elect_routing_table = FindElectRoutingTable(service_type);
            if (!elect_routing_table) {
                xwarn("FindRoutingTable failed");
                return enum_xerror_code_fail;
            }
            message.set_src_node_id(elect_routing_table->get_local_node_info()->kad_key());
        }
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
        xwarn("RecvBaseXid failed");
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

base::ServiceType WrouterXidHandler::ParserServiceType(const std::string & kad_key) {
    auto kad_key_ptr = base::GetKadmliaKey(kad_key);
    return kad_key_ptr->GetServiceType();
}

int32_t WrouterXidHandler::SendGeneral(transport::protobuf::RoutingMessage & message) {
    if (message.des_node_id().empty()) {
        assert(false);
    }

    base::ServiceType service_type = ParserServiceType(message.des_node_id());
    // RoutingTablePtr routing_table = nullptr;
    if (message.has_is_root() && message.is_root()) {
        RootRoutingTablePtr routing_table = FindRootRoutingTable();
        if (!routing_table) {
            xwarn("kroot routing_table not ready, send failed");
            return enum_xerror_code_fail;
        }

        xdbg("sendgeneral using routing_table: %s", (routing_table->get_local_node_info()->kad_key()).c_str());

        std::string des_xid = message.des_node_id();
        routing_table->GetClosestNodes(des_xid, 8);
        std::vector<kadmlia::NodeInfoPtr> nodes = routing_table->GetClosestNodes(des_xid, 8);
        if (nodes.empty()) {
            xwarn("GetClosestNodes failed[%d][%d]", routing_table->nodes_size(), routing_table->get_local_node_info()->kadmlia_key()->xnetwork_id());
            return enum_xerror_code_fail;
        }
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("SendData", message);
        return SendData(message, nodes, kBroadcastGeneral, false);

    } else {
        ElectRoutingTablePtr routing_table = FindElectRoutingTable(service_type);
        if (!routing_table) {
            xdbg("FindRoutingTable failed of service_type: %llu, try crossing network", service_type.value());

            xdbg("crossing network from local_xid: %s to des: %llu", (global_xid->Get()).c_str(), service_type.value());

            std::vector<kadmlia::NodeInfoPtr> des_nodes;
            kadmlia::NodeInfoPtr des_node_ptr;
            if (!wrouter::ServiceNodes::Instance()->GetRootNodes(service_type, message.des_node_id(), des_node_ptr) || !des_node_ptr) {
                xwarn("crossing network failed, can't find des nodes of service_type: %llu", service_type.value());
                return enum_xerror_code_fail;
            }

            des_nodes.push_back(des_node_ptr);
            xdbg("crossing network begin, des_nodes size: %d", des_nodes.size());
            return SendData(message, des_nodes, kBroadcastGeneral, false);
        }

        xdbg("sendgeneral using routing_table: %s", (routing_table->get_local_node_info()->kad_key()).c_str());

        // no root ,no broadcast p2p 1159
        std::string des_xid = message.des_node_id();
        std::vector<kadmlia::NodeInfoPtr> nodes;
        // routing_table->GetRandomNodes(nodes,8);
        nodes.push_back(routing_table->GetNode(des_xid));

        if (nodes.empty()) {
            xwarn("GetClosestNodes failed[%d][%d]", routing_table->nodes_size(), routing_table->get_local_node_info()->kadmlia_key()->xnetwork_id());
            return enum_xerror_code_fail;
        }
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("SendData", message);
        return SendData(message, nodes, kBroadcastGeneral, false);
    }
}

int32_t WrouterXidHandler::SendMulticast(transport::protobuf::RoutingMessage & message) {
    if (message.is_root()) {
        xwarn("wsend: send multicast base xid invalid, must not root message");
        return enum_xerror_code_fail;
    }
    if (message.src_node_id().empty() || message.des_node_id().empty()) {
        assert(false);
    }
    assert(message.has_msg_hash());
    // if (!message.has_msg_hash()) {
    //     auto gossip = message.mutable_gossip();
    //     std::string bin_data = message.data();
    //     if (gossip->has_block()) {
    //         bin_data = gossip->block();
    //     }
    //     if (!gossip->has_block() && gossip->has_header_hash()) {
    //         bin_data = gossip->header_hash();
    //     }
    //     uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + bin_data);
    //     message.set_msg_hash(msg_hash);
    // }

    base::ServiceType des_service_type = ParserServiceType(message.des_node_id());
    xdbg("[WrouterXidHandler::SendMulticast] service_type: %lld %s", des_service_type.value(), des_service_type.info().c_str());
    ElectRoutingTablePtr routing_table = FindElectRoutingTable(des_service_type);

    // local does'nt have way to des, using root or find des-nodes first
    if (!routing_table || routing_table->nodes_size() == 0) {
        xdbg("crossing network from local_xid: %s to des: %llu %s", global_xid->Get().c_str(), des_service_type.value(), message.des_node_id().c_str());

        std::vector<kadmlia::NodeInfoPtr> des_nodes;
        if (!wrouter::ServiceNodes::Instance()->GetRootNodes(des_service_type, des_nodes) || des_nodes.empty()) {
            xwarn("crossing network failed, can't find des nodes of service_type: %llu %s", des_service_type.value(), message.des_node_id().c_str());
            return enum_xerror_code_fail;
        }

        xdbg("crossing network begin, des_nodes size: %d", des_nodes.size());
        return SendData(message, des_nodes, 3, true);
    }

    // return GossipBroadcast(message, routing_table);
    uint32_t gossip_type = message.gossip().gossip_type();
    assert(gossip_type == kGossipDispatcher);

    switch (gossip_type) {
    case kGossipDispatcher:
        gossip_dispatcher_ptr_->Broadcast(message, routing_table);
        break;
    default:
        xwarn("invalid gossip_type:%d", gossip_type);
        assert(false);
        break;
    }
    return enum_xcode_successful;
}

int32_t WrouterXidHandler::SendGossip(transport::protobuf::RoutingMessage & message) {
    if (!message.has_is_root() || !message.is_root()) {
        xwarn("SendGossip must be root_msg");
        return enum_xerror_code_fail;
    }
    assert(message.has_msg_hash());
    // if (!message.has_msg_hash()) {
    //     auto gossip = message.mutable_gossip();
    //     std::string bin_data = message.data();
    //     if (gossip->has_block()) {
    //         bin_data = gossip->block();
    //     }
    //     if (!gossip->has_block() && gossip->has_header_hash()) {
    //         bin_data = gossip->header_hash();
    //     }
    //     uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + bin_data);
    //     message.set_msg_hash(msg_hash);
    // }

    RootRoutingTablePtr routing_table;
    routing_table = FindRootRoutingTable();

    xdbg("sendgossip routing_table: %s", (routing_table->get_local_node_info()->kad_key()).c_str());

    if (!routing_table) {
        xwarn("FindRoutingTable failed");
        return enum_xerror_code_fail;
    }

    // return GossipBroadcast(message, routing_table);

    uint32_t gossip_type = message.gossip().gossip_type();
    if (gossip_type == 0) {
        gossip_type = kGossipBloomfilter;
    }
    std::shared_ptr<std::vector<top::kadmlia::NodeInfoPtr>> neighbors;
    assert(gossip_type == kGossipBloomfilter || gossip_type == kGossipRRS);
    neighbors = routing_table->GetUnLockNodes();
    if (!neighbors) {
        xwarn("GetUnLockNodes empty");
        return enum_xerror_code_fail;
    }

    switch (gossip_type) {
    case kGossipBloomfilter:
        bloom_gossip_ptr_->Broadcast(routing_table->get_local_node_info()->hash64(), message, neighbors);
        break;
    case kGossipRRS:
        gossip_rrs_ptr_->Broadcast(routing_table->get_local_node_info()->hash64(), message, neighbors);
        break;
    default:
        xwarn("invalid gossip_type:%d", gossip_type);
        assert(false);
        break;
    }
    return enum_xcode_successful;
}

// int32_t WrouterXidHandler::GossipBroadcast(transport::protobuf::RoutingMessage & message, kadmlia::RoutingTablePtr & routing_table) {
//     uint32_t gossip_type = message.gossip().gossip_type();
//     if (gossip_type == 0) {
//         gossip_type = kGossipBloomfilter;
//     }
//     std::shared_ptr<std::vector<top::kadmlia::NodeInfoPtr>> neighbors;
//     if (gossip_type == kGossipBloomfilter || kGossipRRS) {
//         neighbors = routing_table->GetUnLockNodes();
//         if (!neighbors) {
//             xwarn("GetUnLockNodes empty");
//             return enum_xerror_code_fail;
//         }
//     }
//     switch (gossip_type) {
//     case kGossipBloomfilter:
//         bloom_gossip_ptr_->Broadcast(routing_table->get_local_node_info()->hash64(), message, neighbors);
//         break;
//     case kGossipBloomfilterAndLayered:
//         bloom_layer_gossip_ptr_->Broadcast(message, routing_table);
//         break;
//     case kGossipRRS:
//         gossip_rrs_ptr_->Broadcast(routing_table->get_local_node_info()->hash64(), message, neighbors);
//         break;
//     default:
//         xwarn("invalid gossip_type:%d", gossip_type);
//         assert(false);
//         break;
//     }
//     return enum_xcode_successful;
// }

int32_t WrouterXidHandler::SendData(transport::protobuf::RoutingMessage & message, const std::vector<kadmlia::NodeInfoPtr> & neighbors, uint32_t next_size, bool broadcast_stride) {
    if (neighbors.empty()) {
        xwarn("invliad neighbors");
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
            auto tmp_routing_table = FindRootRoutingTable();
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
    xdbg("finally get destnode size:%u", rest_neighbors.size());
    for (const auto & item : rest_neighbors) {
        xdbg("finally get %s %s:%u", (item->node_id).c_str(), item->public_ip.c_str(), item->public_port);
    }

    std::string data;
    if (!message.SerializeToString(&data)) {
        xwarn("wrouter message SerializeToString failed");
        return enum_xerror_code_fail;
    }
    auto each_call = [this, &data](kadmlia::NodeInfoPtr node_info_ptr) {
        if (!node_info_ptr) {
            xwarn("kadmlia::NodeInfoPtr null");
            return false;
        }
        if (kadmlia::kKadSuccess != transport_ptr_->SendDataWithProp(data, node_info_ptr->public_ip, node_info_ptr->public_port, node_info_ptr->udp_property)) {
            xwarn("SendData to  endpoint(%s:%d) failed", node_info_ptr->public_ip.c_str(), node_info_ptr->public_port);
            return false;
        }
        return true;
    };

    if (message.broadcast()) {
        std::for_each(rest_neighbors.begin(), rest_neighbors.end(), each_call);
    } else {
        std::for_each(neighbors.begin(), neighbors.begin() + 1, each_call);
    }
    return enum_xcode_successful;
}

bool WrouterXidHandler::HandleSystemMessage(transport::protobuf::RoutingMessage & message) {
    static std::vector<int32_t> direct_vec = {
        kKadBootstrapJoinRequest,
        kKadBootstrapJoinResponse,
        kKadFindNodesRequest,
        kKadFindNodesResponse,
        // kKadHeartbeatRequest,
        // kKadHeartbeatResponse,
        kKadHandshake,
        // kKadConnectRequest,

        // kNatDetectRequest,
        // kNatDetectResponse,
        // kNatDetectHandshake2Node,
        // kNatDetectHandshake2Boot,
        // kNatDetectFinish,
        // kUdpNatDetectRequest,
        // kUdpNatDetectResponse,
        // kUdpNatHeartbeat,

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
        // xdbg("wrouter gossip filter finished, continue code");

        if (message.is_root()) {
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYesAndContinue", message);
            return kJudgeOwnYesAndContinue;
        }
        if (message.src_node_id().empty() || message.des_node_id().empty()) {
            assert(false);
        }

        base::ServiceType des_service_type = ParserServiceType(message.des_node_id());

        ElectRoutingTablePtr routing_table = FindElectRoutingTable(des_service_type);
        if (routing_table) {
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYesAndContinue", message);
            return kJudgeOwnYesAndContinue;
        }
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnNoAndContinue", message);
        return kJudgeOwnNoAndContinue;
    }

    // usually only bootstrap message will come here
    if (message.des_node_id().empty()) {
        xdbg("message type(%d) id(%d) des_node_id empty", message.type(), message.id());
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
        return kJudgeOwnYes;
    }

    // if (message.has_is_root() && message.is_root()) {
    if (message.has_is_root() && message.is_root() && message.type() == kRootMessage) {
        // for root routing table message, just for kRootMessage return yes, other root message also need relay
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
        return kJudgeOwnYes;
    }

    base::ServiceType service_type = ParserServiceType(message.des_node_id());

    if (HandleSystemMessage(message)) {
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
        return kJudgeOwnYes;
    }

    // RoutingTablePtr routing_table = nullptr;
    if (message.is_root()) {
        RootRoutingTablePtr routing_table = FindRootRoutingTable();

        if (!routing_table) {
            xwarn("FindRoutingTable failed, judge own packet: type(%d) failed", message.type());
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnNoAndContinue", message);
            return kJudgeOwnNoAndContinue;
        }
        std::string match_kad_id = routing_table->get_local_node_info()->kad_key();

        if (message.des_node_id().compare(match_kad_id) == 0) {
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
            return kJudgeOwnYes;
        }
    } else {
        ElectRoutingTablePtr routing_table = FindElectRoutingTable(service_type);
        if (!routing_table) {
            xwarn("FindRoutingTable failed, judge own packet: type(%d) failed", message.type());
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnNoAndContinue", message);
            return kJudgeOwnNoAndContinue;
        }
        std::string match_kad_id = routing_table->get_local_node_info()->kad_key();
        if (message.des_node_id().compare(match_kad_id) == 0) {
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
            return kJudgeOwnYes;
        }
    }

    // todo charles make it up and ref this function.!
    // bool closest = false;
    // if (routing_table->ClosestToTarget(message.des_node_id(), closest) != kadmlia::kKadSuccess) {
    //     xwarn("ClosestToTarget goes wrong");
    //     TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnError", message);
    //     return kJudgeOwnError;
    // }
    // if (closest) {
    //     TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
    //     return kJudgeOwnYes;
    // }

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
