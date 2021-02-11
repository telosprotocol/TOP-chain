#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include "xbasic/xns_macro.h"
#include "xdata/xdata_common.h"

NS_BEG2(top, sync)

class xaccount_sync_status_t {
public:
    xaccount_sync_status_t() = default;

    std::string address;
    bool is_complete{false};
    uint64_t max_height{0};
    uint64_t continuous_height{0};
    uint32_t sub_accounts{0};

    xaccount_sync_status_t(const xaccount_sync_status_t &other) {
        address = other.address;
        is_complete = other.is_complete;
        max_height = other.max_height;
        continuous_height = other.continuous_height;
        sub_accounts = other.sub_accounts;
    }
};

class xsync_status_overview_t {
public:
    uint64_t syncing_account{0};
    uint64_t succeed{0};
    uint64_t failed{0};
    uint64_t send_bytes{0};
    uint64_t recv_bytes{0};
};

class xsync_status_t {
public:
    void update_accounts(std::unordered_map<std::string, xaccount_sync_status_t> &accounts);
    std::unordered_map<std::string, xaccount_sync_status_t> get_accounts();
    xsync_status_overview_t get_overview();

public:
    std::atomic<uint64_t> syncing_account{0};
    std::atomic<uint64_t> succeed{0};
    std::atomic<uint64_t> failed{0};
    std::atomic<uint64_t> send_bytes{0};
    std::atomic<uint64_t> recv_bytes{0};

private:
    std::mutex m_lock;
    std::unordered_map<std::string, xaccount_sync_status_t> m_accounts;
};

NS_END2