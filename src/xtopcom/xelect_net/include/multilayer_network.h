#pragma once
#include <vector>

#include "xbasic/xrunnable.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/args_parser.h"
#include "xtransport/transport.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xwrouter/xwrouter.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xelect_net/include/elect_manager_base.h"
#include "xelect_net/include/elect_vhost.h"
#include "xelect_net/include/multilayer_network_base.h"
#include "xdb/xdb.h"

namespace top {

namespace elect {

class MultilayerNetwork : public MultilayerNetworkBase {
public:
    MultilayerNetwork();
    MultilayerNetwork(MultilayerNetwork const &)             = delete;
    MultilayerNetwork & operator=(MultilayerNetwork const &) = delete;
    MultilayerNetwork(MultilayerNetwork &&)                  = delete;
    MultilayerNetwork & operator=(MultilayerNetwork &&)      = delete;
    ~MultilayerNetwork() override                            = default;

    int HandleParamsAndConfig(int argc, char** argv, top::base::Config& edge_config) override;
    bool Init(const base::Config& config) override;
    bool Run(const base::Config& config) override;
    void Stop() override;
public:
    /**
     * @brief Get the Ec Netcard object
     * 
     * @return EcNetcardPtr& 
     */
    inline EcNetcardPtr& GetEcNetcard() { 
        return ec_netcard_; 
    };
    /**
     * @brief create db instance
     * 
     * @param config config struct
     * @return int 
     */
    int InitDb(const base::Config& config);
protected:
    inline transport::TransportPtr& GetCoreTransport() {
        return core_transport_;
    };
    int ResetRootRouting(
            std::shared_ptr<transport::Transport> transport,
            const base::Config& config);
    bool ResetEdgeConfig(top::ArgsParser& args_parser, top::base::Config& edge_config);

private:
    void InitWrouter(
            top::transport::TransportPtr transport,
            std::shared_ptr<top::transport::MultiThreadHandler> message_handler);
    void RegisterCallbackForMultiThreadHandler(
            std::shared_ptr<top::transport::MultiThreadHandler> multi_thread_message_handler);
    std::shared_ptr<top::wrouter::RootRoutingManager> CreateRootManager(
            bool client,
            std::shared_ptr<transport::Transport> transport,
            const top::base::Config& config,
            const std::set<std::pair<std::string, uint16_t>>& public_endpoints_config);
    int ParseParams(int argc, char** argv, top::ArgsParser& args_parser) override;
    int KadKey_GetFromDb(
            base::KadmliaKeyPtr& kadkey,
            const std::string& db_field);
    int KadKey_StoreInDb(
            base::KadmliaKeyPtr& kadkey,
            const std::string& db_field);
    bool GetBootstrapCacheCallback(const uint64_t& service_type, VecBootstrapEndpoint& vec_bootstrap_endpoint);
    bool SetBootstrapCacheCallback(const uint64_t& service_type, const VecBootstrapEndpoint& vec_bootstrap_endpoint);

private:
    std::shared_ptr<wrouter::RootRoutingManager> root_manager_ptr_{ nullptr };
    transport::TransportPtr core_transport_{ nullptr };
    transport::TransportPtr nat_transport_{ nullptr };
    std::shared_ptr<db::xdb_face_t> net_db_ { nullptr };
    std::shared_ptr<transport::MultiThreadHandler> multi_message_handler_{ nullptr };
    EcNetcardPtr ec_netcard_ { nullptr };
};

}  // namespace elect

}  // namespace top
