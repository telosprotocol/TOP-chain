#include "xsync/xsync_status.h"

NS_BEG2(top, sync)

void xsync_status_t::update_accounts(std::unordered_map<std::string, xaccount_sync_status_t> &accounts) {
    std::lock_guard<std::mutex> lock(m_lock);
    m_accounts.clear();
    m_accounts = accounts;
}

std::unordered_map<std::string, xaccount_sync_status_t> xsync_status_t::get_accounts() {
    std::lock_guard<std::mutex> lock(m_lock);
    std::unordered_map<std::string, xaccount_sync_status_t> accounts;
    for (auto &it: m_accounts) {
        xaccount_sync_status_t status = it.second;
        accounts[it.first] = status;
    }

    return accounts;
}

xsync_status_overview_t xsync_status_t::get_overview() {
    xsync_status_overview_t overview;
    overview.syncing_account = syncing_account;
    overview.succeed = succeed;
    overview.failed = failed;
    overview.send_bytes = send_bytes;
    overview.recv_bytes = recv_bytes;

    return overview;
}

NS_END2