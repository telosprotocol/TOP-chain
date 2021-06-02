// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelect_net/include/routing_manager.h"

#include "xkad/routing_table/nodeid_utils.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/top_log.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/root/root_routing_manager.h"

#include <set>

namespace top {

namespace elect {

RoutingManager::RoutingManager() {
}

RoutingManager::RoutingManager(uint64_t service_type) : service_type_(service_type) {
}

RoutingManager::~RoutingManager() {
}

int RoutingManager::Init(base::KadmliaKeyPtr kad_key, std::shared_ptr<transport::Transport> transport, const top::base::Config & config) {
    routing_ptr_ = CreateRoutingTable(kad_key, transport, config);
    if (!routing_ptr_) {
        TOP_KINFO("unregister routing table %llu", kad_key->GetServiceType());
        wrouter::MultiRouting::Instance()->RemoveRoutingTable(kad_key->GetServiceType());
        return top::kadmlia::kKadFailed;
    }

    return top::kadmlia::kKadSuccess;
}

std::shared_ptr<top::kadmlia::RoutingTable> RoutingManager::CreateRoutingTable(base::KadmliaKeyPtr kad_key,
                                                                               std::shared_ptr<transport::Transport> transport,
                                                                               const top::base::Config & config) {
    std::shared_ptr<top::kadmlia::RoutingTable> routing_table_ptr;
    kadmlia::LocalNodeInfoPtr local_node_ptr = kadmlia::CreateLocalInfoFromConfig(config, kad_key);
    std::cout << "ec create routing table: ";
    // PrintXip(kad_key->Xip());
    if (!local_node_ptr) {
        TOP_WARN("local_node_ptr invalid");
        return nullptr;
    }
    routing_table_ptr = std::make_shared<kadmlia::RoutingTable>(transport, kNodeIdSize, local_node_ptr);
    if (!routing_table_ptr->Init()) {
        TOP_ERROR("init edge bitvpn routing table failed!");
        return nullptr;
    }
    uint64_t service_type = kad_key->GetServiceType();
    routing_table_ptr->get_local_node_info()->set_service_type(service_type);
    service_type_ = service_type;
    // wrouter::RegisterRoutingTable(service_type, routing_table_ptr);
    TOP_KINFO("register routing table %llu", service_type);
    wrouter::MultiRouting::Instance()->AddRoutingTable(service_type, routing_table_ptr);
    bool first_node = false;
    std::set<std::pair<std::string, uint16_t>> join_endpoints;
    auto ret = wrouter::NetworkExists(kad_key, join_endpoints);
    TOP_INFO("check routing exists:[%llu], ret: %d, endpoints_size: %d", service_type, ret, join_endpoints.size());
    if (ret != kadmlia::kKadSuccess || join_endpoints.empty()) {
        first_node = true;
    }

    if (first_node) {
        auto root_routing = wrouter::GetRoutingTable(kRoot, true);
        local_node_ptr->set_public_ip(root_routing->get_local_node_info()->public_ip());
        local_node_ptr->set_public_port(root_routing->get_local_node_info()->public_port());
        local_node_ptr->set_first_node(true);
        if (root_routing->get_local_node_info()->public_ip().empty()) {
            TOP_ERROR("RoutingManagerBase local node public ip is empty.");
            assert(false);
        }
        return routing_table_ptr;
    }

    routing_table_ptr->MultiJoinAsync(join_endpoints);
    TOP_INFO("multijoin of service_type: %llu ...", service_type_);
    return routing_table_ptr;
}

}  // namespace elect

}  // namespace top
