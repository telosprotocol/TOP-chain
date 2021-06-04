// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/root/root_routing.h"

#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/routing_table.h"
#include "xwrouter/register_routing_table.h"
#include "xkad/routing_table/node_detection_manager.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/sem.h"
#include "xpbase/base/top_string_util.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"
#include "xtransport/udp_transport/transport_util.h"
#include "xpbase/base/top_log_name.h"

namespace top {

using namespace kadmlia;

namespace wrouter {

static const int32_t kGetNodesTimeout = 3;
static const uint32_t kGetNodesSize = 8;

RootMessageHandler RootRouting::root_message_handler_;

RootRouting::RootRouting(
        std::shared_ptr<transport::Transport> transport,
        kadmlia::LocalNodeInfoPtr local_node_ptr)
        : WrouterBaseRouting(transport, kNodeIdSize, local_node_ptr), root_id_set_(), root_id_set_mutex_() {}

RootRouting::~RootRouting() {}

bool RootRouting::UnInit() {
    if (local_node_ptr_->service_type() != base::ServiceType(kRoot)) {
        RoutingTablePtr root_routing_ptr = GetRoutingTable(base::ServiceType(kRoot), true);
        if (root_routing_ptr) {
            RootRouting* root = dynamic_cast<RootRouting*>(root_routing_ptr.get());
            if (root != nullptr) {
                root->RemoveNetworkRootId(local_node_ptr_->id());
            }
        }
    }
    // if (local_node_ptr_->service_type() == kRoot) {
    //     UnRegisterBootstrapCacheCallback();
    // }
    return WrouterBaseRouting::UnInit();
}

void RootRouting::RemoveNetworkRootId(const std::string& root_id) {
    std::unique_lock<std::mutex> lock(root_id_set_mutex_);
    auto iter = root_id_set_.find(root_id);
    if (iter != root_id_set_.end()) {
        root_id_set_.erase(iter);
    }
}

void RootRouting::AddNetworkRootId(const std::string& root_id) {
    std::unique_lock<std::mutex> lock(root_id_set_mutex_);
    root_id_set_.insert(root_id);
}

void RootRouting::SetFreqMessage(transport::protobuf::RoutingMessage& message) {
    WrouterBaseRouting::SetFreqMessage(message);
    message.set_is_root(true);
}

bool RootRouting::ContainRootId(const std::string& id) {
    assert(local_node_ptr_->service_type() == base::ServiceType(kRoot));
    std::unique_lock<std::mutex> lock(root_id_set_mutex_);
    auto iter = root_id_set_.find(id);
    return iter != root_id_set_.end();
}

bool RootRouting::NewNodeReplaceOldNode(NodeInfoPtr node, bool remove) {
    return WrouterBaseRouting::NewNodeReplaceOldNode(node, remove);
}


int RootRouting::AddNode(NodeInfoPtr node) {
    int res = WrouterBaseRouting::AddNode(node);
    if (res != kKadSuccess) {
        return res;
    }

    return res;
}

int RootRouting::DropNode(NodeInfoPtr node) {
    int res = WrouterBaseRouting::DropNode(node);
    if (res != kKadSuccess) {
        return res;
    }

    return res;
}


RoutingTablePtr RootRouting::FindRoutingTable(const std::string& msg_des_node_id) {
    std::vector<base::ServiceType> vec_type;
    GetAllRegisterType(vec_type);
    TOP_DEBUG("GetAllRegisterType size %d", vec_type.size());
    auto tmp_routing_table1 = GetRoutingTable(base::ServiceType{kRoot}, true);
    auto target_routing_table = tmp_routing_table1;
    if (tmp_routing_table1) {
        std::string tmp_id1 = tmp_routing_table1->get_local_node_info()->id();
        for (auto& tmp_service_type : vec_type) {
            auto tmp_routing_table2 = GetRoutingTable(tmp_service_type, true);
            if (!tmp_routing_table2) {
                TOP_WARN2("GetRoutingTable %llu empty", tmp_service_type.value());
                continue;
            }
            std::string tmp_id2 = tmp_routing_table2->get_local_node_info()->id();
            if (!CloserToTarget(tmp_id1, tmp_id2, msg_des_node_id)) {
                tmp_id1 = tmp_id2;
                tmp_routing_table1 = tmp_routing_table2;
            }
        } // end for
        target_routing_table = tmp_routing_table1;
    }

    if (target_routing_table) {
        TOP_DEBUG("FindRoutingTable xnetwork_id:%d msg.des_node_id:%s",
                target_routing_table->get_local_node_info()->kadmlia_key()->xnetwork_id(),
                HexEncode(msg_des_node_id).c_str());
        return target_routing_table;
    }
    return nullptr;
}

// bool RootRouting::GetRootNodesFromLocalRootRouting(
//         kadmlia::RoutingTablePtr root_routing,
//         const std::string& node_id,
//         std::vector<kadmlia::NodeInfoPtr>& nodes) {
//     if (!root_routing) {
//         return false;
//     }
//     base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(node_id);
//     auto des_routing = root_routing;
//     auto local_nodes = des_routing->nodes();
//     for (uint32_t i = 0; i < local_nodes.size(); ++i) {
//         if (nodes.size() >= kGetNodesSize) {
//             break;
//         }
//         auto tmp_kad_key = base::GetKadmliaKey(local_nodes[i]->node_id);
//         uint64_t node_service_type = tmp_kad_key->GetServiceType();
//         if (kad_key->GetServiceType() != node_service_type) {
//             continue;
//         }
//         if (local_nodes[i]->public_ip == des_routing->get_local_node_info()->public_ip() &&
//                 local_nodes[i]->public_port == des_routing->get_local_node_info()->public_port()) {
//             continue;
//         }
//         nodes.push_back(local_nodes[i]);
//     }
//     if (nodes.size() > 0) {
//         return true;
//     }
//     return false;
// }

// bool RootRouting::GetRootNodesFromLocal(const std::string& node_id, std::vector<kadmlia::NodeInfoPtr>& nodes) {
//     base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(node_id);
//     auto des_routing = GetRoutingTable(kad_key->GetServiceType(), true);
//     if (des_routing) {
//         if (GetRootNodesFromLocalRootRouting(des_routing, node_id, nodes)) {
//             TOP_DEBUG("getrootnodes found des_routing local, des_node_id(%s)", HexEncode(node_id).c_str());
//             return true;
//         }
//         return false;
//     } // end if (des_routing...

//     // no des_node_id service_type
//     std::vector<uint64_t> vec_type;
//     GetAllRegisterType(vec_type);
//     for (auto& tmp_service_type : vec_type) {
//         auto tmp_routing_table = GetRoutingTable(tmp_service_type, true);
//         if (!tmp_routing_table) {
//             continue;
//         }
//         if (GetRootNodesFromLocalRootRouting(tmp_routing_table, node_id, nodes)) {
//             TOP_DEBUG("getrootnodes found random-des_routing local, des_node_id(%s)", HexEncode(node_id).c_str());
//             return true;
//         }
//     } // end for
//     return false;
// }

// int RootRouting::GetRootNodes(const std::string& node_id, std::vector<NodeInfoPtr>& nodes) {
//     if (GetRootNodesFromLocal(node_id, nodes)) {
//         // TOP_DEBUG("getrootnodes %d from local, des_node_id(%s)", nodes.size(), HexEncode(node_id).c_str());
//         // std::cout << "getrootnodes "<< nodes.size() <<" from local, continue sendto remote" << std::endl;
//     }

//     transport::protobuf::RoutingMessage message;
//     SetFreqMessage(message);
//     message.set_des_service_type(kRoot);
//     message.set_des_node_id(node_id);
//     message.set_type(kRootMessage);
//     message.set_id(CallbackManager::MessageId());
//     message.set_xid(global_xid->Get());
// #ifndef NDEBUG
//     auto debug_info = base::StringUtil::str_fmt(
//             "root routing get nodes, [id: %u] [src: %s], [des: %s] ",
//             message.id(),
//             HexEncode(local_node_ptr_->id()).c_str(),
//             HexEncode(node_id).c_str());
//     message.set_debug(debug_info);
//     TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("root_get_nodes_begin", message);
// #endif

//     protobuf::RootGetNodesRequest get_nodes_req;
//     get_nodes_req.set_id(node_id);
//     get_nodes_req.set_count(kGetNodesSize);
//     std::string data;
//     if (!get_nodes_req.SerializeToString(&data)) {
//         TOP_WARN("GetNearestNodesRequest SerializeToString failed!");
//         return kKadFailed;
//     }

//     protobuf::RootMessage root_message;
//     root_message.set_message_type(kGetNodesRequest);
//     root_message.set_data(data);
//     std::string root_data;
//     if (!root_message.SerializeToString(&root_data)) {
//         TOP_INFO("RootMessage SerializeToString failed!");
//         return kKadFailed;
//     }

//     message.set_data(root_data);
//     const int N = 3;
//     SendToNClosestNode(message, N);
//     using RoutingMessagePtr = std::shared_ptr<transport::protobuf::RoutingMessage>;
//     using PacketPtr = std::shared_ptr<base::xpacket_t>;
//     struct Item {
//         int status;
//         RoutingMessagePtr message;
//         PacketPtr packet;
//     };
//     struct Queue {
//         std::mutex msgs_mutex;
//         std::list<Item> msgs;
//         base::Sem sem;
//     };
//     auto q = std::make_shared<Queue>();
//     int res = kKadFailed;
//     auto callback = [q](
//             int status, transport::protobuf::RoutingMessage& message, base::xpacket_t& packet) {
//         // TOP_FATAL("[%d] recv response from %s:%d", (int)message.id(), packet.get_from_ip_addr().c_str(), (int)packet.get_from_ip_port());
//         auto message_ptr = std::make_shared<transport::protobuf::RoutingMessage>();
//         message_ptr->CopyFrom(message);
//         auto packet_ptr = std::make_shared<base::xpacket_t>();
//         packet_ptr->copy_from(packet);
//         {
//             std::unique_lock<std::mutex> lock(q->msgs_mutex);
//             q->msgs.push_back(Item{status, message_ptr, packet_ptr});
//         }
//         q->sem.Post();
//     };

//     // register timeout
//     CallbackManager::Instance()->Add(message.id(), kGetNodesTimeout, callback, N);

//     // until success or timeout
//     int received_msg = 0;
//     for (;;) {
//         q->sem.Pend();
//         Item item;
//         {
//             std::unique_lock<std::mutex> lock(q->msgs_mutex);
//             assert(!q->msgs.empty());
//             item = q->msgs.front();
//             q->msgs.pop_front();
//         }

//         auto& status = item.status;
//         auto& message = *item.message;
//         // auto& packet = *item.packet;
//         if (status == kKadSuccess) {
//             received_msg += 1;
//             do {
//                 if (!message.has_data() || message.data().empty()) {
//                     TOP_WARN("message has no data!");
//                     break;
//                 }
//                 protobuf::RootMessage get_nodes_res;
//                 if (!get_nodes_res.ParseFromString(message.data())) {
//                     TOP_WARN("message ParseFromString failed!");
//                     break;
//                 }
//                 if (!get_nodes_res.has_data() && get_nodes_res.data().empty()) {
//                     TOP_WARN("message root message has no data!");
//                     break;
//                 }
//                 protobuf::RootGetNodesResponse nodes_res;
//                 if (!nodes_res.ParseFromString(get_nodes_res.data())) {
//                     TOP_WARN("message root message ParseFromString!");
//                     break;
//                 }
//                 for (int i = 0; i < nodes_res.nodes_size(); ++i) {
//                     NodeInfoPtr node_ptr;
//                     node_ptr.reset(new NodeInfo(nodes_res.nodes(i).id()));
//                     node_ptr->public_ip = nodes_res.nodes(i).public_ip();
//                     node_ptr->public_port = nodes_res.nodes(i).public_port();
//                     node_ptr->local_ip = nodes_res.nodes(i).local_ip();
//                     node_ptr->local_port = nodes_res.nodes(i).local_port();
//                     nodes.push_back(node_ptr);
//                 }
//                 CallbackManager::Instance()->Remove(message.id());
//                 return kKadSuccess;
//             } while (0);

//             if (received_msg >= N) {
//                 return kKadFailed;
//             }
//         } else if (status == kKadTimeout) {
//             return kKadFailed;
//         }
//     }

//     return kKadFailed;
// }

// int RootRouting::GetRootNodes(uint64_t service_type, std::vector<NodeInfoPtr>& nodes) {
//     std::vector<std::string> exclude;
//     base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(service_type);
//     return GetRootNodes(kad_key->Get(), nodes);
// }

bool RootRouting::Init() {
    local_node_ptr_->set_is_root(true);
    if (!WrouterBaseRouting::Init()) {
        TOP_ERROR("WrouterBaseRouting::Init failed");
        return false;
    }
    assert(local_node_ptr_->kadmlia_key()->xnetwork_id() == kRoot);
    local_node_ptr_->set_kadmlia_key(global_xid);
    // if (!StartBootstrapCacheSaver()) {
    //     TOP_ERROR("WrouterBaseRouting::StartBootstrapCacheSaver failed");
    //     return false;
    // }
    AddNetworkRootId(local_node_ptr_->id());

    TOP_INFO("root routing table Init success");
    return true;
}

int RootRouting::Bootstrap(
        const std::string& peer_ip,
        uint16_t peer_port,
        base::ServiceType des_service_type) {
    return WrouterBaseRouting::Bootstrap(peer_ip, peer_port, base::ServiceType(kRoot));
}

// void RootRouting::HandleRootGetNodesRequest(
//         transport::protobuf::RoutingMessage& message,
//         base::xpacket_t& packet) {
//     TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("handle request", message);
//     base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(message.des_node_id());
//     uint64_t node_service_type = kad_key->GetServiceType();
//     if (message.des_node_id() != local_node_ptr_->id()) {
//         bool closest = false;
//         std::set<std::string> exclude;
//         exclude.insert(message.src_node_id());
//         RoutingTablePtr target_routing = FindRoutingTable(message.des_node_id());
//         NodeInfoPtr closest_node(target_routing->GetClosestNode(message.des_node_id(), false, exclude));
//         if (closest_node) {
//             TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE(
//                     std::string("close: ") + HexEncode(closest_node->node_id) +
//                     std::string(":") + HexEncode(message.des_node_id()), message);
//             closest = CloserToTarget(target_routing->get_local_node_info()->id(), closest_node->node_id, message.des_node_id());
//         } else {
//             closest = false;
//         } // end if (closest_node)

//         if (!closest) {
//             TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("sendto close request", message);
//             RoutingTablePtr target_routing = FindRoutingTable(message.des_node_id());
//             if (!target_routing) {
//                 TOP_WARN("FindRoutingTable failed");
//                 return;
//             }
//             return target_routing->SendToClosestNode(message, false);
//         }

//         TOP_INFO("this is the closest node(%s) of msg.des_node_id(%s)",
//                 HexEncode(target_routing->get_local_node_info()->id()).c_str(),
//                 HexEncode(message.des_node_id()).c_str());
//         if (target_routing->get_local_node_info()->kadmlia_key()->GetServiceType() != node_service_type) {
//             TOP_WARN("target routing table service_type:%llu not equal des_service_type:%llu",
//                     target_routing->get_local_node_info()->kadmlia_key()->GetServiceType(),
//                     node_service_type);
//             return;
//         }
//     }

//     if (!message.has_data() || message.data().empty()) {
//         TOP_WARN("HandleGetGroupNodesRequest has no data!");
//         return;
//     }

//     protobuf::RootMessage root_message;
//     if (!root_message.ParseFromString(message.data())) {
//         TOP_WARN("RootMessage ParseFromString from string failed!");
//         return;
//     }

//     protobuf::RootGetNodesRequest get_nodes_req;
//     if (!get_nodes_req.ParseFromString(root_message.data())) {
//         TOP_WARN("RootGetNodesRequest ParseFromString failed!");
//         return;
//     }

//     std::vector<NodeInfoPtr> nodes;
//     RoutingTablePtr routing_table = GetRoutingTable(node_service_type, true);
//     if (!routing_table) {
//         TOP_WARN("GetRoutingTable failed for service_type:%llu", node_service_type);
//         return;
//     }
//     nodes = routing_table->GetClosestNodes(get_nodes_req.id(), get_nodes_req.count() - 1);
//     auto local_node_ptr = routing_table->get_local_node_info();
//     if (!local_node_ptr) {
//         local_node_ptr = local_node_ptr_;
//     }
//     if (local_node_ptr->kadmlia_key()->GetServiceType() != node_service_type) {
//         return;
//     }

//     transport::protobuf::RoutingMessage res_message;
// #ifndef NDEBUG
//     if (message.has_debug()) {
//         res_message.set_debug(message.debug());
//     }
// #endif

//     SetFreqMessage(res_message);
//     res_message.set_src_service_type(message.des_service_type());
//     res_message.set_des_service_type(message.src_service_type());
//     res_message.set_des_node_id(message.src_node_id());
//     res_message.set_type(kRootMessage);
//     res_message.set_id(message.id());
//     protobuf::RootGetNodesResponse get_nodes_res;
//     if (local_node_ptr) {
//         protobuf::NodeInfo* node_info = get_nodes_res.add_nodes();
//         node_info->set_id(local_node_ptr->id());
//         node_info->set_public_ip(local_node_ptr->public_ip());
//         node_info->set_public_port(local_node_ptr->public_port());
//         node_info->set_local_ip(local_node_ptr->local_ip());
//         node_info->set_local_port(local_node_ptr->local_port());
//     }

//     auto tmp_ready_nodes = 0;
//     for (uint32_t i = 0; i < nodes.size(); ++i) {
//         if (static_cast<uint32_t>(get_nodes_res.nodes_size()) >= get_nodes_req.count()) {
//             break;
//         }

//         if (nodes[i]->node_id == message.des_node_id()) {
//             continue;
//         }
//         if (nodes[i]->xid == message.xid()) {
//             continue;
//         }
//         auto tmp_kad_key = base::GetKadmliaKey(nodes[i]->node_id);
//         if (tmp_kad_key->GetServiceType() != node_service_type) {
//             continue;
//         }
//         protobuf::NodeInfo* node_info = get_nodes_res.add_nodes();
//         node_info->set_id(nodes[i]->node_id);
//         node_info->set_public_ip(nodes[i]->public_ip);
//         node_info->set_public_port(nodes[i]->public_port);
//         node_info->set_local_ip(nodes[i]->local_ip);
//         node_info->set_local_port(nodes[i]->local_port);
//         ++tmp_ready_nodes;
//     }
//     TOP_DEBUG("nodes:%d ready_nodes:%d filtered:%d", nodes.size(), tmp_ready_nodes, nodes.size()-tmp_ready_nodes);

//     std::string data;
//     if (!get_nodes_res.SerializeToString(&data)) {
//         TOP_WARN("RootGetNodesResponse SerializeToString failed!");
//         return;
//     }

//     TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE(
//             std::string("response: ") + std::to_string(get_nodes_res.nodes_size()),
//             message);
//     protobuf::RootMessage root_res_message;
//     root_res_message.set_message_type(kGetNodesResponse);
//     root_res_message.set_data(data);
//     std::string root_data;
//     if (!root_res_message.SerializeToString(&root_data)) {
//         TOP_WARN("RootMessage SerializeToString failed!");
//         return;
//     }

//     res_message.set_data(root_data);
//     if (ContainRootId(res_message.des_node_id())) {
//         CallbackManager::Instance()->Callback(res_message.id(), res_message, packet);
//         return;
//     }

//     RoutingTablePtr target_routing = FindRoutingTable(res_message.des_node_id());
//     if (!target_routing) {
//         TOP_WARN("FindRoutingTable failed");
//         return;
//     }
//     target_routing->SendToClosestNode(res_message);
//     return;
// }

// void RootRouting::HandleRootGetNodesResponse(
//         transport::protobuf::RoutingMessage& message,
//         base::xpacket_t& packet) {
//     if (message.des_node_id() != local_node_ptr_->id()) {
//         RoutingTablePtr target_routing = FindRoutingTable(message.des_node_id());
//         if (!target_routing) {
//             TOP_WARN("FindRoutingTable failed");
//             return;
//         }
//         return target_routing->SendToClosestNode(message);
//     }

//     CallbackManager::Instance()->Callback(message.id(), message, packet);
// }

void RootRouting::HandleMessage(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    if (message.type() != kRootMessage) {
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        TOP_WARN("connect request in data is empty.");
        return;
    }

    protobuf::RootMessage root_message;
    if (!root_message.ParseFromString(message.data())) {
        TOP_WARN("ConnectRequest ParseFromString from string failed!");
        return;
    }

    switch (root_message.message_type()) {
    // case kGetNodesRequest:
    //     return HandleRootGetNodesRequest(message, packet);
    // case kGetNodesResponse:
    //     return HandleRootGetNodesResponse(message, packet);
    case kGetElectNodesRequest:
        return HandleGetElectNodesRequest(message, packet);
    case kGetElectNodesResponse:
        return HandleGetElectNodesResponse(message, packet);
    default:
        TOP_WARN("invalid root message type[%d].", root_message.message_type());
        break;
    }
}

// bool RootRouting::StartBootstrapCacheSaver() {
//     auto get_public_nodes = [this](std::vector<NodeInfoPtr>& nodes) {
//         {
//             NodesLock lock(nodes_mutex_);
//             for (auto& node_ptr : nodes_) {
//                 if (node_ptr->IsPublicNode())
//                     nodes.push_back(node_ptr);
//             }
//         }

//         if (!bootstrap_ip_.empty() && bootstrap_port_ >= 0) {
//             auto node_ptr = std::make_shared<NodeInfo>();
//             node_ptr->public_ip = bootstrap_ip_;
//             node_ptr->public_port = bootstrap_port_;
//             nodes.push_back(node_ptr);
//         }
//     };

//     auto get_service_public_nodes = [this](uint64_t service_type, std::vector<NodeInfoPtr>& nodes) {
//         std::vector<NodeInfoPtr> tmp_nodes;
//         base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(service_type);
//         if (GetRootNodes(kad_key->Get(), tmp_nodes) != kadmlia::kKadSuccess) {
//             TOP_WARN("<StartBootstrapCacheSaver:: get root nodes failed for %llu", service_type);
//             return;
//         }
//         // just keep public nodes of service_type
//         for (auto& node_ptr : tmp_nodes) {
//             if (node_ptr->IsPublicNode()) {
//                 nodes.push_back(node_ptr);
//             }
//         }
//         return;
//     };

//     if (!bootstrap_cache_helper_->Start(
//             local_node_ptr_->kadmlia_key(),
//             get_public_nodes,
//             get_service_public_nodes)) {
//         TOP_ERROR("boostrap_cache_helper start failed");
//         return false;
//     }

//     TOP_INFO("bootstrap_cache_helper start success");
//     return true;
// }

// bool RootRouting::GetCacheServicePublicNodes(
//         uint64_t service_type,
//         std::set<std::pair<std::string, uint16_t>>& boot_endpoints) {
//     return bootstrap_cache_helper_->GetCacheServicePublicNodes(service_type, boot_endpoints);
// }

// bool RootRouting::SetCacheServiceType(uint64_t service_type) {
//     return bootstrap_cache_helper_->SetCacheServiceType(service_type);
// }

// get elect nodes from root-network base elect-data
int RootRouting::GetRootNodesV2(
        const std::string& des_kroot_id,
        base::ServiceType des_service_type,
        std::vector<kadmlia::NodeInfoPtr>& nodes) {
    transport::protobuf::RoutingMessage message;
    SetFreqMessage(message);
    message.set_des_service_type(kRoot);
    message.set_des_node_id(des_kroot_id);
    message.set_type(kRootMessage);
    message.set_id(CallbackManager::MessageId());
    message.set_xid(global_xid->Get());
#ifndef NDEBUG
    auto debug_info = base::StringUtil::str_fmt(
            "root routing get nodes, [id: %u] [src: %s], [des: %s] ",
            message.id(),
            local_node_ptr_->id().c_str(),
            des_kroot_id.c_str());
    message.set_debug(debug_info);
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("root_get_nodes_begin", message);
#endif

    protobuf::RootGetElectNodesRequest get_nodes_req;
    get_nodes_req.set_des_service_type(des_service_type.value());
    get_nodes_req.set_count(kGetNodesSize);
    std::string data;
    if (!get_nodes_req.SerializeToString(&data)) {
        TOP_WARN("RootGetElectNodesRequest SerializeToString failed!");
        return kKadFailed;
    }

    protobuf::RootMessage root_message;
    root_message.set_message_type(kGetElectNodesRequest);
    root_message.set_data(data);
    std::string root_data;
    if (!root_message.SerializeToString(&root_data)) {
        TOP_INFO("RootMessage SerializeToString failed!");
        return kKadFailed;
    }
    message.set_data(root_data);
    TOP_DEBUG("%s GetRootNodesV2 sending message", transport::FormatMsgid(message).c_str());
    SendToClosestNode(message, false);
    base::Sem sem;
    int res = kKadFailed;
    auto callback = [&res, &sem, &nodes, &des_kroot_id](
            int status, transport::protobuf::RoutingMessage& message, base::xpacket_t& packet) {
        TOP_DEBUG("%s GetRootNodesV2 callback", transport::FormatMsgid(message).c_str());
        if (status == kKadSuccess) {
            do {
                if (!message.has_data() || message.data().empty()) {
                    TOP_WARN("%s message has no data!", transport::FormatMsgid(message).c_str());
                    break;
                }
                protobuf::RootMessage get_nodes_res;
                if (!get_nodes_res.ParseFromString(message.data())) {
                    TOP_WARN("%s message ParseFromString failed!", transport::FormatMsgid(message).c_str());
                    break;
                }
                if (!get_nodes_res.has_data() && get_nodes_res.data().empty()) {
                    TOP_WARN("%s message root message has no data!", transport::FormatMsgid(message).c_str());
                    break;
                }
                protobuf::RootGetNodesResponse nodes_res;
                if (!nodes_res.ParseFromString(get_nodes_res.data())) {
                    TOP_WARN("%s message root message ParseFromString!", transport::FormatMsgid(message).c_str());
                    break;
                }
                for (int i = 0; i < nodes_res.nodes_size(); ++i) {
                    NodeInfoPtr node_ptr;
                    node_ptr.reset(new NodeInfo(nodes_res.nodes(i).id()));
                    node_ptr->public_ip = nodes_res.nodes(i).public_ip();
                    node_ptr->public_port = nodes_res.nodes(i).public_port();
                    node_ptr->local_ip = nodes_res.nodes(i).local_ip();
                    node_ptr->local_port = nodes_res.nodes(i).local_port();
                    node_ptr->xid = des_kroot_id;
                    node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
                    nodes.push_back(node_ptr);
                }
                res = kKadSuccess;
            } while (0);
        } else {
            TOP_DEBUG("%s GetRootNodesV2 timeout", transport::FormatMsgid(message).c_str());
        }
        sem.Post();
    };
    CallbackManager::Instance()->Add(message.id(), kGetNodesTimeout, callback, 1);
    sem.Pend();
    return res;
}

// get elect nodes from root-network base elect-data
int RootRouting::GetRootNodesV2Async(
        const std::string& des_kroot_id,
        base::ServiceType des_service_type,
        GetRootNodesV2AsyncCallback cb) {
    TOP_DEBUG_NAME("bluerootasync routing start");
    transport::protobuf::RoutingMessage message;
    SetFreqMessage(message);
    message.set_des_service_type(kRoot);
    message.set_des_node_id(des_kroot_id);
    message.set_type(kRootMessage);
    message.set_id(CallbackManager::MessageId());
    message.set_xid(global_xid->Get());
#ifndef NDEBUG
    auto debug_info = base::StringUtil::str_fmt(
            "root routing get nodes, [id: %u] [src: %s], [des: %s] ",
            message.id(),
            HexEncode(local_node_ptr_->id()).c_str(),
            HexEncode(des_kroot_id).c_str());
    message.set_debug(debug_info);
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("root_get_nodes_begin", message);
#endif

    protobuf::RootGetElectNodesRequest get_nodes_req;
    get_nodes_req.set_des_service_type(des_service_type.value());
    get_nodes_req.set_count(kGetNodesSize);
    std::string data;
    if (!get_nodes_req.SerializeToString(&data)) {
        TOP_WARN("RootGetElectNodesRequest SerializeToString failed!");
        return kKadFailed;
    }

    protobuf::RootMessage root_message;
    root_message.set_message_type(kGetElectNodesRequest);
    root_message.set_data(data);
    std::string root_data;
    if (!root_message.SerializeToString(&root_data)) {
        TOP_INFO("RootMessage SerializeToString failed!");
        return kKadFailed;
    }
    message.set_data(root_data);
    TOP_DEBUG_NAME("%s bluerootasync sending message", transport::FormatMsgid(message).c_str());
    SendToClosestNode(message, false);

    using namespace std::placeholders;
    auto this_ptr = std::dynamic_pointer_cast<RootRouting>(shared_from_this());
    auto callback = std::bind(&RootRouting::OnGetRootNodesV2Async, this_ptr, cb, des_kroot_id, des_service_type, _1, _2, _3);
    CallbackManager::Instance()->Add(message.id(), kGetNodesTimeout, callback, 1);
    return kKadSuccess;
}

void RootRouting::OnGetRootNodesV2Async(
        GetRootNodesV2AsyncCallback cb,
        std::string des_kroot_id,
        base::ServiceType des_service_type,
        int status,
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    TOP_DEBUG_NAME("%s bluerootasync routing callback", transport::FormatMsgid(message).c_str());
    if (status == kKadSuccess) {
        std::vector<kadmlia::NodeInfoPtr> nodes;
        do {
            if (!message.has_data() || message.data().empty()) {
                TOP_WARN("%s message has no data!", transport::FormatMsgid(message).c_str());
                break;
            }
            protobuf::RootMessage get_nodes_res;
            if (!get_nodes_res.ParseFromString(message.data())) {
                TOP_WARN("%s message ParseFromString failed!", transport::FormatMsgid(message).c_str());
                break;
            }
            if (!get_nodes_res.has_data() && get_nodes_res.data().empty()) {
                TOP_WARN("%s message root message has no data!", transport::FormatMsgid(message).c_str());
                break;
            }
            protobuf::RootGetNodesResponse nodes_res;
            if (!nodes_res.ParseFromString(get_nodes_res.data())) {
                TOP_WARN("%s message root message ParseFromString!", transport::FormatMsgid(message).c_str());
                break;
            }
            for (int i = 0; i < nodes_res.nodes_size(); ++i) {
                if(base::GetKadmliaKey(nodes_res.nodes(i).id())->GetServiceType()!=des_service_type){
                    xdbg("Charles Debug not this service type? des:%s get:%s",
                         des_service_type.info().c_str(),
                         base::GetKadmliaKey(nodes_res.nodes(i).id())->GetServiceType().info().c_str());
                    continue;
                }
                NodeInfoPtr node_ptr;
                node_ptr.reset(new NodeInfo(nodes_res.nodes(i).id()));
                node_ptr->public_ip = nodes_res.nodes(i).public_ip();
                node_ptr->public_port = nodes_res.nodes(i).public_port();
                node_ptr->local_ip = nodes_res.nodes(i).local_ip();
                node_ptr->local_port = nodes_res.nodes(i).local_port();
                node_ptr->xid = des_kroot_id;
                node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
                nodes.push_back(node_ptr);
            }

            cb(des_service_type, nodes);
            return;
        } while (0);
    } else {
        TOP_DEBUG("%s OnGetRootNodesV2Async timeout", transport::FormatMsgid(message).c_str());
    }
}

void RootRouting::HandleGetElectNodesRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("handle request", message);
    if (message.des_node_id() != local_node_ptr_->id()) {
        bool closest = false;
        if (ClosestToTarget(message.des_node_id(), closest) != kKadSuccess) {
            TOP_WARN("root routing closesttotarget goes wrong");
            return;
        }
        if (!closest) {
            TOP_DEBUG("root routing continue sendtoclosest");
            return SendToClosestNode(message);
        }
        TOP_INFO("this is the closest node(%s) of msg.des_node_id(%s)",
                HexEncode(get_local_node_info()->id()).c_str(),
                HexEncode(message.des_node_id()).c_str());
    } else {
        TOP_DEBUG("this is the des node(%s)", HexEncode(get_local_node_info()->id()).c_str());
    }

