#include "xelect_net/include/elect_manager.h"

#include "xbasic/xelapsed_time.h"
#include "xbasic/xutility.h"
#include "xchain_timer/xchain_timer.h"
#include "xelect_net/include/elect_uitils.h"
#include "xelect_net/include/node_manager.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/xip_parser.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/register_routing_table.h"

#include <chrono>
#include <limits>

namespace top {

namespace elect {

ElectManager::ElectManager(transport::TransportPtr transport, const base::Config & config) : transport_(transport), config_(config) {
}

// bool ElectManager::Start() {
//     node_manager_ = std::make_shared<NodeManager>(transport_, config_);
//     if (node_manager_->Init() != top::kadmlia::kKadSuccess) {
//         TOP_ERROR("node_manager init fail.");
//         node_manager_ = nullptr;
//         return false;
//     }
//     return true;
// }

void ElectManager::OnElectUpdated(const data::election::xelection_result_store_t & election_result_store, common::xzone_id_t const & zid) {
    using top::data::election::xelection_cluster_result_t;
    using top::data::election::xelection_group_result_t;
    using top::data::election::xelection_info_bundle_t;
    using top::data::election::xelection_info_t;
    using top::data::election::xelection_result_store_t;
    using top::data::election::xelection_result_t;

    // std::vector<ElectNetNode> elect_data;

    for (auto const & election_result_info : election_result_store) {
        auto const network_id = top::get<common::xnetwork_id_t const>(election_result_info);
        auto const & election_type_results = top::get<data::election::xelection_network_result_t>(election_result_info);
        for (auto const & election_type_result : election_type_results) {
            auto node_type = top::get<common::xnode_type_t const>(election_type_result);
            auto const & election_result = top::get<data::election::xelection_result_t>(election_type_result);

            for (auto const & cluster_result_info : election_result) {
                auto const & cluster_id = top::get<common::xcluster_id_t const>(cluster_result_info);
                auto const & cluster_result = top::get<xelection_cluster_result_t>(cluster_result_info);

                for (auto const & group_result_info : cluster_result) {
                    auto const & group_id = top::get<common::xgroup_id_t const>(group_result_info);
                    auto const & group_result = top::get<xelection_group_result_t>(group_result_info);

                    common::xip2_t xip2{network_id, zid, cluster_id, group_id};

                    auto const & size = static_cast<uint16_t>(group_result.size());
                    auto const & height = group_result.group_version().value();  // use group_version as xip height;

                    std::vector<wrouter::WrouterTableNodes> elect_data;
                    for (auto const & node_info : group_result) {
                        auto const & node_id = top::get<xelection_info_bundle_t>(node_info).node_id();
                        auto const & election_info = top::get<xelection_info_bundle_t>(node_info).election_info();
                        auto const & slot_id = top::get<const common::xslot_id_t>(node_info);
                        // here the slot_id is strict increasing. Start with 0.
                        if (node_id.empty()) {
                            continue;
                        }

                        common::xip2_t xip2_{network_id, zid, cluster_id, group_id, slot_id, size, height};

                        wrouter::WrouterTableNodes router_node{xip2_, node_id.to_string()};
                        xinfo("[Charles DEBUG] %s %s", xip2_.to_string().c_str(), node_id.to_string().c_str());

                        // ElectNetNode enode{node_id.to_string(), election_info.consensus_public_key.to_string(), xip, "", associated_gid, version};
                        elect_data.push_back(router_node);
                    }
                    OnElectUpdated(elect_data);
                }
            }
        }
    }
}
#if 0
void ElectManager::OnElectUpdated(const std::vector<ElectNetNode> & elect_data) {
    TOP_DEBUG("onelectupdated begin, size:%u", elect_data.size());
    for (const auto & enode : elect_data) {
        wrouter::NetNode snode{enode.m_account, enode.m_public_key, enode.m_xip, "", enode.m_associated_gid, enode.m_version};
        wrouter::SmallNetNodes::Instance()->AddNode(snode);

        if (global_node_id != enode.m_account) {
            TOP_DEBUG("account not match: self:%s iter:%s", global_node_id.c_str(), enode.m_account.c_str());
            continue;
        }
        TOP_INFO("account match: self:%s iter:%s xip: %s xnetwork_id: %u zone_id: %u cluster_id: %u group_id: %u",
                 global_node_id.c_str(),
                 enode.m_account.c_str(),
                 HexEncode(enode.m_xip.xip()).c_str(),
                 enode.m_xip.xnetwork_id(),
                 enode.m_xip.zone_id(),
                 enode.m_xip.cluster_id(),
                 enode.m_xip.group_id());
        if (node_manager_->Join(enode.m_xip) != kadmlia::kKadSuccess) {
            TOP_ERROR("node join failed!");
        }
    }

    TOP_DEBUG("onelectupdated end");
    return;
}
#endif
void ElectManager::OnElectUpdated(std::vector<wrouter::WrouterTableNodes> const & elect_data) {
    // for(auto const &wrouter_node:elect_data){
    // }
    for (auto const & wrouter_node : elect_data) {
        wrouter::SmallNetNodes::Instance()->AddNode(wrouter_node);
        if (global_node_id != wrouter_node.node_id) {
            xdbg("node id not match self:%s iter:%s", global_node_id.c_str(), wrouter_node.node_id.c_str());
            continue;
        }
        xinfo("account match: self:%s iter:%s ,xip:%s ", global_node_id.c_str(), wrouter_node.node_id.c_str(), wrouter_node.m_xip2.to_string().c_str());
        UpdateRoutingTable(elect_data, wrouter_node);
        // break;
    }
}

void ElectManager::UpdateRoutingTable(std::vector<wrouter::WrouterTableNodes> const & elect_data, wrouter::WrouterTableNodes const & self_wrouter_nodes) {
    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(self_wrouter_nodes.m_xip2);
    base::ServiceType service_type = kad_key->GetServiceType();

    // todo charles if find this routing table don't add again?[done]
    if (wrouter::MultiRouting::Instance()->GetElectRoutingTable(service_type) != nullptr) {
        xinfo("ElectManager::UpdateRoutingTable get repeated routing table info xip2: service_type:%s", self_wrouter_nodes.m_xip2.to_string().c_str(), service_type.info().c_str());
        return;
    }

    base::Config config = config_;
    if (!config.Set("node", "network_id", self_wrouter_nodes.m_xip2.network_id().value())) {
        TOP_ERROR("set config node network_id [%d] failed!", self_wrouter_nodes.m_xip2.network_id().value());
        return;
    }

    if (!config.Set("node", "first_node", false)) {
        TOP_ERROR("set config node first_node [%d] failed!", true);
        return;
    }


    std::shared_ptr<top::kadmlia::ElectRoutingTable> routing_table_ptr;
    kadmlia::LocalNodeInfoPtr local_node_ptr = kadmlia::CreateLocalInfoFromConfig(config, kad_key);

    if (!local_node_ptr) {
        TOP_WARN("local_node_ptr invalid");
        return;
    }
    routing_table_ptr = std::make_shared<kadmlia::ElectRoutingTable>(transport_, local_node_ptr);
    if (!routing_table_ptr->Init()) {
        TOP_ERROR("init edge bitvpn routing table failed!");
        return;
    }
    // routing_table_ptr->get_local_node_info()->set_service_type(service_type);
    // // service_type_ = service_type;
    // // wrouter::RegisterRoutingTable(service_type, routing_table_ptr);
    // TOP_KINFO("register routing table %llu", service_type);
    wrouter::MultiRouting::Instance()->AddElectRoutingTable(service_type, routing_table_ptr);
    // todo charles move it in multirouting.
    std::cout << global_node_id << " create routing table: " << std::hex << service_type.value() << service_type.info() << std::endl;

    // bool first_node = false;
    // std::set<std::pair<std::string, uint16_t>> join_endpoints;

    // auto ret = wrouter::NetworkExists(kad_key, join_endpoints);
    // TOP_INFO("check routing exists:[%llu], ret: %d, endpoints_size: %d", service_type, ret, join_endpoints.size());
    // if (ret != kadmlia::kKadSuccess || join_endpoints.empty()) {
    //     first_node = true;
    // }

    // if (first_node) {
    auto root_routing = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    // std::vector<kadmlia::NodeInfoPtr> res_nodes;
    // std::vector<base::KadmliaKeyPtr> kad_key_ptrs;
    std::map<std::string, base::KadmliaKeyPtr> elect_root_kad_key_ptrs;
    for (auto _node : elect_data) {
        // kad_key_ptrs.push_back(base::GetRootKadmliaKey(_node.node_id));
        elect_root_kad_key_ptrs.insert(std::make_pair(_node.m_xip2.to_string(), base::GetRootKadmliaKey(_node.node_id)));
        // auto des_kad_key = base::GetRootKadmliaKey(_node.node_id);
        // auto des_service_type = base::CreateServiceType(_node.m_xip2);
        // xinfo("Charles Debug GetSameNetworkNodesV2 res_nodes.size(): %zu des_node_id:%s ,des_kad_key:%s ", res_nodes.size(), _node.node_id.c_str(), des_kad_key->Get().c_str());
        // wrouter::GetSameNetworkNodesV2(des_kad_key->Get(), des_service_type, res_nodes);
        // if(!res_nodes.empty()){
        //     for(auto _v:res_nodes){
        //         xinfo("Charles Debug GetSameNetworkNodesV2 node_id:%s ip:%s,port:%d",_v->node_id.c_str(),_v->public_ip.c_str(),_v->public_port);
        //     }
        // }
    }
    routing_table_ptr->SetElectionNodesExpected(elect_root_kad_key_ptrs);
    local_node_ptr->set_public_ip(root_routing->get_local_node_info()->public_ip());
    local_node_ptr->set_public_port(root_routing->get_local_node_info()->public_port());
    // local_node_ptr->set_first_node(true);
    // if (root_routing->get_local_node_info()->public_ip().empty()) {
    //     TOP_ERROR("RoutingManagerBase local node public ip is empty.");
    //     assert(false);
    // }
    wrouter::MultiRouting::Instance()->CheckElectRoutingTable(service_type);
    return;
    // }

    // routing_table_ptr->MultiJoinAsync(join_endpoints);
    // TOP_INFO("multijoin of service_type: %llu ...", service_type_);
    // return;
}

// unregister routing table
int ElectManager::OnElectQuit(const common::xip2_t & xip2) {
    // base::XipParser xip;
    // xip.set_xnetwork_id(static_cast<uint32_t>(xip2.network_id().value()));
    // xip.set_zone_id(static_cast<uint8_t>(xip2.zone_id().value()));
    // xip.set_cluster_id(static_cast<uint8_t>(xip2.cluster_id().value()));
    // xip.set_group_id(static_cast<uint8_t>(xip2.group_id().value()));
    // xip.set_network_type((uint8_t)(address.cluster_address().type()));

    auto service_type = base::GetKadmliaKey(xip2)->GetServiceType();
    wrouter::MultiRouting::Instance()->RemoveElectRoutingTable(service_type);
    // todo charles move it in multirouting.
    std::cout << global_node_id << " delete routing table: " << std::hex << service_type.value() << service_type.info() << std::endl;
    xdbg("OnElectQuit service_type:%lld xip2:%s", service_type.value(), xip2.to_string().c_str());

    return 0;
}

}  // namespace elect

}  // namespace top
