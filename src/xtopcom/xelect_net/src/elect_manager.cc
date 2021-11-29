#include "xelect_net/include/elect_manager.h"

#include "xbasic/xelapsed_time.h"
#include "xbasic/xutility.h"
#include "xchain_timer/xchain_timer.h"
#include "xelect_net/include/elect_uitils.h"
#include "xpbase/base/top_log.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/register_message_handler.h"

#include <chrono>
#include <limits>

namespace top {

namespace elect {

ElectManager::ElectManager(transport::TransportPtr transport, const base::Config & config) : transport_(transport), config_(config) {
}

void ElectManager::OnElectUpdated(const data::election::xelection_result_store_t & election_result_store, common::xzone_id_t const & zid) {
    using top::data::election::xelection_cluster_result_t;
    using top::data::election::xelection_group_result_t;
    using top::data::election::xelection_info_bundle_t;
    using top::data::election::xelection_info_t;
    using top::data::election::xelection_result_store_t;
    using top::data::election::xelection_result_t;

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
                        xdbg("[ElectManager::OnElectUpdated] %s %s", xip2_.to_string().c_str(), node_id.to_string().c_str());

                        // ElectNetNode enode{node_id.to_string(), election_info.consensus_public_key.to_string(), xip, "", associated_gid, version};
                        elect_data.push_back(router_node);
                    }
                    OnElectUpdated(elect_data);
                }
            }
        }
    }
}

void ElectManager::OnElectUpdated(std::vector<wrouter::WrouterTableNodes> const & elect_data) {
    // for(auto const &wrouter_node:elect_data){
    // }
    wrouter::SmallNetNodes::Instance()->AddNode(elect_data);
    for (auto const & wrouter_node : elect_data) {
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

    if (service_type.IsBroadcastService()) {
        wrouter::MultiRouting::Instance()->RemoveElectRoutingTable(service_type);
    } else if (wrouter::MultiRouting::Instance()->GetElectRoutingTable(service_type) != nullptr) {
        xinfo("ElectManager::UpdateRoutingTable get repeated routing table info xip2: service_type:%s", self_wrouter_nodes.m_xip2.to_string().c_str(), service_type.info().c_str());
        return;
    }

    base::Config config = config_;
    if (!config.Set("node", "network_id", self_wrouter_nodes.m_xip2.network_id().value())) {
        TOP_ERROR("set config node network_id [%d] failed!", self_wrouter_nodes.m_xip2.network_id().value());
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
        xerror("init election routing table failed!");
        return;
    }

    auto root_routing = wrouter::MultiRouting::Instance()->GetRootRoutingTable();

    std::map<std::string, base::KadmliaKeyPtr> elect_root_kad_key_ptrs;
    std::set<std::string> new_round_root_xip_set;
    for (auto _node : elect_data) {
        auto root_kad_key = base::GetRootKadmliaKey(_node.node_id);
        elect_root_kad_key_ptrs.insert(std::make_pair(_node.m_xip2.to_string(), root_kad_key));
        new_round_root_xip_set.insert(root_kad_key->Get());
    }

    std::map<std::string, kadmlia::NodeInfoPtr> last_round_out_nodes_map;

    // lastest election_nodes
    auto last_round_routing_table_ptr = wrouter::MultiRouting::Instance()->GetLastRoundRoutingTable(service_type);
    if (last_round_routing_table_ptr) {
        xdbg("ElectManager::UpdateRoutingTable find last round routing_table,service_type: %s this_round:%s",
             last_round_routing_table_ptr->get_local_node_info()->service_type().info().c_str(),
             service_type.info().c_str());
        auto last_round_nodes_map = last_round_routing_table_ptr->GetAllNodesRootKeyMap();
        for (auto const & _p : last_round_nodes_map) {
            auto const & last_election_xip2 = top::get<const std::string>(_p);
            auto const root_kad_key = top::get<base::KadmliaKeyPtr>(_p);
            auto const & root_xip = root_kad_key->Get();
            if (new_round_root_xip_set.find(root_xip) == new_round_root_xip_set.end()) {
                auto const & node_info = last_round_routing_table_ptr->GetNode(last_election_xip2);
                // here is possible to be nullptr (ths dst node never connected in this p2p)
                // but set it anyway, must be sure the index is same in every correct nodes
                // since the dst node(elected out) wasn't even be online in the whole last round, 
                // make sure won't core dump while try send packet.
                // assert(node_info != nullptr);
                last_round_out_nodes_map.insert({last_election_xip2, last_round_routing_table_ptr->GetNode(last_election_xip2)});
            }
        }
    }

    routing_table_ptr->SetElectionNodesExpected(elect_root_kad_key_ptrs, last_round_out_nodes_map);
    local_node_ptr->set_public_ip(root_routing->get_local_node_info()->public_ip());
    local_node_ptr->set_public_port(root_routing->get_local_node_info()->public_port());

    wrouter::MultiRouting::Instance()->AddElectRoutingTable(service_type, routing_table_ptr);
    wrouter::MultiRouting::Instance()->CheckElectRoutingTable(service_type);
    return;
}

void ElectManager::OnElectQuit(const common::xip2_t & xip2) {
    auto service_type = base::GetKadmliaKey(xip2)->GetServiceType();
    wrouter::MultiRouting::Instance()->RemoveElectRoutingTable(service_type);
    xdbg("OnElectQuit service_type:%lld xip2:%s", service_type.value(), xip2.to_string().c_str());
}

}  // namespace elect

}  // namespace top
