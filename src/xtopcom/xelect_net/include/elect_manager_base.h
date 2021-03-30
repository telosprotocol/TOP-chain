#pragma once
#include <vector>


namespace top {

namespace wrouter {
    struct NetNode;
} // end namespace wrouter

namespace base  {
    class XipParser;
} // end namespace base

namespace elect {
using ElectNetNode = wrouter::NetNode;

class ElectManagerBase {
public:
    ElectManagerBase()                                     = default;
    ElectManagerBase(ElectManagerBase const &)             = delete;
    ElectManagerBase & operator=(ElectManagerBase const &) = delete;
    ElectManagerBase(ElectManagerBase &&)                  = delete;
    ElectManagerBase & operator=(ElectManagerBase &&)      = delete;
    virtual ~ElectManagerBase()                            = default;               

    /**
     * @brief start ElectManager, build elect network and replace node
     * 
     * @return true 
     * @return false 
     */
    virtual bool Start() = 0;
    /**
     * @brief build or update elect p2p-network base elect data
     * 
     * @param elect_data contain elect info, such as node_type,xip,account...
     */
    virtual void OnElectUpdated(const std::vector<ElectNetNode>& elect_data) = 0;
    /**
     * @brief destory elect p2p-network
     * 
     * @param xip destory a p2p-network of xip, xip is similar to network type
     * @return int 
     */
    virtual int OnElectQuit(const base::XipParser& xip) = 0;
    /**
     * @brief drop node from p2p-network
     * 
     * @param xip xip is similar to network type
     * @param drop_accounts list of nodes to drop from p2p network
     * @return int 
     */
    virtual int OnElectDropNodes(
            const base::XipParser& xip,
            const std::vector<std::string>& drop_accounts) = 0;
};

}  // namespace elect

}  // namespace top
