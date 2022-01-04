// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_prune.h"

NS_BEG2(top, sync)

void xsync_prune_t::add(const std::string address, const std::set<enum_height_type> types) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto prune_account = m_prune_accounts.find(address);
    if (prune_account != m_prune_accounts.end()) {
        for (auto type:types) {
            if (prune_account->second.find((enum_height_type)type) == prune_account->second.end()) {
                prune_account->second[(enum_height_type)type] = 0;
                m_min_height_accounts[address] = 0;
            }
        }
    } else {
        m_min_height_accounts[address] = 0;
        std::map<enum_height_type, uint64_t> prune;
        for (auto type:types) {
            prune[(enum_height_type)type] = 0;
        }
        m_prune_accounts[address] = prune;
    }

    return;
}

void xsync_prune_t::del(const std::string address, const std::set<enum_height_type> types) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto prune_account = m_prune_accounts.find(address);
    if (prune_account == m_prune_accounts.end()) {
        return;
    }

    for (auto type:types) {
        if (prune_account->second.find((enum_height_type)type) != prune_account->second.end()) {
            prune_account->second.erase((enum_height_type)type);
        }
    }

    m_min_height_accounts[address] = 0;

    if (prune_account->second.empty()){
        m_min_height_accounts.erase(address);
        m_prune_accounts[address].clear();
        m_prune_accounts.erase(address);
    }
    return;
}

bool xsync_prune_t::empty(const std::string address) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto prune_account = m_prune_accounts.find(address);
    if (prune_account == m_prune_accounts.end()) {
        return true;
    }

    return false;
}

bool xsync_prune_t::update(const std::string address, const enum_height_type height_type, const uint64_t height) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto prune_account = m_prune_accounts.find(address);
    if (prune_account == m_prune_accounts.end()) {
        return false;
    }

    if (prune_account->second.find(height_type) == prune_account->second.end()) {
        return true;
    }

    if (prune_account->second[height_type] >= height) {
        return true;
    }

    prune_account->second[height_type] = height;
    uint64_t min = (uint64_t)-1;
    for (auto type:prune_account->second){
       if (min > type.second) {
            min = type.second;
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