    if (!message.has_data() || message.data().empty()) {
        TOP_WARN("HandleGetElectNodesRequest has no data!");
        return;
    }

    protobuf::RootMessage root_message;
    if (!root_message.ParseFromString(message.data())) {
        TOP_WARN("RootMessage ParseFromString from string failed!");
        return;
    }

    protobuf::RootGetElectNodesRequest get_nodes_req;
    if (!get_nodes_req.ParseFromString(root_message.data())) {
        TOP_WARN("RootGetElectNodesRequest ParseFromString failed!");
        return;
    }
    base::ServiceType des_service_type = base::ServiceType(get_nodes_req.des_service_type());
    auto routing_table = GetRoutingTable(des_service_type, false);
    std::vector<NodeInfoPtr> nodes;
    if (!routing_table) {
        TOP_WARN("GetRoutingTable failed for service_type:%llu", des_service_type.value());
        return;
    }
    auto local_node_ptr = routing_table->get_local_node_info();
    if (!local_node_ptr) {
        assert(false);
    }
    nodes = routing_table->GetClosestNodes(local_node_ptr->id(), get_nodes_req.count() - 1);

    transport::protobuf::RoutingMessage res_message;
#ifndef NDEBUG
    if (message.has_debug()) {
        res_message.set_debug(message.debug());
    }
#endif

