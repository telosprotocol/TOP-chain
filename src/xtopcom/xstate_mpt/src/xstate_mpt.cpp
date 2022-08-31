// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt.h"

#include "xdata/xtable_bstate.h"
#include "xstate_mpt/xerror.h"

namespace top {
namespace state_mpt {

std::shared_ptr<xtop_state_mpt> xtop_state_mpt::create(xhash256_t root, base::xvdbstore_t * db, std::error_code & ec) {
    xtop_state_mpt mpt;
    mpt.init(root, db, ec);
    if (ec) {
        xwarn("xtop_state_mpt::create init error, %s %s", ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    return std::make_shared<xtop_state_mpt>(mpt);
}

base::xaccount_index_t xtop_state_mpt::get_account_index(const std::string & account, std::error_code & ec) {
    auto index_str = m_trie->TryGet({account.begin(), account.end()}, ec);
    if (ec) {
        xwarn("xtop_state_mpt::get_account_index TryGet %s error, %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
        return {};
    }
    if (index_str.empty()) {
        xwarn("xtop_state_mpt::get_account_index TryGet %s empty", account.c_str());
        return {};
    }
    base::xaccount_index_t index;
    index.serialize_from({index_str.begin(), index_str.end()});
    return index;
}

void xtop_state_mpt::set_account_index(const std::string & account, const base::xaccount_index_t & index, std::error_code & ec) {
    std::string index_str;
    index.serialize_to(index_str);
    set_account_index(account, index_str, ec);
}

void xtop_state_mpt::set_account_index(const std::string & account, const std::string & index_str, std::error_code & ec) {
    m_trie->TryUpdate({account.begin(), account.end()}, {index_str.begin(), index_str.end()}, ec);
    if (ec) {
        xwarn("xtop_state_mpt::set_account_index TryUpdate %s error, %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
        return;
    }
}

xhash256_t xtop_state_mpt::get_root_hash() const {
    return m_trie->Hash();
}

xhash256_t xtop_state_mpt::commit(std::error_code & ec) {
    auto res = m_trie->Commit(ec);
    if (ec) {
        xwarn("xtop_state_mpt::commit trie commit error, %s %s", ec.category().name(), ec.message().c_str());
        return res.first;
    }
    // TODO: should call outside with roles of node
    m_db->Commit(res.first, nullptr, ec);
    if (ec) {
        xwarn("xtop_state_mpt::commit db commit error, %s %s", ec.category().name(), ec.message().c_str());
        return res.first;
    }
    return res.first;
}

void xtop_state_mpt::init(xhash256_t root, base::xvdbstore_t * db, std::error_code & ec) {
    auto mpt_db = std::make_shared<xstate_mpt_db_t>(db);
    m_db = evm_common::trie::xtrie_db_t::NewDatabase(mpt_db);
    m_trie = evm_common::trie::xtrie_t::New(root, m_db, ec);
    if (ec) {
        xwarn("xtop_state_mpt::init generate trie with %s failed: %s, %s", root.as_hex_str().c_str(), ec.category().name(), ec.message().c_str());
        return;
    }
    return;
}

}  // namespace state_mpt
}  // namespace top