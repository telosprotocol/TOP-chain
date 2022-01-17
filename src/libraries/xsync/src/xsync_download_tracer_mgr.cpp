#include "xbase/xutl.h"
#include "xsync/xsync_download_tracer_mgr.h"
#include <mutex>
NS_BEG2(top, sync)

xsync_download_tracer::xsync_download_tracer(){

}

xsync_download_tracer::xsync_download_tracer(const std::pair<uint64_t, uint64_t> expect_height_interval, 
    const std::map<std::string, std::string> context, const vnetwork::xvnode_address_t& src, const vnetwork::xvnode_address_t& dst) {
    m_expect_height_interval = expect_height_interval;
    m_context = context;
    m_src_addr = src;
    m_dst_addr = dst;
}

void xsync_download_tracer::set_trace_height(const uint64_t trace_height) {
    m_trace_height = trace_height;
}

const uint64_t xsync_download_tracer::trace_height() {
    return m_trace_height;
}

const std::pair<uint64_t, uint64_t> xsync_download_tracer::height_interval(){
    return m_expect_height_interval;
}

const std::map<std::string, std::string> xsync_download_tracer::context(){
    return m_context;
}

void xsync_download_tracer::set_prior(const uint32_t prior) {
    m_prior = prior;
}

const uint32_t xsync_download_tracer::prior() {
    return m_prior;
}

bool xsync_download_tracer_mgr::apply(std::string account, std::pair<uint64_t, uint64_t> expect_height_interval, 
    const std::map<std::string, std::string> context, const vnetwork::xvnode_address_t& src, const vnetwork::xvnode_address_t& dst) {
    uint64_t now = xtime_utl::gmttime_ms();
    std::lock_guard<std::mutex> lck(m_lock);

    expire(now);
    if (m_account_elapses.size() >= m_max_capacity) {
        return false;
    }

    if (is_exist(account)) {
        return false;
    }

    insert(account, expect_height_interval, context, now, src, dst);
    return true;
}

const bool xsync_download_tracer_mgr::exist(std::string account){
    std::lock_guard<std::mutex> lck(m_lock);
    return is_exist(account);
}

bool xsync_download_tracer_mgr::refresh(std::string account, uint64_t downloaded_height) {
    uint64_t now = xtime_utl::gmttime_ms();

    std::lock_guard<std::mutex> lck(m_lock);
    expire(now);
    if (!is_exist(account)){
        return false;
    }
    xsync_download_tracer tracer = m_tracers.find(account)->second;

    if (tracer.trace_height() > downloaded_height) {
        return false;
    }

    if (tracer.height_interval().second < downloaded_height) {
        return false;
    }

    update(account, tracer, downloaded_height, now);
    return true;
}

bool xsync_download_tracer_mgr::refresh(std::string account) {
    uint64_t now = xtime_utl::gmttime_ms();

    std::lock_guard<std::mutex> lck(m_lock);
    expire(now);

    if (!is_exist(account)){
        return false;
    }
    xsync_download_tracer tracer = m_tracers.find(account)->second;

    update(account, tracer, tracer.trace_height(), now);
    return true;
}

bool xsync_download_tracer_mgr::get(const std::string account, xsync_download_tracer &tracer) {
    std::lock_guard<std::mutex> lck(m_lock);
    auto it = m_tracers.find(account);
    if (it != m_tracers.end()) {
        tracer = it->second;
        return true;
    }

    return false;
}

void xsync_download_tracer_mgr::expire() {
    uint64_t now = xtime_utl::gmttime_ms();
    std::lock_guard<std::mutex> lck(m_lock);
    expire(now);
}

void xsync_download_tracer_mgr::expire(std::string account) {
    std::lock_guard<std::mutex> lck(m_lock);
    for (auto it = m_elapses.begin(); it != m_elapses.end(); it++){
        if (it->second == account) {
            m_elapses.erase(it);
            break;
        }
    }

    m_account_elapses.erase(account);
    m_tracers.erase(account);
}

// xsync_download_tracer xsync_download_tracer_mgr::preempt(std::string account, std::pair<uint64_t, uint64_t> expect_height_interval, 
//     uint32_t prior, const std::map<std::string, std::string> context) {
    
// }

const bool xsync_download_tracer_mgr::is_exist(std::string account){
    if (m_tracers.find(account) != m_tracers.end()) {
        return true;
    }

    return false;
}

void xsync_download_tracer_mgr::expire(uint64_t time) {
    for (auto it = m_elapses.rbegin(); it != m_elapses.rend();){
        if (time - it->first >= m_tolerated_expire_time) {
            m_account_elapses.erase(it->second);
            m_tracers.erase(it->second);
            m_elapses.erase(std::next(it).base());
            continue;
        }
        break;
    }
}

void xsync_download_tracer_mgr::insert(xchain_account_t account, std::pair<uint64_t, uint64_t> height_interval, 
    const std::map<std::string, std::string> context, uint64_t now, const vnetwork::xvnode_address_t& src, const vnetwork::xvnode_address_t& dst) {
    xsync_download_tracer tracer(height_interval, context, src, dst);
    m_tracers.insert(std::make_pair(account, tracer));
    m_account_elapses.insert(std::make_pair(account, now));
    m_elapses.insert(std::make_pair(now, account));
}

void xsync_download_tracer_mgr::update(xchain_account_t account, xsync_download_tracer tracer, uint64_t trace_height, uint64_t now) {
    tracer.set_trace_height(trace_height);
    m_tracers[account] = tracer;
    m_account_elapses[account] = now;
    for (auto it = m_elapses.begin(); it != m_elapses.end();it++){
        if (it->second == account) {
            m_elapses.erase(it);
            break;
        }
    }
    m_elapses.insert(make_pair(now, account));
}

NS_END2