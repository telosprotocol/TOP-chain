#pragma once

#include "xdata/xelection/xelection_result_store.h"
#include "xpbase/base/top_config.h"
#include "xtransport/transport.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/multi_routing/net_node.h"

#include <memory>

namespace top {

namespace elect {

class ElectManager {
public:
    ElectManager(ElectManager const &) = delete;
    ElectManager & operator=(ElectManager const &) = delete;
    ElectManager(ElectManager &&) = delete;
    ElectManager & operator=(ElectManager &&) = delete;

    ElectManager(transport::TransportPtr transport, const base::Config & config);

    ~ElectManager() = default;

public:
    void OnElectUpdated(const data::election::xelection_result_store_t & election_result_store, common::xzone_id_t const & zid, std::uint64_t associated_blk_height);

    void OnElectQuit(const common::xip2_t & xip2);

private:
    void OnElectUpdated(std::vector<wrouter::WrouterTableNode> const & elect_data, common::xip2_t const & group_xip, uint64_t routing_table_blk_height);
    void UpdateRoutingTable(std::vector<wrouter::WrouterTableNode> const & elect_data, wrouter::WrouterTableNode const & self_wrouter_nodes);

private:
    transport::TransportPtr transport_{nullptr};
    base::Config config_;
};

}  // namespace elect

}  // namespace top
