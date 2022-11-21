// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt.h"

#include "xdata/xtable_bstate.h"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xmetrics/xmetrics.h"
#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_mpt_store.h"

namespace top {
namespace state_mpt {

std::string xaccount_info_t::encode() {
    base::xstream_t stream(base::xcontext_t::instance());
    std::string data;
    m_index.serialize_to(data);
    stream << m_account;
    stream << data;
    return std::string{(const char *)stream.data(), (size_t)stream.size()};
}

void xaccount_info_t::decode(const std::string & str) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)str.data(), (int32_t)str.size());
    std::string index_str;
    stream >> m_account;
    stream >> index_str;
    m_index.serialize_from(index_str);
    return;
}

std::shared_ptr<xtop_state_mpt> xtop_state_mpt::create(const common::xaccount_address_t & table,
                                                       const evm_common::xh256_t & root,
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

void xtop_state_mpt::init(const common::xaccount_address_t & table, const evm_common::xh256_t & root, base::xvdbstore_t * db, std::error_code & ec) {
    assert(!ec);

    m_table_address = table;
    auto const kv_db = std::make_shared<evm_common::trie::xkv_db_t>(db, table);
    m_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
    m_trie = evm_common::trie::xsecure_trie_t::build_from(root, m_db, ec);
    if (ec) {
        xwarn("xtop_state_mpt::init trie with %s %s maybe not complete yes", table.to_string().c_str(), root.hex().c_str());
        return;
    }
    m_original_root = root;
    return;
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

xbytes_t xtop_state_mpt::get_unit(common::xaccount_address_t const & account, std::error_code & ec) {
    auto obj = get_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::get_unit %s error: %s %s", account.to_string().c_str(), ec.category().name(), ec.message().c_str());
        return {};
    }
    if (obj != nullptr) {
        return obj->get_unit(m_db->DiskDB());
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
    if (obj->dirty_unit) {
        ec = error::xerrc_t::state_mpt_unit_hash_mismatch;
        xwarn("xtop_state_mpt::set_account_index %s unit already dirty", account.to_string().c_str());
        return;
    }
    obj->set_account_index(index);
    m_state_objects_pending.insert(account);
    m_state_objects_dirty.insert(account);
}

void xtop_state_mpt::set_account_index_with_unit(common::xaccount_address_t const & account, const std::string & index_str, const xbytes_t & unit, std::error_code & ec) {
    base::xaccount_index_t index;
    index.serialize_from(index_str);
    return set_account_index_with_unit(account, index, unit, ec);
}

void xtop_state_mpt::set_account_index_with_unit(common::xaccount_address_t const & account, const base::xaccount_index_t & index, const xbytes_t & unit, std::error_code & ec) {
    auto obj = get_or_new_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::set_account_index_with_unit %s %s", ec.category().name(), ec.message().c_str());
        return;
    }
    // auto hash = base::xcontext_t::instance().hash({unit.begin(), unit.end()}, enum_xhash_type_sha2_256);
    // if (hash != index.get_latest_state_hash()) {
    //     ec = error::xerrc_t::state_mpt_unit_hash_mismatch;
    //     xwarn("xtop_state_mpt::set_account_index_with_unit hash mismatch: %s, %s", to_hex(hash).c_str(), to_hex(index.get_latest_state_hash()).c_str());
    //     return;
    // }
    obj->set_account_index_with_unit(index, unit);
    m_state_objects_pending.insert(account);
    m_state_objects_dirty.insert(account);
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
        XMETRICS_TIME_RECORD("state_mpt_load_db_index");
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
    auto obj = xstate_object_t::new_object(account, info.m_index);
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
        XMETRICS_TIME_RECORD("state_mpt_load_db_index");
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
    auto hash = info.m_index.get_latest_unit_hash();
    auto key = base::xvdbkey_t::create_prunable_unit_state_key(base::xvaccount_t{account.to_string()}, info.m_index.get_latest_unit_height(), info.m_index.get_latest_unit_hash());
    m_db->DiskDB()->DeleteDirect({key.begin(), key.end()}, ec);
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
        info.m_account = acc;
        info.m_index = obj->index;
        auto data = info.encode();
        m_trie->try_update(to_bytes(acc), {data.begin(), data.end()}, ec);
        if (ec) {
            xwarn("xtop_state_mpt::get_root_hash update_state_object %s error, %s %s", acc.to_string().c_str(), ec.category().name(), ec.message().c_str());
            return {};
        }
    }
    if (m_state_objects_pending.size() > 0) {
        m_state_objects_pending.clear();
    }
    return m_trie->hash();
}

const evm_common::xh256_t & xtop_state_mpt::get_original_root_hash() const {
    return m_original_root;
}

evm_common::xh256_t xtop_state_mpt::commit(std::error_code & ec) {
    get_root_hash(ec);
    if (ec) {
        xwarn("xtop_state_mpt::commit get_root_hash error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    for (auto & acc : m_state_objects_dirty) {
        auto obj = query_state_object(acc);
        if (obj == nullptr) {
            continue;
        }
        std::map<xbytes_t, xbytes_t> batch;
        if (!obj->unit_bytes.empty() && obj->dirty_unit) {
            auto unit_key = base::xvdbkey_t::create_prunable_unit_state_key(
                base::xvaccount_t{obj->account.to_string()}, obj->index.get_latest_unit_height(), obj->index.get_latest_unit_hash());
            batch.emplace(std::make_pair(xbytes_t{unit_key.begin(), unit_key.end()}, obj->unit_bytes));
            if (batch.size() >= 1024) {
                WriteUnitBatch(m_db->DiskDB(), batch);
                batch.clear();
            }
            obj->dirty_unit = false;
        }
    }
    if (!m_state_objects_dirty.empty()) {
        m_state_objects_dirty.clear();
    }
    {
        std::lock_guard<std::mutex> l(m_state_objects_lock);
        m_state_objects.clear();
    }

    std::lock_guard<std::mutex> lock(m_trie_lock);
    std::pair<evm_common::xh256_t, int32_t> res;
    {
        XMETRICS_TIME_RECORD("state_mpt_trie_commit");
        res = m_trie->commit(ec);
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

void xtop_state_mpt::load_into(std::unique_ptr<xstate_mpt_store_t> const & state_mpt_store, std::error_code & ec) {
}

void xtop_state_mpt::prune(evm_common::xh256_t const & old_trie_root_hash, std::error_code & ec) const {
    assert(!ec);
    assert(m_trie != nullptr);

    std::lock_guard<std::mutex> lock(m_trie_lock);
    m_trie->prune(old_trie_root_hash, ec);
}

void xtop_state_mpt::commit_pruned(std::error_code & ec) const {
    assert(!ec);
    assert(m_trie != nullptr);

    std::lock_guard<std::mutex> lock{m_trie_lock};
    m_trie->commit_pruned(ec);
}


}  // namespace state_mpt
}  // namespace top
