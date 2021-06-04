#if 0
#pragma once
#include <vector>

namespace top {

namespace wrouter {
struct NetNode;
}  // end namespace wrouter

namespace elect {
using ElectNetNode = wrouter::NetNode;

class ElectManagerBase {
public:
    ElectManagerBase() = default;
    ElectManagerBase(ElectManagerBase const &) = delete;
    ElectManagerBase & operator=(ElectManagerBase const &) = delete;
    ElectManagerBase(ElectManagerBase &&) = delete;
    ElectManagerBase & operator=(ElectManagerBase &&) = delete;
    virtual ~ElectManagerBase() = default;

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
    virtual void OnElectUpdated(const std::vector<ElectNetNode> & elect_data) = 0;
};

}  // namespace elect

}  // namespace top
#endif