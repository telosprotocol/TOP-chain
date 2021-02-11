// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <array>
#include <deque>

#include "xtransport/proto/transport.pb.h"

namespace top {

namespace base {
class TimerRepeated;
}

namespace transport {

static const uint64_t kDumpPeriod = 20ll * 1000ll * 1000ll; // 20 seconds

#ifdef ENABLE_XSECURITY
static const uint32_t kDdosBandRateLatestNum = 5;
// TODO(smaug) for simplicity, set the Threshold value of recv bandwidth 80Mbps (10MBps)
static const uint32_t kDdosBandRateThreshold = 80 * 1000 * 1000 / 8;
static const uint32_t kDdosDenyExpiredMaxTime = 10 * 60 * 1000; // 10min

typedef struct DdosBandwidthInfo {
    std::atomic<uint32_t> total_recv_band {0};
    std::atomic<uint32_t> last_total_recv_band {0};
    std::deque<uint32_t> latest_band_rate {0}; // keep latest 3 or more bandwidth_rate
} DdosBandwidthInfo;
typedef std::shared_ptr<DdosBandwidthInfo>  DdosBandwidthInfoPtr;
#endif

#ifdef ENABLE_METRICS
typedef struct TransportInfo {
public:
    std::atomic<uint32_t> send_packet {0};
    std::atomic<uint32_t> recv_packet {0};
    std::atomic<uint32_t> send_band {0};
    std::atomic<uint32_t> recv_band {0};
} TransportInfo;

static const int32_t MsgTypeMaxSize = 4096;
using ArrayInfo = std::array<TransportInfo, MsgTypeMaxSize>;
#endif


class TransportFilter {
public:
    static TransportFilter* Instance();
    bool Init();

public:
#ifdef ENABLE_METRICS
    // just for recv/send bandwidth collection
    bool AddTrafficData(bool send, uint32_t type, uint32_t size);
#endif

#ifdef ENABLE_XSECURITY
    // monitor recv-bandwidth for ddos deny
    bool AddTrafficData(uint32_t size, const std::string& ip);
    bool BlackIpCheck(const std::string& ip);
#endif

private:
    void Dump();

private:
    TransportFilter();
    ~TransportFilter();

private:
    bool inited_{false};
    std::shared_ptr<base::TimerRepeated> dump_timer_{nullptr};

#ifdef ENABLE_METRICS
    ArrayInfo aryinfo_;
#endif

#ifdef ENABLE_XSECURITY
    std::mutex ddos_bandwidth_map_mutex_;
    std::map<std::string, DdosBandwidthInfoPtr> ddos_bandwidth_map_;

    std::mutex ddos_blacklist_mutex_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> ddos_blacklist_;
#endif
};

}

}
