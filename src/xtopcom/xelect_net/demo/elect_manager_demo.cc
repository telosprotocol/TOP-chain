#include "xelect_net/demo/elect_manager_demo.h"

#include "xpbase/base/top_log.h"
#include "xpbase/base/xip_parser.h"
#include "xwrouter/multi_routing/small_net_cache.h"

#include <chrono>
#include <iomanip>
#include <limits>

namespace top {

namespace elect {

ElectManagerDemo::ElectManagerDemo(transport::TransportPtr transport, const base::Config & config) : ElectManagerMulNet(transport, config) {
}

void ElectManagerDemo::OnElectUpdated(json all_info) {
    std::vector<ElectNetNode> elect_data;
    for (json::iterator it = all_info.begin(); it != all_info.end(); ++it) {
        auto key = it.key();
        std::cout << "all_info_key:" << key << std::endl;
        if (key == "all") {
            std::cout << "read all node_id:" << std::endl;
            std::cout << it.value() << std::endl;
            continue;
        }
        if (key == "fullnode") {
            std::cout << "read fullnode_id:" << std::endl;
            std::cout << it.value() << std::endl;
            continue;
        }
        std::cout << std::setw(20) << "node_id" << std::setw(20) << "pubkey" << std::setw(36) << "xip" << std::setw(4) << "gid" << std::endl;
        for (json::iterator lit = it.value().begin(); lit != it.value().end(); ++lit) {
            std::string node_id = (*lit)["node_id"];
            std::string public_key = (*lit)["pubkey"];
            std::string xipstr = (*lit)["xip"];
            uint32_t gid = (*lit)["gid"];

            auto sub_node_id = node_id.substr(0, 6) + "..." + node_id.substr(node_id.size() - 6, 6);
            auto sub_public_key = public_key.substr(0, 6) + "..." + public_key.substr(public_key.size() - 6, 6);
            std::cout << std::setw(20) << sub_node_id << std::setw(20) << sub_public_key << std::setw(36) << xipstr << std::setw(4) << gid << std::endl;

            base::XipParser xip(HexDecode(xipstr));
            ElectNetNode enode{node_id, public_key, xip, "", static_cast<uint8_t>(gid), static_cast<uint64_t>(1)};  // all version is 1
            elect_data.push_back(enode);

        }  // end for (json::iterator lit = it.value().begin()
    }      // end for (json::iterator it = all_info.begin()

    TOP_INFO("elect_manager_demo onelectupdated end, size:%u", elect_data.size());
    return ElectManagerMulNet::OnElectUpdated(elect_data);
}

}  // namespace elect

}  // namespace top
