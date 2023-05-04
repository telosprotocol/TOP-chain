// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt.h"

#include "xdata/xtable_bstate.h"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xmetrics/xmetrics.h"
#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_mpt_database.h"
#include "xstate_mpt/xstate_mpt_store.h"

NS_BEG3(top, state_mpt, details)

std::string xaccount_info_t::encode() const {
    base::xautostream_t<1024> stream(base::xcontext_t::instance());
    std::string data;
    index.serialize_to(data);
    stream << account;
    stream << data;
    return std::string{reinterpret_cast<const char *>(stream.data()), static_cast<size_t>(stream.size())};
}

void xaccount_info_t::decode(const std::string & str) {
    base::xstream_t stream(base::xcontext_t::instance(), const_cast<uint8_t *>(reinterpret_cast<uint8_t const *>(str.data())), static_cast<int32_t>(str.size()));
    std::string index_str;
    stream >> account;
    stream >> index_str;
    index.serialize_from(index_str);
}

static xstate_mpt_caching_db_t & get_caching_db(base::xvdbstore_t * db) {
    static xstate_mpt_caching_db_t container(db);
    return container;
}

std::shared_ptr<xtop_state_mpt> xtop_state_mpt::create(common::xtable_address_t const & table,
                                                       evm_common::xh256_t const & root,
                                                       base::xvdbstore_t * db,
                                                       std::error_code & ec) {
    assert(!ec);

    auto mpt = std::make_shared<xtop_state_mpt>();
    mpt->init(table, root, db, ec);
    if (ec) {
        xwarn("xtop_state_mpt::create init error, %s %s", ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    return mpt;
}

void xtop_state_mpt::init(common::xtable_address_t const & table, const evm_common::xh256_t & root, base::xvdbstore_t * db, std::error_code & ec) {
    assert(!ec);

    m_table_address = table;
    m_trie_db = get_caching_db(db).trie_db(table);
    m_trie = evm_common::trie::xsecure_trie_t::build_from(root, m_trie_db, ec);
    if (ec) {
        xwarn("xtop_state_mpt::init trie with %s %s maybe not complete yes", table.to_string().c_str(), root.hex().c_str());
        return;
    }
}

base::xaccount_index_t xtop_state_mpt::get_account_index(common::xaccount_address_t const & account, std::error_code & ec) {
    auto obj = get_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::get_account_index get_state_object %s error: %s %s", account.to_string().c_str(), ec.category().name(), ec.message().c_str());
        return {};
    }
    if (obj != nullptr) {
        return obj->index;
    }
    return {};
}

void xtop_state_mpt::set_account_index(common::xaccount_address_t const & account, const std::string & index_str, std::error_code & ec) {
    base::xaccount_index_t index;
    index.serialize_from(index_str);
    return set_account_index(account, index, ec);
}

void xtop_state_mpt::set_account_index(common::xaccount_address_t const & account, const base::xaccount_index_t & index, std::error_code & ec) {
    auto obj = get_or_new_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::set_account_index %s %s", ec.category().name(), ec.message().c_str());
        return;
    }
    obj->set_account_index(index);
    m_state_objects_pending.insert(account);
}

std::shared_ptr<xstate_object_t> xtop_state_mpt::get_state_object(common::xaccount_address_t const & account, std::error_code & ec) {
    auto obj = get_deleted_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::get_state_object %s %s", ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    return obj;
}

