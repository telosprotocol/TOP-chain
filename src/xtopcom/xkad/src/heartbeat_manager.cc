// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkad/routing_table/heartbeat_manager.h"
#include "xpbase/base/top_log.h"

namespace top {
namespace kadmlia {

HeartbeatManagerIntf* HeartbeatManagerIntf::Instance() {
    static HeartbeatManager ins;
    return &ins;
}

void HeartbeatManagerIntf::OnHeartbeatCallback(const std::string& ip, uint16_t port) {
    HeartbeatManagerIntf::Instance()->OnHeartbeatFailed(ip, port);
}

// ----------------------------------------------------------------
void HeartbeatManager::Register(const std::string& name, OfflineCallback cb) {
    std::unique_lock<std::mutex> lock(mutex_);
    vec_cb_.push_back(cb);
    TOP_INFO("[ht_cb] register %s", name.c_str());
}

void HeartbeatManager::OnHeartbeatFailed(const std::string& ip, uint16_t port) {
    TOP_INFO("[ht_cb] %s:%d heartbeat failed", ip.c_str(), (int)port);
    std::unique_lock<std::mutex> lock(mutex_);
    failed_eps_.push_back(std::make_pair(ip, port));
}

HeartbeatManager::HeartbeatManager() {
    const long intv = 3 * 1000 * 1000;
    timer_.Start(intv, intv, std::bind(&HeartbeatManager::TimerProc, this));
}

void HeartbeatManager::TimerProc() {
    decltype(vec_cb_) vec_cb;
    decltype(failed_eps_) failed_eps;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        vec_cb = vec_cb_;
        failed_eps.swap(failed_eps_);
    }

    for (auto& kv : failed_eps) {
        for (auto& cb : vec_cb) {
            cb(kv.first, kv.second);
        }
    }
}

}  // namespace kadmlia
}  // namespace top
