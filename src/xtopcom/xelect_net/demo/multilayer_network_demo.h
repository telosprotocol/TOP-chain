#pragma once

#include "xelect_net/demo/elect_manager_demo.h"
#include "xelect_net/demo/elect_command.h"
#include "xelect_net/include/multilayer_network.h"

namespace top {

namespace elect {

class MultilayerNetworkDemo final : public MultilayerNetwork {
public:
    MultilayerNetworkDemo()
      : MultilayerNetwork{common::xnode_id_t{global_node_id}, std::set<uint32_t>{255}} {

        };
    MultilayerNetworkDemo(MultilayerNetworkDemo const &) = delete;
    MultilayerNetworkDemo & operator=(MultilayerNetworkDemo const &) = delete;
    MultilayerNetworkDemo(MultilayerNetworkDemo &&) = delete;
    MultilayerNetworkDemo & operator=(MultilayerNetworkDemo &&) = delete;
    ~MultilayerNetworkDemo() override = default;

public:
    int HandleParamsAndConfig(int argc, char ** argv, top::base::Config & edge_config);
    bool Init(const base::Config & config);
    bool Run(const base::Config & config);
    void Stop();
    bool BuildXelectNetDemoNetwork();

public:
    inline ElectCommands& GetElectCmd() {
        return elect_cmd_;
    }
    inline std::shared_ptr<ElectManager> GetElectManager() const noexcept {
        return elect_manager_;
    }

private:
    int ParseParams(int argc, char ** argv, top::ArgsParser & args_parser);
    bool GenerateXelectNetDemoNodes(const top::base::Config & edge_config);

private:
    ElectCommands elect_cmd_;
    std::shared_ptr<ElectManagerDemo> elect_manager_{nullptr};
};

using MulNetDemo = MultilayerNetworkDemo;

}  // namespace elect

}  // namespace top
