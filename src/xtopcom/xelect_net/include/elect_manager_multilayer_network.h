#pragma once

#include <memory>

#include "xelect_net/include/elect_manager_base.h"
#include "xpbase/base/top_config.h"
#include "xtransport/transport.h"
#include "xtransport/proto/transport.pb.h"
#include "xelect_net/include/node_manager_base.h"
#include "xdata/xelection/xelection_result_store.h"

namespace top {

namespace elect {

class ElectManagerMulNet : public ElectManagerBase {
public:
    ElectManagerMulNet(ElectManagerMulNet const &)              = delete;
    ElectManagerMulNet& operator=(ElectManagerMulNet const &)   = delete;
    ElectManagerMulNet(ElectManagerMulNet &&)                   = delete;
    ElectManagerMulNet& operator=(ElectManagerMulNet &&)        = delete;

    ElectManagerMulNet(
            transport::TransportPtr transport,
            const base::Config& config);

    ~ElectManagerMulNet() override                              = default;

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
    void OnElectUpdated(const std::vector<ElectNetNode>& elect_data) override;
    /**
     * @brief destory elect p2p-network
     * 
     * @param xip destory a p2p-network of xip, xip is similar to network type
     * @return int 
     */
    int OnElectQuit(const base::XipParser& xip) override;
    /**
     * @brief drop node from p2p-network
     * 
     * @param xip xip is similar to network type
     * @param drop_accounts list of nodes to drop from p2p network
     * @return int 
     */
    int OnElectDropNodes(
            const base::XipParser& xip,
            const std::vector<std::string>& drop_accounts) override;
protected:
    virtual int Join(const base::XipParser& xip);
    virtual int Quit(const base::XipParser& xip);

private:
    transport::TransportPtr transport_{ nullptr };
    base::Config config_;
    std::shared_ptr<NodeManagerBase> node_manager_{ nullptr };
};

}  // namespace elect

}  // namespace top
