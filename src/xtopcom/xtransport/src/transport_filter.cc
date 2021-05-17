// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtransport/udp_transport/transport_filter.h"

#include "xpbase/base/top_log.h"

namespace top {

namespace transport {

TransportFilter* TransportFilter::Instance() {
    static TransportFilter ins;
    return &ins;
}

bool TransportFilter::Init() {
    if (inited_) {
        return true;
    }
    #if defined(XENABLE_P2P_BENDWIDTH) || defined(ENABLE_XSECURITY)
    dump_timer_ = std::make_shared<base::TimerRepeated>(base::TimerManager::Instance(), "TransportFilter::Dump");
    dump_timer_->Start(
            500ll * 1000ll,
            kDumpPeriod,
            std::bind(&TransportFilter::Dump, this));
    #endif
    inited_ = true;
    TOP_INFO("TransportFilter::Init ok");
    return true;
}

TransportFilter::TransportFilter() {}

TransportFilter::~TransportFilter() {
//    dump_timer_->Join();
    dump_timer_ = nullptr;
}

void TransportFilter::Dump() {
    float time_step = static_cast<float>(kDumpPeriod / 1000.0 / 1000.0);
#ifdef XENABLE_P2P_BENDWIDTH
    static ArrayInfo last_aryinfo;
    for (uint32_t i = 0; i < aryinfo_.size(); ++i) {
        if (aryinfo_[i].send_packet == 0 && aryinfo_[i].recv_packet == 0) {
            continue;
        }

        uint32_t send_packet = aryinfo_[i].send_packet;
        uint32_t send_band    = aryinfo_[i].send_band;
        uint32_t recv_packet = aryinfo_[i].recv_packet;
        uint32_t recv_band   = aryinfo_[i].recv_band;

        auto send_packet_step  = (send_packet - last_aryinfo[i].send_packet)  / time_step;
        auto send_band_step    = (send_band - last_aryinfo[i].send_band) / time_step;
        auto recv_packet_step  = (recv_packet - last_aryinfo[i].recv_packet) / time_step;
        auto recv_band_step    = (recv_band - last_aryinfo[i].recv_band) / time_step;
        TOP_DEBUG("transportfilter: type:%d send_packet:%d recv_packet:%d send_band:%d recv_band:%d"
                " ##send_packet_step:%.2f recv_packet_step:%.2f send_band_step:%.2f recv_band_step:%.2f",
                i,
                send_packet,
                recv_packet,
                send_band,
                recv_band,
                send_packet_step,
                recv_packet_step,
                send_band_step,
                recv_band_step);

        last_aryinfo[i].send_packet = send_packet;
        last_aryinfo[i].recv_packet = recv_packet;
        last_aryinfo[i].send_band = send_band;
        last_aryinfo[i].recv_band = recv_band;
    } // end for
#endif


#ifdef ENABLE_XSECURITY
    std::vector<std::string> black_ip_vec;
    {
        std::unique_lock<std::mutex> lock(ddos_bandwidth_map_mutex_);
        for (auto it = ddos_bandwidth_map_.begin(); it != ddos_bandwidth_map_.end();) {
            auto dptr = it->second;
            if (dptr->last_total_recv_band == 0) {
                // this first dump time
                dptr->last_total_recv_band.store(dptr->total_recv_band.load());
                ++ it;
                continue;
            }
            auto recv_band_rate = static_cast<uint32_t>((dptr->total_recv_band - dptr->last_total_recv_band)  / time_step);
            dptr->last_total_recv_band.store(dptr->total_recv_band.load());

            if (recv_band_rate == 0) {
                // maybe the trafic between this ip and local_ip stopped, erase it to avoid memory growing
                it = ddos_bandwidth_map_.erase(it);
                continue;
            }
            
            if (recv_band_rate < kDdosBandRateThreshold) {
                while (dptr->latest_band_rate.size() > 0) {
                    dptr->latest_band_rate.pop_front();
                }
                ++ it;
                continue;
            }
            // recv_band_rate larger than kDdosBandRateThresholdt, than put it in deque
            dptr->latest_band_rate.push_back(recv_band_rate);
            if (dptr->latest_band_rate.size() < kDdosBandRateLatestNum) {
                ++ it;
                continue;
            }

            // more than kDdosBandRateLatestNum continuous value all larger than kDdosBandRateThreshold, should mark this ip as ddos-ip
            while (dptr->latest_band_rate.size() >= kDdosBandRateLatestNum) {
                dptr->latest_band_rate.pop_front();
            }
            ++ it;
            // put in blacklist 
            black_ip_vec.push_back(it->first);
        } // end for (auto it...
    }

    auto now = std::chrono::steady_clock::now();
    // add black_ip
    {
        std::unique_lock<std::mutex> block(ddos_blacklist_mutex_);
        for (const auto& item : black_ip_vec) {
            ddos_blacklist_[item] = now;
            TOP_INFO("add ip:%s to ddos black_list", item.c_str());
        }

        for (auto it = ddos_blacklist_.begin(); it != ddos_blacklist_.end();) {
            if (it->second == now) {
                continue;
            }
            if (it->second + std::chrono::milliseconds(kDdosDenyExpiredMaxTime) < now) {
                // expire time 10 min, after expire, remove ip from black_list
                TOP_INFO("remove ip:%s from ddos black_list", it->first.c_str());
                it = ddos_blacklist_.erase(it);
            }
        }

    }
#endif
}

#ifdef ENABLE_XSECURITY
bool TransportFilter::BlackIpCheck(const std::string& ip) {
    {
        std::unique_lock<std::mutex> block(ddos_blacklist_mutex_);
        auto ifind = ddos_blacklist_.find(ip);
        if (ifind == ddos_blacklist_.end()) {
            return false;
        }
    }
    TOP_INFO("ddos check black ip:%s true", ip.c_str());
    return true;
}

bool TransportFilter::AddTrafficData(uint32_t size, const std::string& ip) {
    {
        std::unique_lock<std::mutex> lock(ddos_bandwidth_map_mutex_);
        auto ifind = ddos_bandwidth_map_.find(ip);
        if (ifind == ddos_bandwidth_map_.end()) {
            auto dptr = std::make_shared<DdosBandwidthInfo>();
            dptr->total_recv_band += size;
            ddos_bandwidth_map_[ip] = dptr;
        } else {
            ifind->second->total_recv_band += size;
        }
    }
    TOP_DEBUG("add traffic data for ddos ip:%s", ip.c_str());
    return  true;
}
#endif

#ifdef ENABLE_METRICS
bool TransportFilter::AddTrafficData(bool send, uint32_t type, uint32_t size) {
    if (type >= MsgTypeMaxSize || type <= 0) {
        TOP_WARN("type:%d invalid, size:%d failed", type, size);
        return false;
    }

    if (send) {
        aryinfo_[type].send_packet += 1;
        aryinfo_[type].send_band += size;
    } else {
        aryinfo_[type].recv_packet += 1;
        aryinfo_[type].recv_band += size;
    }
    return true;
}
#endif



} // end namespace transport 

} // end namespace top
