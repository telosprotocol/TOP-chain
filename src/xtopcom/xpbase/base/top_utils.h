// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                     \
    TypeName(const TypeName &);                                                \
    TypeName &operator=(const TypeName &)
#endif // !DISALLOW_COPY_AND_ASSIGN

namespace top {

namespace base {
class KadmliaKey;
}
extern std::shared_ptr<base::KadmliaKey> global_xid;
extern std::string global_node_id;
extern std::string global_node_signkey;
static const uint32_t kRoot = 0xFFFFF; // avoid confilict with xip

static const int kNodeIdSize = 33;
static const uint32_t kUdpPacketBufferSize = 8000U;

uint64_t GetCurrentTimeMsec();
uint64_t GetCurrentTimeMicSec();
void TrimString(std::string &in_str);
void SleepUs(uint64_t time_us);
void SleepMs(uint64_t time_ms);

std::string RandomString(size_t size);
std::mt19937 &random_number_generator();
std::mutex &random_number_generator_mutex();

int32_t RandomInt32();
uint32_t RandomUint32();
uint64_t RandomUint64();
uint16_t RandomUint16();

template <typename String> String GetRandomString(size_t size) {
    std::uniform_int_distribution<> distribution(0, 255);
    String random_string(size, 0);
    {
        std::lock_guard<std::mutex> lock(random_number_generator_mutex());
        std::generate(random_string.begin(), random_string.end(),
                      [&] { return distribution(random_number_generator()); });
    }
    return random_string;
}


std::string HexEncode(const std::string& str);
std::string HexDecode(const std::string& str);
std::string Base64Encode(const std::string& str);
std::string Base64Decode(const std::string& str);
std::string HexSubstr(const std::string& str);
std::string Base64Substr(const std::string& str);
std::string DecodePrivateString(const std::string & pri_key);

enum RoutingMessageType {
    kInvalidMessageType = 0,

    // kad message type
    // for root routing table:
    kRootMessage,
    kKadBootstrapJoinRequest,
    kKadBootstrapJoinResponse,
    kKadFindNodesRequest,
    kKadFindNodesResponse,
    kKadHandshake,

    // root broadcast block sync
    kGossipBlockSyncAsk = 20,
    kGossipBlockSyncAck = 21,
    kGossipBlockSyncRequest = 22,
    kGossipBlockSyncResponse = 23,

    // elect routing message type:
    kElectVhostRumorP2PMessage = 1159,
    kElectVhostRumorGossipMessage = 1160,

    // for test
    kTestMessageType,

};

enum VersionProtocol {
    kSerializeProtocolProtobuf = 1,
    kSerializeProtocolMsgpack = 2,
    kSerializeProtocolXbase = 3,

    kVersionV1ProtocolProtobuf = (1 << 12) | kSerializeProtocolProtobuf,
    kVersionV2ProtocolProtobuf = (2 << 12) | kSerializeProtocolProtobuf,
    kVersionV3ProtocolProtobuf = (3 << 12) | kSerializeProtocolProtobuf,
};

} // namespace top
