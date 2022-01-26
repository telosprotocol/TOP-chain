#pragma once

#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include "xbase/xns_macro.h"
#include "xvnetwork/xaddress.h"
NS_BEG2(top, sync)

using xchain_account_t = std::string;
using base::xtime_utl;

class xsync_download_tracer {
    public:
        xsync_download_tracer();
        xsync_download_tracer(const std::pair<uint64_t, uint64_t> expect_height_interval, 
            const std::map<std::string, std::string> context,
            const vnetwork::xvnode_address_t& src,
            const vnetwork::xvnode_address_t& dst);
        void set_trace_height(const uint64_t trace_height);
        const uint64_t trace_height();
        const std::pair<uint64_t, uint64_t> height_interval();
        const std::map<std::string, std::string> context();
        void set_prior(const uint32_t prior);
        const uint32_t prior();
        vnetwork::xvnode_address_t get_src_addr() {return m_src_addr;}
        vnetwork::xvnode_address_t get_dst_addr() {return m_dst_addr;}
    private:
        std::pair<uint64_t, uint64_t> m_expect_height_interval;
        uint64_t m_trace_height{0}; // 0 is undetermined
        std::map<std::string, std::string> m_context;
        uint32_t m_prior;
        vnetwork::xvnode_address_t m_src_addr;
        vnetwork::xvnode_address_t m_dst_addr;
};

class xsync_download_tracer_mgr {
    public:
        xsync_download_tracer_mgr(){};
        bool apply(std::string account, std::pair<uint64_t, uint64_t> expect_height_interval, 
            const std::map<std::string, std::string> context, const vnetwork::xvnode_address_t& src, const vnetwork::xvnode_address_t& dst);
        const bool exist(std::string account);
        bool refresh(std::string account, uint64_t downloaded_height);
        bool refresh(std::string account);
        bool get(const std::string account, xsync_download_tracer &tracer);

        void expire();
        void expire(std::string account);
        // xsync_download_tracer preempt(std::string account, std::pair<uint64_t, uint64_t> expect_height_interval, 
        //     uint32_t prior, const std::map<std::string, std::string> context);

    private:
        void expire(uint64_t time);
        void insert(xchain_account_t account, std::pair<uint64_t, uint64_t> height_interval, 
            const std::map<std::string, std::string> context, uint64_t now, const vnetwork::xvnode_address_t& src,
            const vnetwork::xvnode_address_t& dst);
        void update(xchain_account_t account, xsync_download_tracer tracer, uint64_t trace_height, uint64_t now);
        const bool is_exist(std::string account);
        std::unordered_map<xchain_account_t, xsync_download_tracer> m_tracers;
        std::multimap<uint64_t, xchain_account_t> m_elapses;
        std::unordered_map<xchain_account_t, uint64_t> m_account_elapses;
        mutable std::mutex m_lock;
        const uint64_t m_tolerated_expire_time = 2000;// time unit is ms
        const uint32_t m_max_capacity = 100;
};

NS_END2