std::shared_ptr<xstate_object_t> xtop_state_mpt::get_deleted_state_object(common::xaccount_address_t const & account, std::error_code & ec) {
    auto cache_obj = query_state_object(account);
    if (nullptr != cache_obj) {
        return cache_obj;
    }
    // get from db
    xbytes_t index_bytes;
    {
        // XMETRICS_TIME_RECORD("state_mpt_load_db_index");
        std::lock_guard<std::mutex> lock(m_trie_lock);
        index_bytes = m_trie->try_get(to_bytes(account), ec);
    }
    if (ec) {
        xwarn("xtop_state_mpt::get_deleted_state_object TryGet %s error, %s %s", account.to_string().c_str(), ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    if (index_bytes.empty()) {
        return nullptr;
    }
    xaccount_info_t info;
    info.decode({index_bytes.begin(), index_bytes.end()});
    auto obj = xstate_object_t::new_object(account, info.index);
    set_state_object(obj);
    return obj;
}

void xtop_state_mpt::set_state_object(std::shared_ptr<xstate_object_t> obj) {
    std::lock_guard<std::mutex> l(m_state_objects_lock);
    m_state_objects[obj->account] = obj;
}

std::shared_ptr<xstate_object_t> xtop_state_mpt::query_state_object(common::xaccount_address_t const& account) const {
    std::lock_guard<std::mutex> l(m_state_objects_lock);
    auto iter = m_state_objects.find(account);
    if (iter != m_state_objects.end()) {
        return iter->second;
    }
    return nullptr;
}

std::shared_ptr<xstate_object_t> xtop_state_mpt::get_or_new_state_object(common::xaccount_address_t const & account, std::error_code & ec) {
    auto obj = get_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::get_or_new_state_object error, %s %s", account.to_string().c_str(), ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    if (obj == nullptr) {
        obj = create_object(account, ec);
        if (ec) {
            xwarn("xtop_state_mpt::get_or_new_state_object error, %s %s", account.to_string().c_str(), ec.category().name(), ec.message().c_str());
            return nullptr;
        }
    }
    return obj;
}

std::shared_ptr<xstate_object_t> xtop_state_mpt::create_object(common::xaccount_address_t const & account, std::error_code & ec) {
    auto prev = get_deleted_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::create_object %s error, %s %s", account.to_string().c_str(), ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    auto obj = xstate_object_t::new_object(account, {});
    set_state_object(obj);
    return obj;
}

void xtop_state_mpt::prune_unit(const common::xaccount_address_t & account, std::error_code & ec) {
    // get from db
    xbytes_t index_bytes;
    {
        std::lock_guard<std::mutex> lock(m_trie_lock);
        // XMETRICS_TIME_RECORD("state_mpt_load_db_index");
        index_bytes = m_trie->try_get(to_bytes(account), ec);
    }
    if (ec) {
        xwarn("xtop_state_mpt::get_deleted_state_object TryGet %s error, %s %s", account.to_string().c_str(), ec.category().name(), ec.message().c_str());
        return;
    }
    if (index_bytes.empty()) {
        return;
    }
    xaccount_info_t info;
    info.decode({index_bytes.begin(), index_bytes.end()});
    auto hash = info.index.get_latest_unit_hash();
    auto key = base::xvdbkey_t::create_prunable_unit_state_key(base::xvaccount_t{account.to_string()}, info.index.get_latest_unit_height(), info.index.get_latest_unit_hash());
    m_trie_db->DiskDB()->DeleteDirect({key.begin(), key.end()}, ec);
    if (ec) {
        xwarn("xtop_state_mpt::prune_unit db Delete error: %s, %s", ec.category().name(), ec.message().c_str());
        return;
    }
    return;
}

evm_common::xh256_t xtop_state_mpt::get_root_hash(std::error_code & ec) {
    std::lock_guard<std::mutex> lock(m_trie_lock);
    for (auto & acc : m_state_objects_pending) {
        auto obj = query_state_object(acc);
        xaccount_info_t info;
        info.account = acc;
        info.index = obj->index;
        auto data = info.encode();
        m_trie->try_update(to_bytes(acc), {data.begin(), data.end()}, ec);
        if (ec) {
            xwarn("xtop_state_mpt::get_root_hash update_state_object %s error, %s %s", acc.to_string().c_str(), ec.category().name(), ec.message().c_str());
            return {};
        }
    }
    if (!m_state_objects_pending.empty()) {
        m_state_objects_pending.clear();
    }
    return m_trie->hash();
}

evm_common::xh256_t const & xtop_state_mpt::original_root_hash() const noexcept {
    return m_trie->original_root_hash();
}

evm_common::xh256_t xstate_mpt_t::commit(std::error_code & ec) {
    assert(!ec);

    get_root_hash(ec);
    if (ec) {
        xwarn("xstate_mpt_t::commit get_root_hash error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    {
        std::lock_guard<std::mutex> l(m_state_objects_lock);
        m_state_objects.clear();
    }

    std::lock_guard<std::mutex> lock(m_trie_lock);
    std::pair<evm_common::xh256_t, int32_t> res;
    {
        // XMETRICS_TIME_RECORD("state_mpt_trie_commit");
        res = m_trie->commit(ec);
    }
    if (ec) {
        xwarn("xstate_mpt_t::commit trie commit error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    // TODO: should call outside with roles of node
    m_trie_db->Commit(res.first, nullptr, ec);
    if (ec) {
        xwarn("xstate_mpt_t::commit db commit error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    return res.first;
}

void xtop_state_mpt::prune(evm_common::xh256_t const & old_trie_root_hash, std::unordered_set<evm_common::xh256_t> & pruned_hashes, std::error_code & ec) const {
    assert(!ec);

    std::lock_guard<std::mutex> lock(m_trie_lock);
    assert(m_trie != nullptr);
    m_trie->prune(old_trie_root_hash, pruned_hashes, ec);
}

void xtop_state_mpt::commit_pruned(std::unordered_set<evm_common::xh256_t> const & pruned_hashes, std::error_code & ec) const {
    assert(!ec);

    std::lock_guard<std::mutex> lock{m_trie_lock};
    assert(m_trie != nullptr);
    m_trie->commit_pruned(pruned_hashes, ec);
}

void xtop_state_mpt::prune(std::error_code & ec) {
    assert(!ec);

    m_trie->prune(ec);
    if (ec) {
        xwarn("xstate_mpt_t::commit pruning old trie data failed. category %s errc %d msg %s", ec.category().name(), ec.value(), ec.message().c_str());
    }
}

void xtop_state_mpt::commit_pruned(std::vector<evm_common::xh256_t> pruned_keys, std::error_code & ec) const {
    assert(!ec);
    std::lock_guard<std::mutex> lock{m_trie_lock};
    assert(m_trie != nullptr);
    m_trie->commit_pruned(std::move(pruned_keys), ec);
}

void xtop_state_mpt::clear_pruned(evm_common::xh256_t const & pruned_key, std::error_code & ec) const {
    assert(!ec);
    std::lock_guard<std::mutex> lock{m_trie_lock};
    assert(m_trie != nullptr);
    m_trie->clear_pruned(pruned_key, ec);
}

void xtop_state_mpt::clear_pruned(std::error_code & ec) const {
    assert(!ec);
    std::lock_guard<std::mutex> lock{m_trie_lock};
    assert(m_trie != nullptr);
    m_trie->clear_pruned(ec);
}

NS_END3