    SetFreqMessage(res_message);
    res_message.set_src_service_type(message.des_service_type());
    res_message.set_des_service_type(kRoot);
    res_message.set_des_node_id(message.src_node_id());
    res_message.set_type(kRootMessage);
    res_message.set_id(message.id());
    protobuf::RootGetElectNodesResponse get_nodes_res;
    if (local_node_ptr->public_port() > 0) {
        protobuf::NodeInfo* node_info = get_nodes_res.add_nodes();
        node_info->set_id(local_node_ptr->id());
        node_info->set_public_ip(local_node_ptr->public_ip());
        node_info->set_public_port(local_node_ptr->public_port());
        node_info->set_local_ip(local_node_ptr->local_ip());
        node_info->set_local_port(local_node_ptr->local_port());
    } else {
        TOP_WARN("public_port invalid: %d of this node:%s",
                local_node_ptr->public_port(),
                HexEncode(local_node_ptr->id()).c_str());
    }

    auto tmp_ready_nodes = 0;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (static_cast<uint32_t>(get_nodes_res.nodes_size()) >= get_nodes_req.count()) {
            break;
        }

        if (nodes[i]->node_id == message.des_node_id()) {
            continue;
        }
        if (nodes[i]->xid == message.xid()) {
            continue;
        }
        if (nodes[i]->public_port <= 0) {
            continue;
        }
        auto tmp_kad_key = base::GetKadmliaKey(nodes[i]->node_id);
        if (tmp_kad_key->GetServiceType() != des_service_type) {
            continue;
        }
        protobuf::NodeInfo* node_info = get_nodes_res.add_nodes();
        node_info->set_id(nodes[i]->node_id);
        node_info->set_public_ip(nodes[i]->public_ip);
        node_info->set_public_port(nodes[i]->public_port);
        node_info->set_local_ip(nodes[i]->local_ip);
        node_info->set_local_port(nodes[i]->local_port);
        ++tmp_ready_nodes;
    }
    TOP_DEBUG("nodes:%d ready_nodes:%d filtered:%d", nodes.size(), tmp_ready_nodes, nodes.size()-tmp_ready_nodes);

