// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#pragma once

#include "xwrouter/multi_routing/service_node_cache.h"
#include "xtransport/transport.h"
#include "xgossip/gossip_interface.h"


namespace top {

namespace gossip {

struct SuperNodeMsgKey {
public:
    SuperNodeMsgKey() {}
    SuperNodeMsgKey(uint32_t hash,uint64_t type)
                 :msg_hash(hash),
                 service_type(type) { }
    ~SuperNodeMsgKey(){}


    bool operator==(const SuperNodeMsgKey& key) const {
		return msg_hash == key.msg_hash && service_type == key.service_type;
	}
		
    bool operator<(const SuperNodeMsgKey& key) const {
        if(msg_hash < key.msg_hash) {
            return true;
        } else if(msg_hash == key.msg_hash && service_type < key.service_type) {
            return true;
        }
        return false;
    }

    uint32_t msg_hash;
    uint64_t service_type;
};

struct SuperNodeMsgHash {
	size_t operator()(const SuperNodeMsgKey& key)const {
		return std::hash<uint32_t>()(key.msg_hash) ^ std::hash<uint64_t>()(key.service_type);
    }
};

// using supernode to broadcast all for xnetwork_id
class GossipBloomfilterSuperNode : public GossipInterface {
public:
    explicit GossipBloomfilterSuperNode(transport::TransportPtr transport_ptr);
    GossipBloomfilterSuperNode(
        transport::TransportPtr transport_ptr,
        const GossipInterfacePtr& bloom_gossip_ptr,
        const GossipInterfacePtr& layer_gossip_ptr);
    virtual ~GossipBloomfilterSuperNode();
    virtual void Broadcast(
        uint64_t local_hash64,
        transport::protobuf::RoutingMessage& message,
        std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> neighbors);
    void Broadcast(
         transport::protobuf::RoutingMessage& message,
         kadmlia::RoutingTablePtr& routing_table);

    virtual void BroadcastWithNoFilter(
        const std::string& local_id,
        transport::protobuf::RoutingMessage& message,
        const std::vector<kadmlia::NodeInfoPtr>& neighbors);

private:
    std::weak_ptr<GossipInterface> bloom_gossip_ptr_;
    std::weak_ptr<GossipInterface> bloom_layer_gossip_ptr_;

    DISALLOW_COPY_AND_ASSIGN(GossipBloomfilterSuperNode);

    void SetServiceSuperNodesToBloom(
            transport::protobuf::RoutingMessage & message,
            const uint64_t & service_type,
            const std::vector<kadmlia::NodeInfoPtr> & super_nodes);
    void RestoreServiceSuperNodesToBloom(
            transport::protobuf::RoutingMessage & message,
            const uint64_t & service_type,
            const std::vector<kadmlia::NodeInfoPtr> & super_nodes,
            const std::shared_ptr<base::Uint64BloomFilter> & old_bloomfilter);

    void GetBroadcastInfo(
            const transport::protobuf::RoutingMessage & message,
            uint32_t & xnetwork_id,
            uint64_t & service_type,
            kadmlia::RoutingTablePtr & routing_table,
            uint64_t & local_hash64,
            bool & local_is_super_node,
            std::vector<kadmlia::NodeInfoPtr> &super_nodes);

    bool HaveSendToSevice(const SuperNodeMsgKey& gossip_key);
};
}  // namespace gossip

}  // namespace top
#endif