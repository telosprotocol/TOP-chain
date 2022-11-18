// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_kv_db.h"

#include "xbasic/xhex.h"
#include "xcommon/xnode_id.h"
#include "xevm_common/xerror/xerror.h"
#include "xmetrics/xmetrics.h"
#include "xvledger/xvdbkey.h"

namespace top {
namespace evm_common {
namespace trie {

xtop_kv_db::xtop_kv_db(base::xvdbstore_t * db, common::xaccount_address_t table) : m_db(db), m_table(table) {
    xassert(db != nullptr);
    m_node_key_prefix = base::xvdbkey_t::create_prunable_mpt_node_key_prefix(m_table.vaccount());
}

std::string xtop_kv_db::convert_key(gsl::span<xbyte_t const> const key) const {
    return base::xvdbkey_t::create_prunable_mpt_node_key(m_node_key_prefix, std::string{key.begin(), key.end()});
}

void xtop_kv_db::Put(gsl::span<xbyte_t const> const key, xbytes_t const & value, std::error_code & ec) {
    XMETRICS_COUNTER_INCREMENT("trie_put_nodes", 1);
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db->set_value(convert_key(key), {value.begin(), value.end()}) == false) {
        xwarn("xtop_kv_db::Put key: %s, value: %s, error", to_hex(key).c_str(), to_hex(value).c_str());
        ec = error::xenum_errc::trie_db_put_error;
        return;
    }
    xdbg("xtop_kv_db::Put key: %s, value: %s", to_hex(key).c_str(), to_hex(value).c_str());
    return;
}

void xtop_kv_db::PutBatch(std::map<xbytes_t, xbytes_t> const & batch, std::error_code & ec) {
    XMETRICS_COUNTER_INCREMENT("trie_put_nodes", batch.size());
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, std::string> convert_batch;
    for (auto b : batch) {
        convert_batch.emplace(std::make_pair(convert_key(b.first), std::string{b.second.begin(), b.second.end()}));
    }
    if (m_db->set_values(convert_batch) == false) {
        xwarn("xtop_kv_db::PutBatch error");
        ec = error::xenum_errc::trie_db_put_error;
        return;
    }
    return;
}

void xtop_kv_db::PutDirect(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
    XMETRICS_COUNTER_INCREMENT("trie_put_units", 1);
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db->set_value({key.begin(), key.end()}, {value.begin(), value.end()}) == false) {
        xwarn("xtop_kv_db::PutDirect key: %s, value: %s, error", to_hex(key).c_str(), to_hex(value).c_str());
        ec = error::xenum_errc::trie_db_put_error;
        return;
    }
    xdbg("xtop_kv_db::PutDirect key: %s, value: %s", to_hex(key).c_str(), to_hex(value).c_str());
    return;
}

void xtop_kv_db::PutDirectBatch(std::map<xbytes_t, xbytes_t> const & batch, std::error_code & ec) {
    XMETRICS_COUNTER_INCREMENT("trie_put_units", batch.size());
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, std::string> convert_batch;
    for (auto b : batch) {
        convert_batch.emplace(std::make_pair(std::string{b.first.begin(), b.first.end()}, std::string{b.second.begin(), b.second.end()}));
    }
    if (m_db->set_values(convert_batch) == false) {
        xwarn("xtop_kv_db::PutBatch error");
        ec = error::xenum_errc::trie_db_put_error;
        return;
    }
    return;
}

void xtop_kv_db::Delete(xbytes_t const & key, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db->delete_value(convert_key(key)) == false) {
        xwarn("xtop_kv_db::Delete key: %s, error", top::to_hex(key).c_str());
        ec = error::xerrc_t::trie_db_delete_error;
        return;
    }
    xdbg("xtop_kv_db::Delete key: %s", top::to_hex(key).c_str());
}

void xtop_kv_db::DeleteBatch(std::vector<xbytes_t> const & batch, std::error_code & ec) {
    assert(!ec);
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> convert_batch;
    for (auto const & b : batch) {
        convert_batch.emplace_back(convert_key(b));
    }

    if (m_db->delete_values(convert_batch) == false) {
        xwarn("xtop_kv_db::DeleteBatch error");
        ec = error::xerrc_t::trie_db_delete_error;
    }
}

void xtop_kv_db::DeleteDirect(xbytes_t const & key, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db->delete_value({key.begin(), key.end()}) == false) {
        xwarn("xtop_kv_db::DeleteDirect key: %s, error", top::to_hex(key).c_str());
        ec = error::xerrc_t::trie_db_delete_error;
        return;
    }
    xdbg("xtop_kv_db::DeleteDirect key: %s", top::to_hex(key).c_str());
    return;
}

void xtop_kv_db::DeleteDirectBatch(std::vector<xbytes_t> const & batch, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> convert_batch;
    for (auto const & b : batch) {
        convert_batch.emplace_back(std::string{b.begin(), b.end()});
    }
    if (m_db->delete_values(convert_batch) == false) {
        xwarn("xtop_kv_db::DeleteDirectBatch error");
        ec = error::xerrc_t::trie_db_delete_error;
        return;
    }
    return;
}

bool xtop_kv_db::Has(xbytes_t const & key, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_db->get_value(convert_key(key))) != std::string();
}

bool xtop_kv_db::HasDirect(xbytes_t const & key, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_db->get_value({key.begin(), key.end()})) != std::string();
}

xbytes_t xtop_kv_db::Get(xbytes_t const & key, std::error_code & ec) {
    XMETRICS_COUNTER_INCREMENT("trie_get_nodes", 1);
    std::lock_guard<std::mutex> lock(m_mutex);
    auto value = m_db->get_value(convert_key(key));
    if (value == std::string()) {
        xwarn("xtop_kv_db::Get key: %s, not found", top::to_hex(key).c_str());
        ec = error::xerrc_t::trie_db_not_found;
        return {};
    }
    xdbg("xtop_kv_db::Get key: %s, value: %s", top::to_hex(key).c_str(), top::to_hex(value).c_str());
    return {value.begin(), value.end()};
}

xbytes_t xtop_kv_db::GetDirect(xbytes_t const & key, std::error_code & ec) {
    XMETRICS_COUNTER_INCREMENT("trie_get_units", 1);
    std::lock_guard<std::mutex> lock(m_mutex);
    auto value = m_db->get_value({key.begin(), key.end()});
    if (value == std::string()) {
        xwarn("xtop_kv_db::GetDirect key: %s, not found", top::to_hex(key).c_str());
        ec = error::xerrc_t::trie_db_not_found;
        return {};
    }
    xdbg("xtop_kv_db::GetDirect key: %s, value: %s", top::to_hex(key).c_str(), top::to_hex(value).c_str());
    return {value.begin(), value.end()};
}

}  // namespace trie
}  // namespace evm_common
}  // namespace top