    std::string data;
    if (!get_nodes_res.SerializeToString(&data)) {
        TOP_WARN("RootGetElectNodesResponse SerializeToString failed!");
        return;
    }

    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE(
            std::string("response: ") + std::to_string(get_nodes_res.nodes_size()),
            message);
    protobuf::RootMessage root_res_message;
    root_res_message.set_message_type(kGetElectNodesResponse);
    root_res_message.set_data(data);
    std::string root_data;
    if (!root_res_message.SerializeToString(&root_data)) {
        TOP_WARN("RootMessage SerializeToString failed!");
        return;
    }

    res_message.set_data(root_data);

    TOP_DEBUG("send response of msg.des: %s size: %d",
              HexEncode(message.des_node_id()).c_str(),
              tmp_ready_nodes);
    SendToClosestNode(res_message);
    return;
}

void RootRouting::HandleGetElectNodesResponse(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    if (message.des_node_id() != local_node_ptr_->id()) {
        return SendToClosestNode(message);
    }

    TOP_DEBUG("response arrive");
    CallbackManager::Instance()->Callback(message.id(), message, packet);
}

// void RootRouting::RegisterBootstrapCacheCallback(
//         on_bootstrap_cache_get_callback_t get_cache_callback,
//         on_bootstrap_cache_set_callback_t set_cache_callback) {
//     assert(get_cache_callback);
//     assert(set_cache_callback);
//     bootstrap_cache_helper_->RegisterBootstrapCacheCallback(get_cache_callback, set_cache_callback);
// }

// void RootRouting::UnRegisterBootstrapCacheCallback() {
//     bootstrap_cache_helper_->UnRegisterBootstrapCacheCallback();
// }





}  // namespace wrouter

}  // namespace top
