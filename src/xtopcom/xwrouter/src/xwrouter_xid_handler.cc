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
#include "xwrouter/xwrouter_error.h"

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
    auto routing_table = MultiRouting::Instance()->GetElectRoutingTable(service_type);
    // when recv a message. it not possible to duduce service ver from dst xip.
    // thus at version 1 (NOT 2). should add compatablity with two version. 
    if (!routing_table) {
        service_type.set_ver(base::service_type_height_use_version);// version two
        // service_type.set_ver(base::service_type_height_use_blk_height); // version one
        routing_table = MultiRouting::Instance()->GetElectRoutingTable(service_type);
    }
    return routing_table;
    // return MultiRouting::Instance()->GetElectRoutingTable(service_type);
}
kadmlia::RootRoutingTablePtr WrouterXidHandler::FindRootRoutingTable() {
    return MultiRouting::Instance()->GetRootRoutingTable();
}

void WrouterXidHandler::SendPacket(transport::protobuf::RoutingMessage & message, std::error_code & ec) {
    // assert(message.has_msg_hash());
    if (message.hop_num() >= kadmlia::kHopToLive) {
        ec = xwrouter::xwrouter_error_t::hop_num_beyond_max, xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    // root broadcast might have empyt src && dst node id
    if (BroadcastPacketCheck(message)) {
        SendBroadcast(message, ec);
        return;
    }

    if (message.des_node_id().empty()) {
        ec = xwrouter::xwrouter_error_t::empty_dst_address, xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
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
                ec = xwrouter::xwrouter_error_t::not_find_routing_table, xwarn("%s %s", ec.category().name(), ec.message().c_str());
                return;
            }
            message.set_src_node_id(elect_routing_table->get_local_node_info()->kad_key());
        }
    }

    if (RumorPacketCheck(message)) {
        SendRumor(message, ec);
        return;
    }

    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("SendPacket base xid", message);
    SendGeneral(message, ec);
    return;
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
        std::error_code ec;
        SendPacket(message, ec);
        return kRecvOk;
    }
    case kJudgeOwnYesAndContinue: {
        std::error_code ec;
        SendPacket(message, ec);
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

void WrouterXidHandler::SendGeneral(transport::protobuf::RoutingMessage & message, std::error_code & ec) {
    assert(!ec);
    if (message.des_node_id().empty()) {
        assert(false);
    }

    base::ServiceType service_type = ParserServiceType(message.des_node_id());
    // RoutingTablePtr routing_table = nullptr;
    if (message.has_is_root() && message.is_root()) {
        RootRoutingTablePtr routing_table = FindRootRoutingTable();
        if (!routing_table) {
            ec = xwrouter::xwrouter_error_t::not_find_routing_table, xwarn("%s %s", ec.category().name(), ec.message().c_str());
            return;
        }

        xdbg("sendgeneral using routing_table: %s", (routing_table->get_local_node_info()->kad_key()).c_str());

        std::string des_xid = message.des_node_id();
        routing_table->GetClosestNodes(des_xid, 8);
        std::vector<kadmlia::NodeInfoPtr> nodes = routing_table->GetClosestNodes(des_xid, 8);
        if (nodes.empty()) {
            ec = xwrouter::xwrouter_error_t::routing_table_find_closest_nodes_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
            return;
        }
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("SendData", message);
        SendData(message, nodes, kBroadcastGeneral, false, ec);
        return;
    } else {
        ElectRoutingTablePtr routing_table = FindElectRoutingTable(service_type);
        if (!routing_table) {
            xdbg("FindRoutingTable failed of service_type: %s, try crossing network", service_type.info().c_str());

            xdbg("crossing network from local_xid: %s to des: %s", (global_xid->Get()).c_str(), service_type.info().c_str());

            std::vector<kadmlia::NodeInfoPtr> des_nodes;
            kadmlia::NodeInfoPtr des_node_ptr;
            if (!wrouter::ServiceNodes::Instance()->GetRootNodes(service_type, message.des_node_id(), des_node_ptr) || !des_node_ptr) {
                xwarn("crossing network failed, can't find des nodes of service_type: %s des_node_id: %s", service_type.info().c_str(),message.des_node_id().c_str());
                ec = xwrouter::xwrouter_error_t::crossing_network_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
                return;
            }

            des_nodes.push_back(des_node_ptr);
            xdbg("crossing network begin, des_nodes size: %d", des_nodes.size());
            SendData(message, des_nodes, kBroadcastGeneral, false, ec);
            return;
        }

        xdbg("sendgeneral using routing_table: %s", (routing_table->get_local_node_info()->kad_key()).c_str());

        // no root ,no broadcast p2p 1159
        std::string des_xid = message.des_node_id();
        std::vector<kadmlia::NodeInfoPtr> nodes;
        // routing_table->GetRandomNodes(nodes,8);
        nodes.push_back(routing_table->GetNode(des_xid));

        if (nodes.empty()) {
            ec = xwrouter::xwrouter_error_t::routing_table_find_closest_nodes_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
            return;
        }
        TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("SendData", message);
        SendData(message, nodes, kBroadcastGeneral, false, ec);
    }
}

void WrouterXidHandler::SendRumor(transport::protobuf::RoutingMessage & message, std::error_code & ec) {
    assert(message.has_is_root() && message.is_root() == false);
    assert(!message.src_node_id().empty());
    assert(!message.des_node_id().empty());
    assert(message.has_msg_hash());
    assert(!ec);

    base::ServiceType des_service_type = ParserServiceType(message.des_node_id());
    xdbg("[WrouterXidHandler::SendRumor] service_type: %s %s", des_service_type.info().c_str(), des_service_type.info().c_str());
    ElectRoutingTablePtr routing_table = FindElectRoutingTable(des_service_type);

    // local does'nt have way to des, using root or find des-nodes first
    if (!routing_table || routing_table->nodes_size() == 0) {
        xdbg("crossing network from local_xid: %s to des: %s %s", global_xid->Get().c_str(), des_service_type.info().c_str(), message.des_node_id().c_str());

        std::vector<kadmlia::NodeInfoPtr> des_nodes;
        if (!wrouter::ServiceNodes::Instance()->GetRootNodes(des_service_type, des_nodes) || des_nodes.empty()) {
            xwarn("crossing network failed, can't find des nodes of service_type: %s %s", des_service_type.info().c_str(), message.des_node_id().c_str());
            ec = xwrouter::xwrouter_error_t::crossing_network_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
            return;
        }

        xdbg("crossing network begin, des_nodes size: %d", des_nodes.size());
        SendData(message, des_nodes, 3, true, ec);
        return;
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
    return;
}

void WrouterXidHandler::SendBroadcast(transport::protobuf::RoutingMessage & message, std::error_code & ec) {
    assert(message.has_is_root() && message.is_root());
    // assert(message.has_msg_hash());
    assert(!ec);

    RootRoutingTablePtr routing_table;
    routing_table = FindRootRoutingTable();

    xdbg("sendgossip routing_table: %s", (routing_table->get_local_node_info()->kad_key()).c_str());

    if (!routing_table) {
        ec = xwrouter::xwrouter_error_t::not_find_routing_table, xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
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
        ec = xwrouter::xwrouter_error_t::routing_table_find_closest_nodes_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
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
    return;
}

void WrouterXidHandler::SendData(transport::protobuf::RoutingMessage & message, const std::vector<kadmlia::NodeInfoPtr> & neighbors, uint32_t next_size, bool broadcast_stride, std::error_code & ec) {
    assert(!ec);
    if (neighbors.empty()) {
        ec = xwrouter::xwrouter_error_t::routing_table_find_closest_nodes_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
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
    std::size_t success_send_count = 0;
    if (!message.SerializeToString(&data)) {
        ec = xwrouter::xwrouter_error_t::serialized_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }
    auto each_call = [this, &data, &success_send_count](kadmlia::NodeInfoPtr node_info_ptr) {
        if (!node_info_ptr) {
            xwarn("kadmlia::NodeInfoPtr null");
            return;
        }
        if (kadmlia::kKadSuccess != transport_ptr_->SendDataWithProp(data, node_info_ptr->public_ip, node_info_ptr->public_port, node_info_ptr->udp_property)) {
            xwarn("SendData to  endpoint(%s:%d) failed", node_info_ptr->public_ip.c_str(), node_info_ptr->public_port);
            return;
        }
        success_send_count = success_send_count + 1;
        return;
    };

    if (message.broadcast()) {
        if (rest_neighbors.empty()) {
            ec = xwrouter::xwrouter_error_t::routing_table_find_closest_nodes_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
            return;
        }
        std::for_each(rest_neighbors.begin(), rest_neighbors.end(), each_call);

        if (success_send_count < rest_neighbors.size()) {
            ec = xwrouter::xwrouter_error_t::multi_send_partial_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
        }
    } else {
        std::for_each(neighbors.begin(), neighbors.begin() + 1, each_call);
        if (success_send_count < neighbors.size()) {
            ec = xwrouter::xwrouter_error_t::multi_send_partial_fail, xwarn("%s %s", ec.category().name(), ec.message().c_str());
        }
    }
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
            xwarn("FindRoutingTable failed, judge own packet: type(%d) failed service_type: %s", message.type(), service_type.info().c_str());
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnNoAndContinue", message);
            return kJudgeOwnNoAndContinue;
        }
        std::string match_kad_id = routing_table->get_local_node_info()->kad_key();
        if (message.des_node_id().compare(match_kad_id) == 0) {
            TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter kJudgeOwnYes", message);
            return kJudgeOwnYes;
        }
        // for compatibility , here should once more do fuzzy identification :
        if (base::GetKadmliaKey(message.des_node_id())->slot_id() == routing_table->get_local_node_info()->kadmlia_key()->slot_id()) {
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

bool WrouterXidHandler::RumorPacketCheck(transport::protobuf::RoutingMessage & message) {
    if (!message.has_broadcast() || !message.broadcast()) {
        return false;
    }

    if (message.has_is_root() && message.is_root()) {
        return false;
    }
    // broadcast to same network or different network
    return true;
}

bool WrouterXidHandler::BroadcastPacketCheck(transport::protobuf::RoutingMessage & message) {
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
