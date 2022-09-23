// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt_db.h"

#include "xbasic/xhex.h"
#include "xstate_mpt/xerror.h"
#include "xvledger/xvdbkey.h"

namespace top {
namespace state_mpt {

xtop_state_mpt_db::xtop_state_mpt_db(base::xvdbstore_t * db, std::string table) : m_db(db), m_table(table) {
    xassert(db != nullptr);
}

void xtop_state_mpt_db::Put(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto db_key = base::xvdbkey_t::create_prunable_mpt_key(base::xvaccount_t{m_table}, {key.begin(), key.end()});
    if (m_db->set_value(db_key, {value.begin(), value.end()}) == false) {
        xwarn("xtop_state_mpt_db::Put key: %s, value: %s, error", to_hex(key).c_str(), to_hex(value).c_str());
        ec = error::xerrc_t::state_mpt_db_set_error;
        return;
    }
    xdbg("xtop_state_mpt_db::Put key: %s, value: %s", to_hex(key).c_str(), to_hex(value).c_str());
    return;
}

void xtop_state_mpt_db::Delete(xbytes_t const & key, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto db_key = base::xvdbkey_t::create_prunable_mpt_key(base::xvaccount_t{m_table}, {key.begin(), key.end()});
    if (m_db->delete_value(db_key) == false) {
        xwarn("xtop_state_mpt_db::Delete key: %s, error", top::to_hex(key).c_str());
        ec = error::xerrc_t::state_mpt_db_delete_error;
        return;
    }
    xdbg("xtop_state_mpt_db::Delete key: %s", top::to_hex(key).c_str());
    return;
}

bool xtop_state_mpt_db::Has(xbytes_t const & key, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto db_key = base::xvdbkey_t::create_prunable_mpt_key(base::xvaccount_t{m_table}, {key.begin(), key.end()});
    return (m_db->get_value(db_key)) != std::string();
}

xbytes_t xtop_state_mpt_db::Get(xbytes_t const & key, std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto db_key = base::xvdbkey_t::create_prunable_mpt_key(base::xvaccount_t{m_table}, {key.begin(), key.end()});
    auto value = m_db->get_value(db_key);
    if (value == std::string()) {
        xwarn("xtop_state_mpt_db::Get key: %s, not found", top::to_hex(key).c_str());
        ec = error::xerrc_t::state_mpt_db_not_found;
        return {};
    }
    xdbg("xtop_state_mpt_db::Get key: %s, value: %s", top::to_hex(key).c_str(), top::to_hex(value).c_str());
    return {value.begin(), value.end()};
}

}  // namespace state_mpt
}  // namespace top