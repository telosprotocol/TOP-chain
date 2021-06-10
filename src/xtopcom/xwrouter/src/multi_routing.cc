// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/multi_routing.h"

#include "xkad/routing_table/local_node_info.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/register_message_handler.h"

#include <list>

namespace top {

using namespace kadmlia;

namespace wrouter {

static const int32_t kCheckElectRoutingTableNodesPeriod = 5 * 1000 * 1000;

MultiRouting::MultiRouting() : elect_routing_table_map_(), elect_routing_table_map_mutex_() {
    WrouterRegisterMessageHandler(kRootMessage, [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) { HandleRootMessage(message, packet); });
    check_elect_routing_ = std::make_shared<base::TimerRepeated>(timer_manager_, "MultiRouting::CheckElectRoutingTableTimer");
    check_elect_routing_->Start(kCheckElectRoutingTableNodesPeriod, kCheckElectRoutingTableNodesPeriod, std::bind(&MultiRouting::CompleteElectRoutingTable, this));
}

MultiRouting::~MultiRouting() {
    TOP_KINFO("MultiRouting destroy");
}

MultiRouting * MultiRouting::Instance() {
    static MultiRouting ins;
    return &ins;
}

int MultiRouting::CreateRootRouting(std::shared_ptr<transport::Transport> transport, const base::Config & config, base::KadmliaKeyPtr kad_key_ptr) {
    base::ServiceType service_type = base::ServiceType{kRoot};
    assert(kad_key_ptr->xnetwork_id() == kRoot);
    {
        std::unique_lock<std::mutex> lock(root_routing_table_mutex_);
        if (root_routing_table_ != nullptr) {
            TOP_WARN("service type[%lu] has added!", service_type.value());
            return kKadSuccess;
        }
    }

    std::set<std::pair<std::string, uint16_t>> public_endpoints_config;
    GetPublicEndpointsConfig(config, public_endpoints_config);
    TOP_INFO("enter CreateRoutingTable:%lu", service_type.value());
    kadmlia::LocalNodeInfoPtr local_node_ptr = kadmlia::CreateLocalInfoFromConfig(config, kad_key_ptr);
    if (!local_node_ptr) {
        TOP_FATAL("create local_node_ptr for service_type(%ld) failed", (long)service_type.value());
        return kKadFailed;
    }
    auto routing_table_ptr = std::make_shared<RootRouting>(transport, local_node_ptr);

    if (!routing_table_ptr->Init()) {  // RootRouting::Init()
        TOP_FATAL("init edge bitvpn routing table failed!");
        return kKadFailed;
    }

    TOP_INFO("kroot routing table enable bootstrapcache, register set and get");

    routing_table_ptr->get_local_node_info()->set_service_type(service_type);
    {
        std::unique_lock<std::mutex> lock(root_routing_table_mutex_);
        root_routing_table_ = routing_table_ptr;
    }

    bool first_node = false;
    if (config.Get("node", "first_node", first_node)) {
        if (first_node) {
            TOP_INFO("first node started!");
            return kKadSuccess;
        }
    }

    if (public_endpoints_config.empty()) {
        TOP_FATAL("node join must has bootstrap endpoints!");
        return kKadFailed;
    }

    if (routing_table_ptr->MultiJoin(public_endpoints_config) != kKadSuccess) {
        TOP_FATAL("MultiJoin failed");
        return kKadFailed;
    }
    TOP_INFO("MultiJoin success.");

    return kKadSuccess;
}

void MultiRouting::HandleRootMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
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
    case kGetNodesRequest:
        // return HandleFindNodesFromOthersRequest(message, packet);
    case kGetNodesResponse:
        // return HandleFindNodesFromOthersResponse(message, packet);
        return root_routing_table_->HandleMessage(message, packet);
    case kGetElectNodesRequest:
        return HandleGetElectNodesRequest(message, packet);
    case kGetElectNodesResponse:
        return HandleGetElectNodesResponse(message, packet);
    default:
        TOP_WARN("invalid root message type[%d].", root_message.message_type());
        break;
    }
}

// ----------

