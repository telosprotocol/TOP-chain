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
#include "xbase/xlog.h"

namespace top { namespace db {

bool xdb_mem_t::read(const std::string& key, std::string& value) const {
    std::lock_guard<std::mutex> lock(m_lock);

    m_meta.m_read_count++;

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
    auto iter = m_values.find(key);
    if (iter != m_values.end()) {
        m_meta.m_db_key_size -= key.size();
        m_meta.m_db_value_size -= iter->second.size();   
        m_meta.m_key_count--;     
    }
    m_values[key] = value;
    m_meta.m_db_key_size += key.size();
    m_meta.m_db_value_size += value.size();
    m_meta.m_key_count++;

    m_meta.m_write_count++;

    xdbg("xdb_mem_t::write key=%s", key.c_str());
    return true;
}

bool xdb_mem_t::write(const std::string& key, const char* data, size_t size) {
    std::string value(data, size);
    return write(key, value);
}

bool xdb_mem_t::write(const std::map<std::string, std::string>& batches) {
    std::lock_guard<std::mutex> lock(m_lock);
    for (const auto& entry : batches) {
        auto & key = entry.first;
        auto & value = entry.second;

        auto iter = m_values.find(key);
        if (iter != m_values.end()) {
            m_meta.m_db_key_size -= key.size();
            m_meta.m_db_value_size -= iter->second.size();   
            m_meta.m_key_count--;
        }
        m_values[key] = value;
        m_meta.m_db_key_size += key.size();
        m_meta.m_db_value_size += value.size();
        m_meta.m_key_count++;

        m_meta.m_write_count++;
    }    
    return true;
}

bool xdb_mem_t::erase(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_lock);
    auto iter = m_values.find(key);
    if (iter != m_values.end()) {
        m_meta.m_db_key_size -= key.size();
        m_meta.m_db_value_size -= iter->second.size();
        m_meta.m_key_count--;

        m_values.erase(key);
    }

    m_meta.m_erase_count++;
    return true;
}

bool xdb_mem_t::erase(const std::vector<std::string>& keys) {
    std::lock_guard<std::mutex> lock(m_lock);
    for (const auto& key : keys) {
        auto iter = m_values.find(key);
        if (iter != m_values.end()) {
            m_meta.m_db_key_size -= key.size();
            m_meta.m_db_value_size -= iter->second.size();
            m_meta.m_key_count--;

            m_values.erase(key);  
        }
        m_meta.m_erase_count++;
    }
    return true;
}

bool xdb_mem_t::batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) {
    write(objs);
    erase(delete_keys);
    return true;
}

//prefix must start from first char of key
bool xdb_mem_t::read_range(const std::string& prefix, std::vector<std::string>& values)
{
    xassert(0);
    return false;
}
//note:begin_key and end_key must has same style(first char of key)
bool xdb_mem_t::delete_range(const std::string& begin_key,const std::string& end_key)
{
    xassert(0);
    return false;
}

//key must be readonly(never update after PUT),otherwise the behavior is undefined
bool xdb_mem_t::single_delete(const std::string& key)
{
    return erase(key);
}

//iterator each key of prefix.note: go throuh whole db if prefix is empty
bool xdb_mem_t::read_range(const std::string& prefix,xdb_iterator_callback callback,void * cookie)
{
    return false;
}

//compact whole DB if both begin_key and end_key are empty
//note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
bool xdb_mem_t::compact_range(const std::string & begin_key,const std::string & end_key)
{
    return false;
}

bool xdb_mem_t::get_estimate_num_keys(uint64_t & num) const
{
    return false;
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
