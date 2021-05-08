//
//  chain_genesis.cc
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xelect_net/include/node_manager.h"

#include "xpbase/base/kad_key/chain_kadmlia_key.h"
#include "xtransport/transport.h"
#include "xkad/routing_table/routing_utils.h"
#include "xelect_net/include/routing_manager.h"
#include "xelect_net/include/elect_manager.h"
#include "xelect_net/include/elect_uitils.h"

namespace top {

using namespace kadmlia;

namespace elect {

NodeManager::NodeManager(
        std::shared_ptr<transport::Transport> transport,
        const base::Config& config)
        : NodeManagerBase(transport, config) {
        }

NodeManager::~NodeManager() {}

int NodeManager::Init() {
    NodeManagerBase::Init();
    return kKadSuccess;

    /*
    static const uint32_t kCheckRootTimes = 3u;
    for (uint32_t i = 0; i < kCheckRootTimes; ++i) {
        auto root_routing = wrouter::GetRoutingTable(kRoot, true);
        std::cout << "init root routing size: " << root_routing->nodes_size() << std::endl;
        TOP_INFO("init root routing size: %u", root_routing->nodes_size());
        if (root_routing->get_local_node_info()->first_node()) {
            return kKadSuccess;
        }

        if (root_routing->nodes_size() <= 0) {
            SleepUs(1000 * 1000);
            continue;
        }
        return kKadSuccess;
    }

    TOP_WARN("electnode init failed");
    return kKadFailed;
    */
}

int NodeManager::Join(const base::XipParser& xip) {
    base::KadmliaKeyPtr kad_key = GetKadmliaKey(xip);
    TOP_INFO("Join committee[%d][%d]", xip.xnetwork_id(), xip.zone_id());
    if (AddCommitteeRole(kad_key) != kadmlia::kKadSuccess) {
        return top::kadmlia::kKadFailed;
    }

    TOP_INFO("this is rec node add rec root success.");
    return kKadSuccess;
}

int NodeManager::Quit(const base::XipParser& xip) {
    std::shared_ptr<RoutingManager> quit_ptr = nullptr;
    base::KadmliaKeyPtr kad_key = GetKadmliaKey(xip);
    {
        std::unique_lock<std::mutex> lock(ec_manager_map_mutex_);
        auto iter = ec_manager_map_.find(kad_key->GetServiceType());
        if (iter == ec_manager_map_.end()) {
            TOP_WARN("this node has no role: %llu", kad_key->GetServiceType());
            return top::kadmlia::kKadFailed;
        }
        quit_ptr = iter->second;
        ec_manager_map_.erase(iter);
    }
    std::cout << "delete service : " << kad_key->GetServiceType() << std::endl;
    return kKadSuccess;
}


int NodeManager::DropNodes(const base::XipParser& xip, const std::vector<std::string>& drop_accounts) {
    base::KadmliaKeyPtr kad_key = GetKadmliaKey(xip);
    std::vector<std::string> drop_nodes;
    drop_nodes.reserve(30);
    for (auto& item : drop_accounts) {
        base::KadmliaKeyPtr drop_kad_key = GetKadmliaKey(xip, item);
        drop_nodes.push_back(drop_kad_key->Get());
        TOP_DEBUG("elect_dropnodes for xip: %s account: %s", HexEncode(xip.xip()).c_str(), item.c_str());
    }

    auto service_routing = wrouter::GetRoutingTable(kad_key->GetServiceType(), false);
    if (service_routing) {
        service_routing->BulkDropNode(drop_nodes);
        TOP_DEBUG("elect_dropnodes for service_routing: %s",
                HexEncode(service_routing->get_local_node_info()->id()).c_str());
    }
    auto root_routing = wrouter::GetRoutingTable(kad_key->GetServiceType(), true);
    if (root_routing) {
        root_routing->BulkDropNode(drop_nodes);
        TOP_DEBUG("elect_dropnodes for root_service_routing: %s",
                HexEncode(root_routing->get_local_node_info()->id()).c_str());
    }
    auto kroot_routing = wrouter::GetRoutingTable(kRoot, true);
    if (kroot_routing) {
        kroot_routing->BulkDropNode(drop_nodes);
        TOP_DEBUG("elect_dropnodes for kroot_routing: %s",
                HexEncode(kroot_routing->get_local_node_info()->id()).c_str());
    }
    return kKadSuccess;
}

int NodeManager::AddCommitteeRole(base::KadmliaKeyPtr& kad_key) {
    auto rt = wrouter::GetRoutingTable(kad_key->GetServiceType(), false);

    std::unique_lock<std::mutex> lock(ec_manager_map_mutex_);
    auto iter = ec_manager_map_.find(kad_key->GetServiceType());
    if (iter != ec_manager_map_.end()) {
        if (rt) {
            TOP_WARN("this node has role: %llu", kad_key->GetServiceType());
            return top::kadmlia::kKadFailed;
        } else {
            ec_manager_map_.erase(iter);
        }
    }

    auto ec_manager = std::make_shared<RoutingManager>(kad_key->GetServiceType());
    base::Config rec_conf = base_config_;
    if (!rec_conf.Set("node", "network_id", kad_key->xnetwork_id())) {
        TOP_ERROR("set config node network_id [%d] failed!", kad_key->xnetwork_id());
        return top::kadmlia::kKadFailed;
    }

    // TODO(Charlie): may be first node except beacon
    if (!rec_conf.Set("node", "first_node", false)) {
        TOP_ERROR("set config node first_node [%d] failed!", true);
        return top::kadmlia::kKadFailed;
    }

    int res = ec_manager->Init(
            kad_key,
            transport_,
            rec_conf);
    if (res != kKadSuccess) {
        TOP_ERROR("init root election committee failed!");
        return top::kadmlia::kKadFailed;
    }
    ec_manager_map_[kad_key->GetServiceType()] = ec_manager;
    auto ec_routing = std::dynamic_pointer_cast<ElectRouting>(ec_manager->routing_table());
    if (!ec_routing) {
        TOP_ERROR("dynamic cast routing  table failed!");
        return top::kadmlia::kKadFailed;
    }
    std::cout << "new service : " << kad_key->GetServiceType() << " joined." << std::endl;
    return top::kadmlia::kKadSuccess;
}

}  // namespace elect

}  // namespace top