void MultiRouting::HandleGetElectNodesRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    std::unique_lock<std::mutex> lock(root_routing_table_mutex_);
    if (message.des_node_id() != root_routing_table_->get_local_node_info()->id()) {
        bool closest = false;
        if (root_routing_table_->ClosestToTarget(message.des_node_id(), closest) != kKadSuccess) {
            TOP_WARN("root routing closesttotarget goes wrong");
            return;
        }
        if (!closest) {
            TOP_DEBUG("root routing continue sendtoclosest");
            return root_routing_table_->SendToClosestNode(message);
        }
        TOP_INFO("this is the closest node(%s) of msg.des_node_id(%s)", (root_routing_table_->get_local_node_info()->id()).c_str(), (message.des_node_id()).c_str());
    } else {
        TOP_DEBUG("this is the des node(%s)", (root_routing_table_->get_local_node_info()->id()).c_str());
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

    // todo charles check this .
    // #if 0
    auto routing_table = GetElectRoutingTable(des_service_type);

    std::vector<NodeInfoPtr> nodes;
    if (!routing_table) {
        TOP_WARN("GetRoutingTable failed for service_type:%llu", des_service_type.value());
        return;
    }
    auto local_node_ptr = routing_table->get_local_node_info();
    if (!local_node_ptr) {
        assert(false);
    }
    routing_table->GetRandomNodes(nodes, get_nodes_req.count());
    // nodes = routing_table->GetClosestNodes(local_node_ptr->id(), get_nodes_req.count() - 1);

    transport::protobuf::RoutingMessage res_message;
#ifndef NDEBUG
    if (message.has_debug()) {
        res_message.set_debug(message.debug());
    }
#endif

    root_routing_table_->SetFreqMessage(res_message);
    res_message.set_is_root(true);
    res_message.set_src_service_type(message.des_service_type());
    res_message.set_des_service_type(kRoot);
    res_message.set_des_node_id(message.src_node_id());
    res_message.set_type(kRootMessage);
    res_message.set_id(message.id());
    protobuf::RootGetElectNodesResponse get_nodes_res;
    if (local_node_ptr->public_port() > 0) {
        protobuf::NodeInfo * node_info = get_nodes_res.add_nodes();
        node_info->set_id(local_node_ptr->id());
        node_info->set_public_ip(local_node_ptr->public_ip());
        node_info->set_public_port(local_node_ptr->public_port());
        node_info->set_local_ip(local_node_ptr->local_ip());
        node_info->set_local_port(local_node_ptr->local_port());
    } else {
        TOP_WARN("public_port invalid: %d of this node:%s", local_node_ptr->public_port(), (local_node_ptr->id()).c_str());
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
        protobuf::NodeInfo * node_info = get_nodes_res.add_nodes();
        node_info->set_id(nodes[i]->node_id);
        node_info->set_public_ip(nodes[i]->public_ip);
        node_info->set_public_port(nodes[i]->public_port);
        node_info->set_local_ip(nodes[i]->local_ip);
        node_info->set_local_port(nodes[i]->local_port);
        ++tmp_ready_nodes;
    }
    TOP_DEBUG("nodes:%d ready_nodes:%d filtered:%d", nodes.size(), tmp_ready_nodes, nodes.size() - tmp_ready_nodes);

    std::string data;
    if (!get_nodes_res.SerializeToString(&data)) {
        TOP_WARN("RootGetElectNodesResponse SerializeToString failed!");
        return;
    }

    protobuf::RootMessage root_res_message;
    root_res_message.set_message_type(kGetElectNodesResponse);
    root_res_message.set_data(data);
    std::string root_data;
    if (!root_res_message.SerializeToString(&root_data)) {
        TOP_WARN("RootMessage SerializeToString failed!");
        return;
    }

    res_message.set_data(root_data);

    TOP_DEBUG("send response of msg.des: %s size: %d", HexEncode(message.des_node_id()).c_str(), tmp_ready_nodes);
    root_routing_table_->SendToClosestNode(res_message);
    // #endif
    return;
}

void MultiRouting::HandleGetElectNodesResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    std::unique_lock<std::mutex> lock(root_routing_table_mutex_);
    if (message.des_node_id() != root_routing_table_->get_local_node_info()->id()) {
        return root_routing_table_->SendToClosestNode(message);
    }

    TOP_DEBUG("response arrive");
    CallbackManager::Instance()->Callback(message.id(), message, packet);
}

// ----------

// kadmlia::RoutingTablePtr MultiRouting::GetRoutingTable(base::ServiceType const &service_type,bool is_root){
//     if(is_root){
//         if (!root_manager_ptr_) {
//             return nullptr;
//         }
//         return root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot});
//     }else{
//         for (auto const & _p : elect_routing_table_map_) {
//             if (_p.first == service_type) {
//                 return _p.second;
//             }
//         }
//         return nullptr;
//     }
// }

kadmlia::ElectRoutingTablePtr MultiRouting::GetElectRoutingTable(base::ServiceType const & service_type) {
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    for (auto const & _p : elect_routing_table_map_) {
        if (_p.first == service_type) {
            return _p.second;
        }
    }
    return nullptr;
}

kadmlia::RootRoutingTablePtr MultiRouting::GetRootRoutingTable() {
    std::unique_lock<std::mutex> lock(root_routing_table_mutex_);
    if (!root_routing_table_) {
        return nullptr;
    }
    return root_routing_table_;
    // return root_manager_ptr_->GetRootRoutingTable();
}

