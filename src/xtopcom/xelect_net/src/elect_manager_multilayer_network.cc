#include "xelect_net/include/elect_manager_multilayer_network.h"

#include <chrono>
#include <limits>

#include "xpbase/base/top_log.h"
#include "xpbase/base/xip_parser.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xchain_timer/xchain_timer.h"
#include "xelect_net/include/node_manager.h"
#include "xelect_net/include/elect_uitils.h"
#include "xbasic/xelapsed_time.h"

#include "xbasic/xutility.h"

namespace top {

namespace elect {

ElectManagerMulNet::ElectManagerMulNet(
        transport::TransportPtr transport,
        const base::Config& config)
        : transport_(transport),
          config_(config) {
}

int ElectManagerMulNet::Join(const base::XipParser& xip) {
    return node_manager_->Join(xip);
}

int ElectManagerMulNet::Quit(const base::XipParser& xip) {
    return node_manager_->Quit(xip);
}

// unregister routing table
int ElectManagerMulNet::OnElectQuit(const base::XipParser& xip) {
    TOP_INFO("electquit for xip:%s", HexEncode(xip.xip()).c_str());
    return Quit(xip);
}

int ElectManagerMulNet::OnElectDropNodes(
        const base::XipParser& xip,
        const std::vector<std::string>& drop_accounts) {
    return node_manager_->DropNodes(xip, drop_accounts);
}

bool ElectManagerMulNet::Start() {
    node_manager_ = std::make_shared<NodeManager>(transport_, config_);
    if (node_manager_->Init() != top::kadmlia::kKadSuccess) {
        TOP_ERROR("node_manager init fail.");
        node_manager_ = nullptr;
        return false;
    }
    return true;
}


class Trace {
public:
    Trace() {
    }
    ~Trace() {
        if (elapsed_time_.elapsed() > 5) {
            TOP_FATAL("############## OnElectUpdated %lf s ##############", elapsed_time_.elapsed());
        }
    }
private:
    xelapsed_time elapsed_time_;
};


void ElectManagerMulNet::OnElectUpdated(const std::vector<ElectNetNode>& elect_data) {
    TOP_DEBUG("onelectupdated begin, size:%u", elect_data.size());
    for (const auto& enode : elect_data) {
        wrouter::NetNode snode {enode.m_account, enode.m_public_key, enode.m_xip, "", enode.m_associated_gid, enode.m_version};
        wrouter::SmallNetNodes::Instance()->AddNode(snode);

        if (global_node_id != enode.m_account) {
            TOP_DEBUG("account not match: self:%s iter:%s", global_node_id.c_str(), enode.m_account.c_str());
           continue;
        }
        TOP_INFO("account match: self:%s iter:%s xip: %s xnetwork_id: %u zone_id: %u cluster_id: %u group_id: %u",
                global_node_id.c_str(),
                enode.m_account.c_str(),
                HexEncode(enode.m_xip.xip()).c_str(),
                enode.m_xip.xnetwork_id(),
                enode.m_xip.zone_id(),
                enode.m_xip.cluster_id(),
                enode.m_xip.group_id());
        if (Join(enode.m_xip) != kadmlia::kKadSuccess) {
            TOP_ERROR("node join failed!");
        }
    }

    TOP_DEBUG("onelectupdated end");
    return;
}

}  // namespace elect

}  // namespace top
