#include "xelect_net/include/elect_routing.h"

#include <functional>
#include <memory>

#include "xkad/proto/kadmlia.pb.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_log_name.h"
#include "xpbase/base/top_string_util.h"
#include "xpbase/base/xip_generator.h"
#include "xpbase/base/kad_key/chain_kadmlia_key.h"
#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#include "xkad/routing_table/nodeid_utils.h"
#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/xwrouter.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xtransport/udp_transport/transport_util.h"

namespace top {

using namespace kadmlia;  // NOLINT

namespace elect {

ElectRouting::ElectRouting(
        std::shared_ptr<transport::Transport> transport,
        kadmlia::LocalNodeInfoPtr local_node_ptr,
        const uint32_t RoutingMaxNodesSize)
        : wrouter::WrouterBaseRouting(transport, kNodeIdSize, local_node_ptr),
        RoutingMaxNodesSize_(RoutingMaxNodesSize) {}

ElectRouting::~ElectRouting() {}

bool ElectRouting::Init() {
    if (!wrouter::WrouterBaseRouting::Init()) {
        TOP_ERROR("init routing table failed");
        return false;
    }
    // return SupportRumor(false);
    return true;
}

bool ElectRouting::UnInit() {
    return wrouter::WrouterBaseRouting::UnInit();
}


bool ElectRouting::NewNodeReplaceOldNode(NodeInfoPtr node, bool remove) {
    TOP_DEBUG_NAME("node_id(%s), pub(%s:%d)", HexSubstr(node->node_id).c_str(), node->public_ip.c_str(), node->public_port);
    const auto max_count = RoutingMaxNodesSize_;
    // const auto max_count = 16;  // for test
    if (nodes_.size() < max_count) {
        TOP_DEBUG_NAME("");
        return true;
    }

    std::map<uint32_t, unsigned int> bucket_rank_map;
    unsigned int max_bucket(kInvalidBucketIndex), max_bucket_count(0);
    std::for_each(
            std::begin(nodes_),
            std::end(nodes_),
        [&bucket_rank_map, &max_bucket, &max_bucket_count, node](const NodeInfoPtr & node_info) {
            bucket_rank_map[node_info->bucket_index] += 1;

            if (bucket_rank_map[node_info->bucket_index] >= max_bucket_count) {
                max_bucket = node_info->bucket_index;
                max_bucket_count = bucket_rank_map[node_info->bucket_index];
            }
    });

    // not satisfy replacing
    // if (max_bucket_count <= kKadParamK) {
    //     // first node in empty k-bucket, add directly
    //     if (bucket_rank_map[node->bucket_index] < kKadParamK) {
    //         TOP_DEBUG_NAME("");
    //         return true;
    //     }

    //     // no replace
    //     TOP_DEBUG_NAME("");
    //     return false;
    // }

    // dump all nodes
    // {
    //     std::string fmt("all nodes:\n");
    //     for (int i = 0; i < nodes_.size(); ++i) {
    //         // fmt += base::StringUtil::str_fmt("%d: count(%d)\n", kv.first, kv.second);
    //         fmt += base::StringUtil::str_fmt("%3d]: %s, %s:%d, dis(%d)\n", (int)i, HexSubstr(nodes_[i]->node_id).c_str(),
    //                 nodes_[i]->public_ip.c_str(), (int)nodes_[i]->public_port, nodes_[i]->bucket_index);
    //     }
    //     TOP_DEBUG_NAME("%s", fmt.c_str());
    // }

    // replace node
    for (auto it(nodes_.rbegin()); it != nodes_.rend(); ++it) {
        if (static_cast<unsigned int>((*it)->bucket_index) != max_bucket ||
                node->bucket_index == (*it)->bucket_index) {
            continue;
        }

        const bool very_less = bucket_rank_map[node->bucket_index] < bucket_rank_map[(*it)->bucket_index] - 1;
        const bool less_and_closer = bucket_rank_map[node->bucket_index] < bucket_rank_map[(*it)->bucket_index]
                && node->bucket_index < (*it)->bucket_index;
        const bool empty_and_closer = bucket_rank_map[node->bucket_index] == 0
                && node->bucket_index < (*it)->bucket_index;
        if (very_less || less_and_closer || empty_and_closer) {
            if (!remove) {
                return true;
            }
            TOP_DEBUG_NAME("newnodereplaceoldnode: ready_remove:%s ready_add:%s local:%s",
                    HexEncode((*it)->node_id).c_str(),
                    HexEncode(node->node_id).c_str(),
                    HexEncode(local_node_ptr_->id()).c_str());
            {
                std::unique_lock<std::mutex> set_lock(node_id_map_mutex_);
                auto id_map_iter = node_id_map_.find((*it)->node_id);
                if (id_map_iter != node_id_map_.end()) {
                    node_id_map_.erase(id_map_iter);
                }
            }
            {
                std::unique_lock<std::mutex> lock_hash(node_hash_map_mutex_);
                auto hash_iter = node_hash_map_->find((*it)->hash64);
                if (hash_iter != node_hash_map_->end()) {
                    node_hash_map_->erase(hash_iter);
                }
            }
            
            EraseNode(--(it.base()));
            return true;
        } // end if (replace...
    } // end for

    return false;

}

uint32_t ElectRouting::GetFindNodesMaxSize() {
    return RoutingMaxNodesSize_;
}

int ElectRouting::CheckElectHasNode(const std::string& node_id) {
    bool flag = wrouter::SmallNetNodes::Instance()->AllowAdd(node_id);
    if (flag) {
        return kKadSuccess;
    }
    return kKadFailed;
}


int ElectRouting::AddNode(NodeInfoPtr node) {
    if (CheckElectHasNode(node->node_id) == kKadFailed) {
        TOP_WARN("ElectRouting AddNode Check elect data failed of node:%s", HexEncode(node->node_id).c_str());
        return kKadFailed;
    }

    int res = wrouter::WrouterBaseRouting::AddNode(node);
    if (res != kKadSuccess) {
        return res;
    }
    return kKadSuccess;
}

int ElectRouting::DropNode(kadmlia::NodeInfoPtr node) {
    int res = wrouter::WrouterBaseRouting::DropNode(node);
    if (res != kKadSuccess) {
        return res;
    }
    return res;
}

bool ElectRouting::SetJoin(
        const std::string& boot_id,
        const std::string& boot_ip,
        int boot_port) {
    return wrouter::WrouterBaseRouting::SetJoin(boot_id, boot_ip, boot_port);
}

}  // namespace elect

}  // namespace top
