// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_prune.h"

NS_BEG2(top, sync)

void xsync_prune_t::add(const std::string address) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto prune_account = m_min_height_accounts.find(address);
    if (prune_account != m_min_height_accounts.end()) {
        return;
    }

    m_min_height_accounts[address] = 0;

    std::map<enum_height_type, uint64_t> prune;
    for (int i = enum_height_type::start; i < enum_height_type::end; i++) {
        prune[(enum_height_type)i] = 0;
    }

    m_prune_accounts[address] = prune;
}

void xsync_prune_t::del(const std::string address) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto prune_account = m_min_height_accounts.find(address);
    if (prune_account == m_min_height_accounts.end()) {
        return;
    }

    m_min_height_accounts.erase(address);
    m_prune_accounts[address].clear();
    m_prune_accounts.erase(address);
}

bool xsync_prune_t::update(const std::string address, const enum_height_type height_type, const uint64_t height) {
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_min_height_accounts.find(address) == m_min_height_accounts.end()) {
        return false;
    }

    if (m_prune_accounts[address][height_type] >= height) {
        return true;
    }

    m_prune_accounts[address][height_type] = height;

    auto prune_account = m_prune_accounts[address];
    uint64_t min = (uint64_t)-1;
    for (int i = enum_height_type::start; i < enum_height_type::end; i++) {
        if (min > prune_account[(enum_height_type)i]) {
            min = prune_account[(enum_height_type)i];
        }
    }

    if (m_min_height_accounts[address] < min) {
        m_min_height_accounts[address] = min;
    }
    return true;
}

bool xsync_prune_t::get_height(const std::string address, uint64_t &min_height) {
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_min_height_accounts.find(address) == m_min_height_accounts.end()) {
        return false;
    }

    min_height = m_min_height_accounts[address];
    return true;
}
NS_END2