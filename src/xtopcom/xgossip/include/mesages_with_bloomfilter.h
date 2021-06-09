// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xpbase/base/top_timer.h"
#include "xpbase/base/uint64_bloomfilter.h"

#include <memory>
#include <queue>
#include <unordered_map>
// todo charles might delete ServiceType in MessageKey here?[done]
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xtransport/proto/transport.pb.h"

namespace top {

namespace gossip {

struct MessageKey {
public:
    MessageKey() {
    }
    MessageKey(uint32_t hash) : msg_hash(hash) {
    }
    ~MessageKey() {
    }

    bool operator==(const MessageKey & key) const {
        return msg_hash == key.msg_hash;
    }

    bool operator<(const MessageKey & key) const {
        if (msg_hash < key.msg_hash) {
            return true;
        } else if (msg_hash == key.msg_hash) {
            return true;
        }
        return false;
    }

    uint32_t msg_hash;
};

struct MessageHash {
    size_t operator()(const MessageKey & key) const {
        return std::hash<uint64_t>()(key.msg_hash);
    }
};

class MessageWithBloomfilter {
public:
    static MessageWithBloomfilter * Instance();
    std::shared_ptr<base::Uint64BloomFilter> GetMessageBloomfilter(transport::protobuf::RoutingMessage & message);
    bool StopGossip(const MessageKey &, uint32_t);

private:
    MessageWithBloomfilter() {
    }
    ~MessageWithBloomfilter() {
    }

    std::shared_ptr<base::Uint64BloomFilter> MergeBloomfilter(const uint32_t &,
                                                              std::shared_ptr<base::Uint64BloomFilter> & bloomfilter,
                                                              const uint32_t & stop_times,
                                                              bool & stop_gossip);

    // static const uint32_t kMaxMessageQueueSize = 1048576u;
    static const uint32_t kMaxMessageQueueSize = 508576u;

    std::unordered_map<MessageKey, uint8_t, MessageHash> messsage_bloomfilter_map_;
    std::mutex messsage_bloomfilter_map_mutex_;

    DISALLOW_COPY_AND_ASSIGN(MessageWithBloomfilter);
};

}  // namespace gossip

}  // namespace top
