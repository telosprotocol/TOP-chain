// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt.h"

#include "xdata/xtable_bstate.h"
#include "xmetrics/xmetrics.h"
#include "xstate_mpt/xerror.h"

namespace top {
namespace state_mpt {

std::shared_ptr<xtop_state_mpt> xtop_state_mpt::create(xhash256_t root, base::xvdbstore_t * db, std::string table, std::error_code & ec) {
    xtop_state_mpt mpt;
    mpt.init(root, db, table, ec);
    if (ec) {
        xwarn("xtop_state_mpt::create init error, %s %s", ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    return std::make_shared<xtop_state_mpt>(mpt);
}

base::xaccount_index_t xtop_state_mpt::get_account_index(const std::string & account, std::error_code & ec) {
    auto index_str = get_account_index_str(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::get_account_index_str error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    if (index_str.empty()) {
        return {};
    }
    base::xaccount_index_t index;
    index.serialize_from(index_str);
    return index;
}

void xtop_state_mpt::set_account_index(const std::string & account, const base::xaccount_index_t & index, std::error_code & ec) {
    std::string index_str;
    index.serialize_to(index_str);
    return set_account_index(account, index_str, ec);
}

void xtop_state_mpt::set_account_index(const std::string & account, const std::string & index_str, std::error_code & ec) {
    auto prev_index_str = get_account_index_str(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::set_account_index get_account_index %s error, %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
        return;
    }
    m_journal.append({account, prev_index_str});
    set_account_index_str(account, index_str);
    return;
}

std::string xtop_state_mpt::get_account_index_str(const std::string & account, std::error_code & ec) {
    if (m_indexes.count(account)) {
        return {m_indexes[account].begin(), m_indexes[account].end()};
    }
    xbytes_t index_bytes;
    {
        XMETRICS_TIME_RECORD("state_mpt_load_db_index");
        index_bytes = m_trie->TryGet({account.begin(), account.end()}, ec);
    }
    if (ec) {
        xwarn("xtop_state_mpt::get_account_index_str TryGet %s error, %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
        return {};
    }
    if (index_bytes.empty()) {
        xwarn("xtop_state_mpt::get_account_index_str TryGet %s empty", account.c_str());
        return {};
    }
    set_account_index_str(account, {index_bytes.begin(), index_bytes.end()});
    return {index_bytes.begin(), index_bytes.end()};
}

void xtop_state_mpt::set_account_index_str(const std::string & account, const std::string & index_str) {
    m_indexes[account] = {index_str.begin(), index_str.end()};
}

xhash256_t xtop_state_mpt::get_root_hash(std::error_code & ec) {
    finalize();
    for (auto acc : m_pending_indexes) {
        auto index_str = m_indexes[acc];
        m_trie->TryUpdate({acc.begin(), acc.end()}, {index_str.begin(), index_str.end()}, ec);
        if (ec) {
            xwarn("xtop_state_mpt::get_root_hash TryUpdate %s error, %s %s", acc.c_str(), ec.category().name(), ec.message().c_str());
            return {};
        }
    }
    if (m_pending_indexes.size() > 0) {
        m_pending_indexes.clear();
    }
    return m_trie->Hash();
}

std::shared_ptr<evm_common::trie::xtrie_db_t> xtop_state_mpt::get_database() const {
    return m_db;
}

void xtop_state_mpt::finalize() {
    for (auto pair : m_journal.dirties) {
        auto acc = pair.first;
        if (!m_indexes.count(acc)) {
            continue;
        }
        m_pending_indexes.insert(acc);
    }
    clear_journal();
}

void xtop_state_mpt::clear_journal() {
    if (m_journal.index_changes.size() > 0) {
        m_journal.index_changes.clear();
        m_journal.dirties.clear();
    }
}

xhash256_t xtop_state_mpt::commit(std::error_code & ec) {
    get_root_hash(ec);
    if (ec) {
        xwarn("xtop_state_mpt::commit get_root_hash error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    std::pair<xhash256_t, int32_t> res;
    {
        XMETRICS_TIME_RECORD("state_mpt_trie_commit");
        res = m_trie->Commit(ec);
    }
    if (ec) {
        xwarn("xtop_state_mpt::commit trie commit error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    // TODO: should call outside with roles of node
    m_db->Commit(res.first, nullptr, ec);
    if (ec) {
        xwarn("xtop_state_mpt::commit db commit error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    return res.first;
}

void xtop_state_mpt::init(xhash256_t root, base::xvdbstore_t * db, std::string table, std::error_code & ec) {
    auto mpt_db = std::make_shared<xstate_mpt_db_t>(db, table);
    m_db = evm_common::trie::xtrie_db_t::NewDatabase(mpt_db);
    m_trie = evm_common::trie::xtrie_t::New(root, m_db, ec);
    if (ec) {
        xwarn("xtop_state_mpt::init generate trie with %s failed: %s, %s", root.as_hex_str().c_str(), ec.category().name(), ec.message().c_str());
        return;
    }
    m_original_root = root;
    return;
}

}  // namespace state_mpt
}  // namespace top