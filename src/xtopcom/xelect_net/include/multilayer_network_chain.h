#if 0
#pragma once

#include "xelect_net/include/multilayer_network.h"
#include "xelect_net/include/elect_manager.h"
#include "xdata/xchain_param.h"

namespace top {

namespace elect {
struct seeds_info_t {
	std::string ip;
	uint16_t port;
};

class MultilayerNetworkChain final : public xtrival_runnable_t<MultilayerNetworkChain>, public MultilayerNetwork {
public:
    MultilayerNetworkChain(MultilayerNetworkChain const &)             = delete;
    MultilayerNetworkChain & operator=(MultilayerNetworkChain const &) = delete;
    MultilayerNetworkChain(MultilayerNetworkChain &&)                  = delete;
    MultilayerNetworkChain & operator=(MultilayerNetworkChain &&)      = delete;
    ~MultilayerNetworkChain() override                                 = default;

    MultilayerNetworkChain(
            common::xnode_id_t const & node_id,
            const std::set<uint32_t>& xnetwork_id_set);

public:
    /**
     * @brief start multilayernetworkchain: init, then wait for elect data and build multiple elect p2p network
     * 
     */
    void start() override;
    /**
     * @brief stop multilayernetworkchain
     * 
     */
    void stop() override;
    /**
     * @brief init multilayernetworkchain
     * 
     * @param config config for multilayernetworkchain
     * @return true 
     * @return false 
     */
    bool Init(const base::Config& config) override;
    /**
     * @brief run multilaynetworkchain, will build a root kademlia routing table then wait for elect data to build elect p2p network
     * 
     * @param config config for multilayernetworkchain
     * @return true 
     * @return false 
     */
    bool Run(const base::Config& config) override;
    /**
     * @brief stop multilayernetworkchain
     * 
     */
    void Stop() override;
    /**
     * @brief Get the Service Neighbours object
     * 
     * @param xip2 xip is simliar to node type
     * @return std::vector<std::string> a list of nodes
     */
    std::vector<std::string> GetServiceNeighbours(const common::xip2_t& xip2);
public:
    /**
     * @brief Get the Elect Manager object
     * 
     * @return std::shared_ptr<ElectManager> 
     */
    inline std::shared_ptr<ElectManager> GetElectManager() const noexcept {
        return elect_manager_;
    }
    /**
     * @brief parse params and stored in edge_config
     * 
     * @param platform_param config struct from top level
     * @param edge_config after parse store config
     * @return int 
     */
    int HandleParamsAndConfig(const top::data::xplatform_params & platform_param, top::base::Config & edge_config);
    /**
     * @brief register callback to multilayernetworkchain
     * 
     * @param cb callback to register
     * @return int 
     */
    int RegisterNodeCallback(std::function<int32_t(std::string const & node_addr, std::string const & node_sign)> cb);
    /**
     * @brief add elect vhost instance and manage
     * 
     * @param xnetwork_id network id of this elect vhost
     * @param ec_vhost instance of elect vhost
     */
    void AddEcVhost(const uint32_t& xnetwork_id, const EcVHostPtr& ec_vhost);
    /**
     * @brief Get the Ec Vhost object of networkid
     * 
     * @param xnetwork_id network id of the target elect vhost
     * @return std::shared_ptr<network::xnetwork_driver_face_t> 
     */
    std::shared_ptr<network::xnetwork_driver_face_t> GetEcVhost(const uint32_t & xnetwork_id) const noexcept;

private:
    int JsonParseSeeds(const std::string& return_https, std::vector<seeds_info_t>& seeds_info);
private:
    mutable std::mutex vhost_map_mutex_;
    std::map<uint32_t, EcVHostPtr> vhost_map_;
    std::shared_ptr<ElectManager> elect_manager_ { nullptr };
    common::xnode_id_t m_node_id_;
    std::set<uint32_t> xnetwork_id_set_;
};

}  // namespace elect

}  // namespace top
#endif