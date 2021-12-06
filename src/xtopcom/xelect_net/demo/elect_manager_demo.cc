#include "xelect_net/demo/elect_manager_demo.h"

#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xwrouter/multi_routing/small_net_cache.h"

#include <chrono>
#include <iomanip>
#include <limits>

namespace top {

namespace elect {

ElectManagerDemo::ElectManagerDemo(transport::TransportPtr transport, const base::Config & config) : ElectManager(transport, config) {
}

void ElectManagerDemo::OnElectUpdated(json all_info) {
    for (json::iterator it = all_info.begin(); it != all_info.end(); ++it) {
        auto key = it.key();
        std::cout << "all_info_key:" << key << std::endl;
        if (key == "all") {
            std::cout << "read all node_id:" << std::endl;
            std::cout << it.value() << std::endl;
            continue;
        }
        if (key == "exchange") {
            std::cout << "read exchange_id:" << std::endl;
            std::cout << it.value() << std::endl;
            continue;
        }

        std::vector<wrouter::WrouterTableNodes> elect_data;
        for (json::iterator lit = it.value().begin(); lit != it.value().end(); ++lit) {
            std::string node_id = (*lit)["node_id"];
            std::string xipstr = (*lit)["xip"];
            auto kad_key = base::GetKadmliaKey(xipstr);
            wrouter::WrouterTableNodes router_node{kad_key->Xip(), node_id};
            std::cout << std::setw(20) << node_id << std::setw(36) << xipstr << std::endl;
            elect_data.push_back(router_node);
        }
        ElectManager::OnElectUpdated(elect_data);
    }
}

}  // namespace elect

}  // namespace top
