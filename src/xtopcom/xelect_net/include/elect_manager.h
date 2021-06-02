#pragma once

#include "xdata/xelection/xelection_result_store.h"
#include "xelect_net/include/elect_manager_base.h"
#include "xelect_net/include/node_manager_base.h"
#include "xpbase/base/top_config.h"
#include "xtransport/proto/transport.pb.h"
#include "xtransport/transport.h"

#include <memory>

namespace top {

namespace elect {

class ElectManager : public ElectManagerBase {
public:
    ElectManager(ElectManager const &) = delete;
    ElectManager & operator=(ElectManager const &) = delete;
    ElectManager(ElectManager &&) = delete;
    ElectManager & operator=(ElectManager &&) = delete;

    ElectManager(transport::TransportPtr transport, const base::Config & config);

    ~ElectManager() override = default;

public:
    /**
     * @brief start ElectManagerMulNet, build elect network and replace node
     *
     * @return true
     * @return false
     */
    bool Start() override;
    /**
     * @brief build or update elect p2p-network base elect data
     *
     * @param elect_data contain elect info, such as node_type,xip,account...
     */
    void OnElectUpdated(const std::vector<ElectNetNode> & elect_data) override;

    /**
     * @brief build or update elect p2p-network base elect data
     *
     * @param election_result_store contain elect info, such as node_type,xip,account..
     * @param zid zone id
     */
    void OnElectUpdated(const data::election::xelection_result_store_t & election_result_store, common::xzone_id_t const & zid);
    /**
     * @brief destory elect p2p-network
     *
     * @param xip2 xip of p2p network to destory
     * @return int
     */
    int OnElectQuit(const common::xip2_t & xip2);

private:
    transport::TransportPtr transport_{nullptr};
    base::Config config_;
    std::shared_ptr<NodeManagerBase> node_manager_{nullptr};
};

}  // namespace elect

}  // namespace top