void MultiRouting::AddElectRoutingTable(base::ServiceType service_type, kadmlia::ElectRoutingTablePtr routing_table) {
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    if (!routing_table) {
        return;
    }
    auto iter = elect_routing_table_map_.find(service_type);
    if (iter != elect_routing_table_map_.end()) {
        assert(false);
        return;
    }

    elect_routing_table_map_[service_type] = routing_table;
}

void MultiRouting::RemoveElectRoutingTable(base::ServiceType service_type) {
    ElectRoutingTablePtr remove_routing_table = nullptr;
    {
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
        auto iter = elect_routing_table_map_.find(service_type);
        if (iter != elect_routing_table_map_.end()) {
            remove_routing_table = iter->second;
            elect_routing_table_map_.erase(iter);
        }
    }
    if (remove_routing_table) {
        remove_routing_table->UnInit();
        TOP_KINFO("remove service routing table: %llu", service_type.value());
    }

    std::vector<base::ServiceType> vec_type;
    GetAllRegisterType(vec_type);
    for (auto & v : vec_type) {
        TOP_KINFO("after unregister routing table, still have %llu", v.value());
    }
}

void MultiRouting::GetAllRegisterType(std::vector<base::ServiceType> & vec_type) {
    vec_type.clear();
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    for (auto & it : elect_routing_table_map_) {
        vec_type.push_back(it.first);
    }
}

void MultiRouting::GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::ElectRoutingTable>> & vec_rt) {
    vec_rt.clear();
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    for (auto & it : elect_routing_table_map_) {
        vec_rt.push_back(it.second);
    }
}

void MultiRouting::CheckElectRoutingTable(base::ServiceType service_type) {
    kadmlia::ElectRoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
        assert(elect_routing_table_map_.find(service_type) != elect_routing_table_map_.end());
        routing_table = elect_routing_table_map_[service_type];
    }
    auto kad_key_ptrs = routing_table->GetElectionNodesExpected();
    if (!kad_key_ptrs.empty()) {
        std::map<std::string, kadmlia::NodeInfoPtr> res_nodes;  // election_node_id, NodeInfoPtr
        {
            std::unique_lock<std::mutex> lock(root_routing_table_mutex_);
            root_routing_table_->FindElectionNodesInfo(kad_key_ptrs, res_nodes);
        }
        routing_table->HandleElectionNodesInfoFromRoot(res_nodes);
    }
}

void MultiRouting::CheckElectRoutingTableTimer() {
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    for (auto _p : elect_routing_table_map_) {
        kadmlia::ElectRoutingTablePtr routing_table = _p.second;
        auto kad_key_ptrs = routing_table->GetElectionNodesExpected();
        if (!kad_key_ptrs.empty()) {
            std::map<std::string, kadmlia::NodeInfoPtr> res_nodes;  // election_node_id, NodeInfoPtr
            {
                std::unique_lock<std::mutex> lock(root_routing_table_mutex_);
                root_routing_table_->FindElectionNodesInfo(kad_key_ptrs, res_nodes);
            }
            routing_table->HandleElectionNodesInfoFromRoot(res_nodes);
        }
    }
}

void MultiRouting::CompleteElectRoutingTable() {
    bool flag{false};
    {
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);

        for (auto const & routing_table_pair : elect_routing_table_map_) {
            kadmlia::ElectRoutingTablePtr routing_table = routing_table_pair.second;
            // map<election_xip2_str,node_id_root_kad_key>
            auto kad_key_ptrs = routing_table->GetElectionNodesExpected();
            if (!kad_key_ptrs.empty()) {
                for (auto const & _p : kad_key_ptrs) {
                    OnCompleteElectRoutingTableCallback cb =
                        std::bind(&MultiRouting::OnCompleteElectRoutingTable, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
                    // OnCompleteElectRoutingTableCallback cb = std::bind(&MultiRouting::OnCompleteElectRoutingTable, this, routing_table_pair.first, _p.first,
                    // std::placeholders::_1);
                    std::unique_lock<std::mutex> lock(root_routing_table_mutex_);
                    if (root_routing_table_->FindNodesFromOthers(routing_table_pair.first, _p.first, cb, _p.second) == false) {
                        flag = true;
                        break;
                    }
                }
            }
            if (flag)
                break;
        }
    }
    if (flag) {
        CheckElectRoutingTableTimer();
    }
}

void MultiRouting::OnCompleteElectRoutingTable(base::ServiceType const service_type, std::string const election_xip2, kadmlia::NodeInfoPtr const & node_info) {
    xdbg("Charles Debug MultiRouting::OnCompleteElectRoutingTable %s", election_xip2.c_str());
    kadmlia::ElectRoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
        if (elect_routing_table_map_.find(service_type) == elect_routing_table_map_.end()) {
            return;
        }
        assert(elect_routing_table_map_.find(service_type) != elect_routing_table_map_.end());
        routing_table = elect_routing_table_map_[service_type];
    }
    routing_table->OnFindNodesFromRootRouting(election_xip2, node_info);
}

}  // namespace wrouter

}  // namespace top
