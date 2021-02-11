// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <queue>
#include <unordered_map>
#include <memory>

#include "xpbase/base/uint64_bloomfilter.h"
#include "xpbase/base/top_timer.h"
#include "xtransport/proto/transport.pb.h"

namespace top {

namespace gossip {

struct MessageKey{
public:
    MessageKey() {}
    MessageKey(uint8_t flag,uint32_t hash,uint64_t type)
                 :super_flag(flag),
                 msg_hash(hash),
                 service_type(type) { }
    ~MessageKey(){}

    bool operator==(const MessageKey& key) const {
		return msg_hash == key.msg_hash && service_type == key.service_type;
	}
		
    bool operator<(const MessageKey& key) const {
        if(msg_hash < key.msg_hash) {
            return true;
        } else if(msg_hash == key.msg_hash && service_type < key.service_type) {
            return true;
        }
        return false;
    }
    
    uint8_t  super_flag;
    uint32_t msg_hash;
    uint64_t service_type;
};

struct MessageHash {
	size_t operator()(const MessageKey& key)const {
		return std::hash<uint64_t>()(key.msg_hash) ^ std::hash<uint64_t>()(key.service_type);
    }
};

struct GossipItem {
    std::shared_ptr<base::Uint64BloomFilter> bloom_filter;
    uint32_t sendout_times;
};

class MessageWithBloomfilter {
public:
    static MessageWithBloomfilter* Instance();
    std::shared_ptr<base::Uint64BloomFilter> GetMessageBloomfilter(
            transport::protobuf::RoutingMessage& message);
    bool StopGossip(const MessageKey&, uint32_t);

private:
    MessageWithBloomfilter() {}
    ~MessageWithBloomfilter() {}

    std::shared_ptr<base::Uint64BloomFilter> MergeBloomfilter(
            const uint32_t&,
            std::shared_ptr<base::Uint64BloomFilter>& bloomfilter,
            const uint32_t& stop_times,
            bool& stop_gossip);

    // static const uint32_t kMaxMessageQueueSize = 1048576u;
    static const uint32_t kMaxMessageQueueSize = 508576u;

    std::unordered_map<MessageKey, uint8_t,MessageHash> messsage_bloomfilter_map_;
    std::mutex messsage_bloomfilter_map_mutex_;

    DISALLOW_COPY_AND_ASSIGN(MessageWithBloomfilter);
};

}  // namespace gossip

}  // namespace top
