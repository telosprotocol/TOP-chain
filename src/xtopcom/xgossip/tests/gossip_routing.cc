// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <string>
#include <memory>
#include <thread>

#include "xgossip/tests/gossip_routing.h"
#include "xgossip/include/gossip_utils.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xwrouter/register_routing_table.h"

namespace top {

namespace gossip {

int GossipBaseRouting::AddNode(kadmlia::NodeInfoPtr node) {
    return kadmlia::RoutingTable::AddNode(node);
}

bool GossipBaseRouting::NewNodeReplaceOldNode(kadmlia::NodeInfoPtr node, bool remove) {
    TOP_DEBUG("node_id(%s), pub(%s:%d)", HexSubstr(node->node_id).c_str(), node->public_ip.c_str(), node->public_port);
    const auto max_count = RoutingMaxNodesSize_;
    // const auto max_count = 16;  // for test
    if (nodes_.size() < max_count) {
        TOP_DEBUG("");
        return true;
    }

    std::map<uint32_t, unsigned int> bucket_rank_map;
    unsigned int max_bucket(kadmlia::kInvalidBucketIndex), max_bucket_count(0);
    std::for_each(
            std::begin(nodes_),
            std::end(nodes_),
        [&bucket_rank_map, &max_bucket, &max_bucket_count, node](const kadmlia::NodeInfoPtr & node_info) {
            bucket_rank_map[node_info->bucket_index] += 1;

            if (bucket_rank_map[node_info->bucket_index] >= max_bucket_count) {
                max_bucket = node_info->bucket_index;
                max_bucket_count = bucket_rank_map[node_info->bucket_index];
            }
    });

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
            TOP_DEBUG("newnodereplaceoldnode: ready_remove:%s ready_add:%s local:%s",
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

kadmlia::RoutingTablePtr CreateGossipRoutingTableForTest(
        transport::TransportPtr transport,
        uint32_t routing_max_node,
        uint32_t xnetwork_id,
        uint8_t zone_id,
        uint8_t cluster_id,
        uint8_t group_id) {
    base::XipParser xip;
    xip.set_xnetwork_id(xnetwork_id);
    xip.set_zone_id(zone_id);
    xip.set_cluster_id(cluster_id);
    xip.set_group_id(group_id);

    base::KadmliaKeyPtr kad_key = GetKadmliaKey(xip);
    auto old_rt = wrouter::GetRoutingTable(kad_key->GetServiceType(), false);
    if (old_rt) {
        return old_rt;
    }

    kadmlia::LocalNodeInfoPtr local_node_ptr = nullptr;
    local_node_ptr.reset(new top::kadmlia::LocalNodeInfo());
    if (!local_node_ptr->Init(
            "127.0.0.1",
            9000,
            false,
            false,
            "",
            kad_key,
            kad_key->xnetwork_id(),
            kRoleInvalid)) {
        TOP_WARN("local_node_ptr init failed");
        return nullptr;
    }
    kadmlia::RoutingTablePtr rt =  std::make_shared<GossipBaseRouting>(transport, local_node_ptr, routing_max_node);
    if (!rt->Init()) {
        TOP_WARN("gossip routing table init failed");
        return nullptr;
    }
    
    for (uint32_t i = 0; i < routing_max_node; ++i) {
        std::string account = RandomString(60);
        auto kad_key_ptr = base::GetKadmliaKey(xip, account);
        kadmlia::NodeInfoPtr node;
        node.reset(new kadmlia::NodeInfo(kad_key_ptr->Get()));
        node->public_ip = "122.122.122.122";
        uint32_t port = RandomUint32() % 200 + 10000;
        node->public_port = port; 
        node->local_ip = "127.0.0.1";
        node->local_port = port;
        node->nat_type = kadmlia::kNatTypePublic;
        node->hash64 = base::xhash64_t::digest(node->node_id);

        rt->AddNode(node);
    }

    if (!rt) {
        return nullptr;
    }
    wrouter::RegisterRoutingTable(kad_key->GetServiceType(), rt);

    return rt;
}

kadmlia::RoutingTablePtr CreateGossipRootRoutingTableForTest(
        transport::TransportPtr transport,
        uint32_t size,
        const base::Config& config) {
    // kRoot kadkey
    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(global_node_id, true);
    base::XipParser xip = kad_key->Xip();

    auto old_rt = wrouter::GetRoutingTable(kad_key->GetServiceType(), true);
    if (old_rt) {
        return old_rt;
    }

    auto get_cache_callback = [](const uint64_t& service_type, std::vector<std::pair<std::string, uint16_t>>& vec_bootstrap_endpoint) -> bool {
        return true;
    };
    auto set_cache_callback = [](const uint64_t& service_type, const std::vector<std::pair<std::string, uint16_t>>& vec_bootstrap_endpoint) -> bool {
        return true;
    };

    auto root_manager_ptr = wrouter::RootRoutingManager::Instance();
    wrouter::SetRootRoutingManager(root_manager_ptr);
    if (root_manager_ptr->AddRoutingTable(
            transport,
            config,
            kad_key,
            get_cache_callback,
            set_cache_callback,
            false) != top::kadmlia::kKadSuccess) {
        TOP_ERROR("add root_table[root] failed!");
        return nullptr;
    }
    auto root_rt = wrouter::GetRoutingTable(kad_key->GetServiceType(), true);

    for (uint32_t i = 0; i < size; ++i) {
        std::string account = RandomString(60);
        auto kad_key_ptr = base::GetKadmliaKey(account, true);
        kadmlia::NodeInfoPtr node;
        node.reset(new kadmlia::NodeInfo(kad_key_ptr->Get()));
        node->public_ip = "122.122.122.122";
        uint32_t port = RandomUint32() % 200 + 10000;
        node->public_port = port; 
        node->local_ip = "127.0.0.1";
        node->local_port = port;
        node->nat_type = kadmlia::kNatTypePublic;
        node->hash64 = base::xhash64_t::digest(node->node_id);

        root_rt->AddNode(node);
    }
    return root_rt;
}


void CreateMessageForTest(transport::protobuf::RoutingMessage& message, const std::string& src, const std::string& des) {
    // make message
    message.set_is_root(false);
    message.set_broadcast(true);
    message.set_type(100); // any msg type
    message.set_priority(enum_xpacket_priority_type_routine);
    message.set_src_node_id(src);
    message.set_des_node_id(des);
    message.set_id(kadmlia::CallbackManager::MessageId());
    message.set_data(RandomString(100));

    // broadcast block
    auto gossip_block = message.mutable_gossip();
    gossip_block->set_neighber_count(3);
    gossip_block->set_gossip_type(gossip::kGossipBloomfilter);
    gossip_block->set_max_hop_num(10);
    gossip_block->set_evil_rate(0);
    if (message.is_root()) {
        gossip_block->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    } else {
        gossip_block->set_ign_bloomfilter_level(0);
    }
    gossip_block->set_left_overlap(0);
    gossip_block->set_right_overlap(0);
    gossip_block->set_block(message.data());

    uint32_t vhash = base::xhash32_t::digest(message.data());
    std::string header_hash = std::to_string(vhash);
    gossip_block->set_header_hash(header_hash);

    uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
    gossip_block->set_msg_hash(msg_hash);
}

}  // namespace gossip

}  // namespace top
