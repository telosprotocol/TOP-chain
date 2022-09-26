// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt_cache.h"

#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address.h"

namespace top {
namespace state_mpt {

#define TABLE_CACHE_SIZE 1024U

xtop_state_mpt_cache * xtop_state_mpt_cache::instance() {
    static xtop_state_mpt_cache * instance = nullptr;
    if (instance) {
        return instance;
    }
    instance = new xtop_state_mpt_cache();
    return instance;
}

std::shared_ptr<base::xlru_cache<std::string, std::string>> xtop_state_mpt_cache::get_lru(const std::string & table) {
    auto it = m_cache.find(table);
    if (it == m_cache.end()) {
        auto lru_ptr = std::make_shared<base::xlru_cache<std::string, std::string>>(TABLE_CACHE_SIZE);
        auto pair = m_cache.insert(std::make_pair(table, lru_ptr));
        it = pair.first;
    }
    return it->second;
}

std::string xtop_state_mpt_cache::get(std::shared_ptr<base::xlru_cache<std::string, std::string>> lru, const xhash256_t & root, const std::string & account) {
    std::string v;
    lru->get({account + "@" + root.as_hex_str().substr(0, 8)}, v);
    return v;
}

void xtop_state_mpt_cache::set(std::shared_ptr<base::xlru_cache<std::string, std::string>> lru, const xhash256_t & root, common::xaccount_address_t const & account, const std::string & value) {
    lru->put({account.value() + "@" + root.as_hex_str().substr(0, 8)}, value);
}

void xtop_state_mpt_cache::set(std::shared_ptr<base::xlru_cache<std::string, std::string>> lru, const xhash256_t & root, const std::map<common::xaccount_address_t, xbytes_t> & batch) {
    for (auto value : batch) {
        lru->put({value.first.value() + "@" + root.as_hex_str().substr(0, 8)}, {value.second.begin(), value.second.end()});
    }
}

std::string xtop_state_mpt_cache::get(const std::string & table, const xhash256_t & root, const std::string & account) {
    auto it = m_cache.find(table);
    if (it == m_cache.end()) {
        return {};
    }
    std::string v;
    m_cache[table]->get({account + "@" + root.as_hex_str().substr(0, 8)}, v);
    return v;
}

void xtop_state_mpt_cache::set(const std::string & table, const xhash256_t & root, const std::string & account, const std::string & value) {
    auto it = m_cache.find(table);
    if (it == m_cache.end()) {
        auto lru_ptr = std::make_shared<base::xlru_cache<std::string, std::string>>(TABLE_CACHE_SIZE);
        auto pair = m_cache.insert(std::make_pair(table, lru_ptr));
        it = pair.first;
    }
    it->second->put({account + "@" + root.as_hex_str().substr(0, 8)}, value);
}

void xtop_state_mpt_cache::set(const std::string & table, const xhash256_t & root, const std::map<std::string, xbytes_t> & batch) {
    auto it = m_cache.find(table);
    if (it == m_cache.end()) {
        auto lru_ptr = std::make_shared<base::xlru_cache<std::string, std::string>>(TABLE_CACHE_SIZE);
        auto pair = m_cache.insert(std::make_pair(table, lru_ptr));
        it = pair.first;
    }
    for (auto value : batch) {
        it->second->put({value.first + "@" + root.as_hex_str().substr(0, 8)}, {value.second.begin(), value.second.end()});
    }
}

}  // namespace state_mpt
}  // namespace top
