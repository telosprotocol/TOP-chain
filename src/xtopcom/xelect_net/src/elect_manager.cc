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

#include <chrono>
#include <limits>

namespace top {

namespace elect {

ElectManager::ElectManager(transport::TransportPtr transport, const base::Config & config) : transport_(transport), config_(config) {
}

bool ElectManager::Start() {
    node_manager_ = std::make_shared<NodeManager>(transport_, config_);
    if (node_manager_->Init() != top::kadmlia::kKadSuccess) {
        TOP_ERROR("node_manager init fail.");
        node_manager_ = nullptr;
        return false;
    }
    return true;
}

void ElectManager::OnElectUpdated(const data::election::xelection_result_store_t & election_result_store, common::xzone_id_t const & zid) {
    using top::data::election::xelection_cluster_result_t;
    using top::data::election::xelection_group_result_t;
    using top::data::election::xelection_info_bundle_t;
    using top::data::election::xelection_info_t;
    using top::data::election::xelection_result_store_t;
    using top::data::election::xelection_result_t;
    std::vector<ElectNetNode> elect_data;

    TOP_DEBUG("elect_manager onelectupdated begin");
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

                    for (auto const & node_info : group_result) {
                        auto const & node_id = top::get<xelection_info_bundle_t>(node_info).node_id();
                        if (node_id.empty()) {
                            continue;
                        }
                        auto const & election_info = top::get<xelection_info_bundle_t>(node_info).election_info();

                        uint64_t version = std::numeric_limits<uint64_t>::max();
                        if (group_result.group_version().has_value()) {
                            version = group_result.group_version().value();
                        }

                        uint8_t associated_gid = 0;
                        if (group_result.associated_group_id().has_value()) {
                            associated_gid = group_result.associated_group_id().value();
                        }

                        // base::XipParser xip(xip2.raw_high_part(), xip2.raw_low_part());
                        base::XipParser xip;
                        xip.set_xnetwork_id(static_cast<uint32_t>(xip2.network_id().value()));
                        xip.set_zone_id(static_cast<uint8_t>(xip2.zone_id().value()));
                        xip.set_cluster_id(static_cast<uint8_t>(xip2.cluster_id().value()));
                        xip.set_group_id(static_cast<uint8_t>(xip2.group_id().value()));
                        // xip.set_network_type((uint8_t)(address.cluster_address().type()));
                        ElectNetNode enode{node_id.to_string(), election_info.consensus_public_key.to_string(), xip, "", associated_gid, version};
                        elect_data.push_back(enode);
                    }  // end for (auto const & node_info : group_result) {
                }      // end for (auto const & group_result_info : cluster_result) {
            }          // end for (auto const & cluster_result_info : election_result) {
        }
    }  // end for (auto const & election_result_info : election_result_store) {

    TOP_INFO("elect_manager onelectupdated end, size:%u", elect_data.size());
    return OnElectUpdated(elect_data);
}

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

// unregister routing table
int ElectManager::OnElectQuit(const common::xip2_t & xip2) {
    base::XipParser xip;
    xip.set_xnetwork_id(static_cast<uint32_t>(xip2.network_id().value()));
    xip.set_zone_id(static_cast<uint8_t>(xip2.zone_id().value()));
    xip.set_cluster_id(static_cast<uint8_t>(xip2.cluster_id().value()));
    xip.set_group_id(static_cast<uint8_t>(xip2.group_id().value()));
    // xip.set_network_type((uint8_t)(address.cluster_address().type()));

    TOP_INFO("electquit for xip:%s", HexEncode(xip.xip()).c_str());
    return node_manager_->Quit(xip);
}

}  // namespace elect

}  // namespace top
