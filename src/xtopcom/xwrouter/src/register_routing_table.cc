// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/register_routing_table.h"

#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/root/root_routing.h"
#include "xwrouter/root/root_routing_manager.h"

namespace top {

using namespace kadmlia;

namespace wrouter {

std::shared_ptr<kadmlia::RoutingTable> GetRoutingTable(const uint64_t & type, bool root) {
    RoutingTablePtr routing_table = MultiRouting::Instance()->GetRoutingTable(type, root);
    if (routing_table) {
        return routing_table;
    }
    return nullptr;
}

std::shared_ptr<kadmlia::RoutingTable> GetRoutingTable(const std::string & routing_id, bool root) {
    RoutingTablePtr routing_table = MultiRouting::Instance()->GetRoutingTable(routing_id, root);
    if (routing_table) {
        return routing_table;
    }
    return nullptr;
}

void GetAllRegisterType(std::vector<uint64_t> & vec_type) {
    return MultiRouting::Instance()->GetAllRegisterType(vec_type);
}

void GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::RoutingTable>> & vec_rt) {
    return MultiRouting::Instance()->GetAllRegisterRoutingTable(vec_rt);
}

bool SetCacheServiceType(uint64_t service_type) {
    return MultiRouting::Instance()->SetCacheServiceType(service_type);
}

bool GetServiceBootstrapRootNetwork(uint64_t service_type, std::set<std::pair<std::string, uint16_t>> & boot_endpoints) {
    return MultiRouting::Instance()->GetServiceBootstrapRootNetwork(service_type, boot_endpoints);
}

int NetworkExists(base::KadmliaKeyPtr & kad_key_ptr, std::set<std::pair<std::string, uint16_t>> & endpoints) {
    auto service_type = kad_key_ptr->GetServiceType();
    wrouter::NetNode ele_first_node;
    if (!(SmallNetNodes::Instance()->FindNewNode(ele_first_node, service_type))) {
        TOP_WARN("Findnode small_node_cache invalid for service_type:%llu", service_type);
        return kadmlia::kKadFailed;
    }

    TOP_WARN("findnode small_node_cache account:%s, action NetworkExists for service_type:%llu", ele_first_node.m_account.c_str(), service_type);
    auto tmp_service_type = base::CreateServiceType(ele_first_node.m_xip);
    if (tmp_service_type != service_type) {
        TOP_WARN("small_node_cache find service_type: %llu not equal elect_service_type: %llu", tmp_service_type, service_type);
        return kadmlia::kKadFailed;
    }

    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(ele_first_node.m_account, true);  // kRoot id
    if (!kad_key) {
        TOP_WARN("small_node_cache kad_key nullptr");
        return kadmlia::kKadFailed;
    }

    std::vector<kadmlia::NodeInfoPtr> tmp_nodes;
    int res = GetSameNetworkNodesV2(kad_key->Get(), service_type, tmp_nodes);
    if (res == kadmlia::kKadSuccess) {
        if (tmp_nodes.empty()) {
            TOP_WARN("get root nodes failed for service_type:%llu", service_type);
            return kadmlia::kKadFailed;
        }
    }

    TOP_INFO("GetRootNodes by root routing ok.");

    for (uint32_t i = 0; i < tmp_nodes.size(); ++i) {
        auto tmp_kad_key = base::GetKadmliaKey(tmp_nodes[i]->node_id);
        uint64_t node_service_type = tmp_kad_key->GetServiceType();
        if (service_type != node_service_type) {
            continue;
        }
        if (tmp_nodes[i]->IsPublicNode()) {
            endpoints.insert(std::make_pair(tmp_nodes[i]->public_ip, tmp_nodes[i]->public_port));
        }
    }
    if (!endpoints.empty()) {
        TOP_INFO("GetRootNodes from remote ok, size:%d", endpoints.size());
        return kadmlia::kKadSuccess;
    }
    return kadmlia::kKadFailed;
}

int GetSameNetworkNodesV2(const std::string & des_kroot_id, uint64_t des_service_type, std::vector<kadmlia::NodeInfoPtr> & ret_nodes) {
    auto root_routing = std::dynamic_pointer_cast<RootRouting>(GetRoutingTable(kRoot, true));
    if (!root_routing) {
        TOP_WARN("create root manager failed!");
        return kadmlia::kKadFailed;
    }

    std::vector<kadmlia::NodeInfoPtr> nodes;
    if (root_routing->GetRootNodesV2(des_kroot_id, des_service_type, nodes) != kadmlia::kKadSuccess) {
        /*
        TOP_WARN("getrootnodes failed,des_kroot_id: %s des_service_type: %llu",
                HexEncode(des_kroot_id).c_str(),
                des_service_type);
                */
        return kadmlia::kKadFailed;
    }

    for (uint32_t i = 0; i < nodes.size(); ++i) {
        auto tmp_kad_key = base::GetKadmliaKey(nodes[i]->node_id);
        uint64_t node_service_type = tmp_kad_key->GetServiceType();
        if (des_service_type != node_service_type) {
            continue;
        }
        ret_nodes.push_back(nodes[i]);
    }
    return kadmlia::kKadSuccess;
}

}  // namespace wrouter

}  // namespace top
