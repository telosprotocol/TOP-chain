// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xbase/xhash.h"
#include "xgossip/include/gossip_utils.h"
#include "xgossip/include/mesages_with_bloomfilter.h"
#include "xpbase/base/top_log.h"

namespace top {

namespace gossip {

MessageWithBloomfilter * MessageWithBloomfilter::Instance() {
    static MessageWithBloomfilter ins;
    return &ins;
}

std::shared_ptr<base::Uint64BloomFilter> MessageWithBloomfilter::GetMessageBloomfilter(transport::protobuf::RoutingMessage & message) {
    std::vector<uint64_t> new_bloomfilter_vec;
    new_bloomfilter_vec.reserve(message.bloomfilter_size());
    for (auto i = 0; i < message.bloomfilter_size(); ++i) {
        new_bloomfilter_vec.push_back(message.bloomfilter(i));
    }

    std::shared_ptr<base::Uint64BloomFilter> new_bloomfilter;
    if (new_bloomfilter_vec.empty()) {
        // readonly, mark static can improve performance
        static std::vector<uint64_t> construct_vec(gossip::kGossipBloomfilterSize / 64, 0ull);
        new_bloomfilter = std::make_shared<base::Uint64BloomFilter>(construct_vec, gossip::kGossipBloomfilterHashNum);
    } else {
        new_bloomfilter = std::make_shared<base::Uint64BloomFilter>(new_bloomfilter_vec, gossip::kGossipBloomfilterHashNum);
    }
    // (Charlie): avoid evil
    // MergeBloomfilter(hash32, new_bloomfilter, message.gossip().stop_times(), stop_gossip);
    return new_bloomfilter;
}

bool MessageWithBloomfilter::StopGossip(const MessageKey & msg_key, uint32_t stop_times) {
    if (stop_times <= 0) {
        stop_times = kGossipSendoutMaxTimes;
    }
    std::unique_lock<std::mutex> lock(messsage_bloomfilter_map_mutex_);
    auto iter = messsage_bloomfilter_map_.find(msg_key);
    if (iter != messsage_bloomfilter_map_.end()) {
        TOP_DEBUG("msg_hash:%u stop_times:%d", msg_key.msg_hash, iter->second);
        if (iter->second >= stop_times) {
            return true;
        }
        ++(iter->second);
    } else {
        messsage_bloomfilter_map_[msg_key] = 1;
    }

    // (Charlie): avoid memory crash
    if (messsage_bloomfilter_map_.size() >= kMaxMessageQueueSize) {
        messsage_bloomfilter_map_.clear();
    }
    return false;
}

void MessageWithBloomfilter::UpdateHandle(MessageKey const & message_key, uint64_t & sit1, uint64_t & sit2) {
    std::pair<uint64_t, uint64_t> exist_value(0, 0);
    message_dispatch_map_.get(message_key, exist_value);
    xdbg("MessageWithBloomfilter::UpdateHandle before update %llu %llu", exist_value.first, exist_value.second);

    sit1 |= exist_value.first;
    sit2 |= exist_value.second;

    message_dispatch_map_.put(message_key, std::make_pair(exist_value.first | (~sit1), exist_value.second | (~sit2)));

    message_dispatch_map_.get(message_key, exist_value);
    xdbg("MessageWithBloomfilter::UpdateHandle after update %llu %llu", exist_value.first, exist_value.second);
}

}  // namespace gossip

}  // namespace top
