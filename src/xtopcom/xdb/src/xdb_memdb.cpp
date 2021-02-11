// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <stdexcept>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "xdb/xdb_mem.h"

namespace top { namespace db {

bool xdb_mem_t::read(const std::string& key, std::string& value) const {
    std::lock_guard<std::mutex> lock(m_lock);
    auto iter = m_values.find(key);
    if (iter != m_values.end()) {
        value = iter->second;
        return true;
    }
    return false;
}

bool xdb_mem_t::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_lock);
    auto iter = m_values.find(key);
    if (iter != m_values.end()) {
        return true;
    }
    return false;
}

bool xdb_mem_t::write(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_lock);
    m_values[key] = value;
    return true;
}

bool xdb_mem_t::write(const std::string& key, const char* data, size_t size) {
    std::lock_guard<std::mutex> lock(m_lock);
    std::string value(data, size);
    m_values[key] = value;
    return true;
}

bool xdb_mem_t::write(const std::map<std::string, std::string>& batches) {
    std::lock_guard<std::mutex> lock(m_lock);
    for (const auto& entry : batches) {
        m_values[entry.first] = entry.second;
    }
    return true;
}

bool xdb_mem_t::erase(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_lock);
    m_values.erase(key);
    return true;
}

bool xdb_mem_t::erase(const std::vector<std::string>& keys) {
    std::lock_guard<std::mutex> lock(m_lock);
    for (const auto& key : keys) {
        m_values.erase(key);
    }
    return true;
}

bool xdb_mem_t::batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) {
    std::lock_guard<std::mutex> lock(m_lock);
    for (const auto& entry : objs) {
        m_values[entry.first] = entry.second;
    }
    for (const auto& key : delete_keys) {
        m_values.erase(key);
    }
    return true;
}

xdb_transaction_t* xdb_mem_t::begin_transaction() {
    return new xdb_memdb_transaction_t(this);
}

bool xdb_memdb_transaction_t::rollback() {
    // drop every thing
    m_read_values.clear();
    m_write_values.clear();
    m_erase_keys.clear();
    return true;
}

bool xdb_memdb_transaction_t::commit() {
    if (m_db == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_db->m_lock);
    {
        for (const auto& entry : m_read_values) {
            if (m_db->m_values[entry.first] != entry.second) {
                return false;
            }
        }
        for (const auto& entry : m_write_values) {
            m_db->m_values[entry.first] = entry.second;
        }

        for (const auto& key : m_erase_keys) {
            m_db->m_values.erase(key);
        }
    }
    return true;
}

bool xdb_memdb_transaction_t::read(const std::string& key, std::string& value) const {
    if (m_db == nullptr) {
        return false;
    }
    m_db->read(key, value);
    m_read_values[key] = value;

    return true;
}

bool xdb_memdb_transaction_t::write(const std::string& key, const std::string& value) {
    if (m_db == nullptr) {
        return false;
    }
    m_write_values[key] = value;
    return true;
}

bool xdb_memdb_transaction_t::write(const std::string& key, const char* data, size_t size) {
    if (m_db == nullptr) {
        return false;
    }
    m_write_values[key] = std::string(data, size);
    return true;
}

bool xdb_memdb_transaction_t::erase(const std::string& key) {
    if (m_db == nullptr) {
        return false;
    }
    m_erase_keys.insert(key);
    return true;
}

}  // namespace ledger
}  // namespace top
