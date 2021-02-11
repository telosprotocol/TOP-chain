#pragma once

#include <memory>

#include "xelect_net/include/elect_manager_multilayer_network.h"
#include "xpbase/base/top_config.h"
#include "xtransport/transport.h"
#include "xtransport/proto/transport.pb.h"
#include "xelect_net/include/node_manager_base.h"
#include "xdata/xelection/xelection_result_store.h"

namespace top {

namespace elect {

class ElectManagerChain : public ElectManagerMulNet {
public:
    ElectManagerChain(ElectManagerChain const &)              = delete;
    ElectManagerChain& operator=(ElectManagerChain const &)   = delete;
    ElectManagerChain(ElectManagerChain &&)                   = delete;
    ElectManagerChain& operator=(ElectManagerChain &&)        = delete;

    ElectManagerChain(
            transport::TransportPtr transport,
            const base::Config& config);

    ~ElectManagerChain() override                             = default;

public:
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
    /**
     * @brief build or update elect p2p-network base elect data
     * 
     * @param election_result_store contain elect info, such as node_type,xip,account..
     * @param zid zone id
     */
    void OnElectUpdated(
            const data::election::xelection_result_store_t& election_result_store,
            common::xzone_id_t const & zid);
    /**
     * @brief destory elect p2p-network
     * 
     * @param xip2 xip of p2p network to destory
     * @return int 
     */
    int OnElectQuit(const common::xip2_t& xip2);
    /**
     * @brief drop node from p2p-network
     * 
     * @param chain_xip_high high address of xip
     * @param chain_xip_low low address of xip
     * @param drop_accounts list of nodes to drop from p2p networ
     * @return int 
     */
    int OnElectDropNodes(
            uint64_t chain_xip_high,
            uint64_t chain_xip_low,
            const std::vector<std::string>& drop_accounts);
};

}  // namespace elect

}  // namespace top
