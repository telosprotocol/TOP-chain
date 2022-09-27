// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt.h"

#include "xdata/xtable_bstate.h"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xmetrics/xmetrics.h"
#include "xstate_mpt/xerror.h"

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
                                                       const xhash256_t & root,
                                                       base::xvdbstore_t * db,
                                                       xstate_mpt_cache_t * cache,
                                                       std::error_code & ec) {
    cache = nullptr;  // TODO(jimmy)

    auto mpt = std::make_shared<xtop_state_mpt>();
    mpt->init(table, root, db, cache, ec);
    if (ec) {
        xwarn("xtop_state_mpt::create init error, %s %s", ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    return mpt;
}

void xtop_state_mpt::init(const common::xaccount_address_t & table, const xhash256_t & root, base::xvdbstore_t * db, xstate_mpt_cache_t * cache, std::error_code & ec) {
    // check sync flag

    m_table_address = table;
    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(db, table);
    m_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
    m_trie = evm_common::trie::xsecure_trie_t::NewSecure(root, m_db, ec);
    if (ec) {
        xwarn("xtop_state_mpt::init trie with %s %s maybe not complete yes", table.c_str(), root.as_hex_str().c_str());
        return;
    }
    if (evm_common::trie::ReadTrieSyncFlag(kv_db, root)) {
        xwarn("xtop_state_mpt::init generate trie with %s %s failed: %s, %s", table.c_str(), root.as_hex_str().c_str(), ec.category().name(), ec.message().c_str());
        ec = error::xerrc_t::state_mpt_not_complete;
        return;
    }
    m_original_root = root;
    if (cache != nullptr) {
        // m_lru = cache->get_lru(table);
    }
    return;
}

base::xaccount_index_t xtop_state_mpt::get_account_index(common::xaccount_address_t const & account, std::error_code & ec) {
    auto obj = get_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::get_account_index get_state_object %s error: %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
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
        xwarn("xtop_state_mpt::get_unit %s error: %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
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
        xwarn("xtop_state_mpt::set_account_index %s unit already dirty", account.c_str());
        return;
    }
    m_journal.append(obj->set_account_index(index));
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
    auto hash = base::xcontext_t::instance().hash({unit.begin(), unit.end()}, enum_xhash_type_sha2_256);
    if (hash != index.get_latest_state_hash()) {
        ec = error::xerrc_t::state_mpt_unit_hash_mismatch;
        xwarn("xtop_state_mpt::set_account_index_with_unit hash mismatch: %s, %s", to_hex(hash).c_str(), to_hex(index.get_latest_state_hash()).c_str());
        return;
    }
    m_journal.append(obj->set_account_index_with_unit(index, unit));
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
    if (m_state_objects.count(account)) {
        return m_state_objects[account];
    }
    // get from cache
    if (m_lru != nullptr) {
        // TODO
    }
    // get from db
    xbytes_t index_bytes;
    {
        XMETRICS_TIME_RECORD("state_mpt_load_db_index");
        index_bytes = m_trie->TryGet(to_bytes(account), ec);
    }
    if (ec) {
        xwarn("xtop_state_mpt::get_deleted_state_object TryGet %s error, %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    if (index_bytes.empty()) {
        xwarn("xtop_state_mpt::get_deleted_state_object TryGet %s empty", account.c_str());
        return nullptr;
    }
    if (m_lru != nullptr) {
        m_cache_indexes.insert({account, index_bytes});
    }
    xaccount_info_t info;
    info.decode({index_bytes.begin(), index_bytes.end()});
    auto obj = xstate_object_t::new_object(account, info.m_index);
    set_state_object(obj);
    return obj;
}

void xtop_state_mpt::set_state_object(std::shared_ptr<xstate_object_t> obj) {
    m_state_objects[obj->account] = obj;
}

std::shared_ptr<xstate_object_t> xtop_state_mpt::get_or_new_state_object(common::xaccount_address_t const & account, std::error_code & ec) {
    auto obj = get_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::get_or_new_state_object error, %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    if (obj == nullptr) {
        obj = create_object(account, ec);
        if (ec) {
            xwarn("xtop_state_mpt::get_or_new_state_object error, %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
            return nullptr;
        }
    }
    return obj;
}

std::shared_ptr<xstate_object_t> xtop_state_mpt::create_object(common::xaccount_address_t const & account, std::error_code & ec) {
    auto prev = get_deleted_state_object(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::create_object %s error, %s %s", account.c_str(), ec.category().name(), ec.message().c_str());
        return nullptr;
    }
    auto obj = xstate_object_t::new_object(account, {});
    if (prev == nullptr) {
        // TODO
        // s.journal.append
    } else {

    }
    set_state_object(obj);
    return obj;
}

void xtop_state_mpt::update_state_object(std::shared_ptr<xstate_object_t> obj, std::error_code & ec) {
    auto acc = obj->account;
    xaccount_info_t info;
    info.m_account = acc;
    info.m_index = obj->index;
    auto data = info.encode();
    m_trie->TryUpdate(to_bytes(acc), {data.begin(), data.end()}, ec);
    if (ec) {
        xwarn("xtop_state_mpt::update_state_object %s error, %s %s", acc.c_str(), ec.category().name(), ec.message().c_str());
        return;
    }
    return;
}

void xtop_state_mpt::prune_unit(const common::xaccount_address_t & account, std::error_code & ec) {
    auto index = get_account_index(account, ec);
    if (ec) {
        xwarn("xtop_state_mpt::prune_unit get_account_index error: %s, %s", ec.category().name(), ec.message().c_str());
        return;
    }
    auto hash = index.get_latest_unit_hash();
    auto key = evm_common::trie::schema::unitKey(xhash256_t(to_bytes(hash)));
    m_db->DiskDB()->Delete(key, ec);
    if (ec) {
        xwarn("xtop_state_mpt::prune_unit db Delete error: %s, %s", ec.category().name(), ec.message().c_str());
        return;
    }
    return;
}

xhash256_t xtop_state_mpt::get_root_hash(std::error_code & ec) {
    finalize();
    for (auto & acc : m_state_objects_pending) {
        auto obj = m_state_objects[acc];
        update_state_object(obj, ec);
        if (ec) {
            xwarn("xtop_state_mpt::get_root_hash update_state_object %s error, %s %s", acc.c_str(), ec.category().name(), ec.message().c_str());
            return {};
        }
    }
    if (m_state_objects_pending.size() > 0) {
        m_state_objects_pending.clear();
    }
    return m_trie->Hash();
}

const xhash256_t & xtop_state_mpt::get_original_root_hash() const {
    return m_original_root;
}

std::shared_ptr<evm_common::trie::xtrie_db_t> xtop_state_mpt::get_database() const {
    return m_db;
}

void xtop_state_mpt::finalize() {
    for (auto & pair : m_journal.dirties) {
        auto acc = pair.first;
        if (!m_state_objects.count(acc)) {
            continue;
        }
        m_state_objects_pending.insert(acc);
        m_state_objects_dirty.insert(acc);
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
    auto new_hash = get_root_hash(ec);
    if (ec) {
        xwarn("xtop_state_mpt::commit get_root_hash error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }

    for (auto & acc : m_state_objects_dirty) {
        auto obj = m_state_objects[acc];
        if (!obj->unit_bytes.empty() && obj->dirty_unit) {
            auto hash = base::xcontext_t::instance().hash({obj->unit_bytes.begin(), obj->unit_bytes.end()}, enum_xhash_type_sha2_256);
            WriteUnit(m_db->DiskDB(), xhash256_t({hash.begin(), hash.end()}), obj->unit_bytes);
            obj->dirty_unit = false;
        }
    }
    if (m_state_objects_dirty.size() > 0) {
        m_state_objects_dirty.clear();
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
    // clear old flag
    evm_common::trie::DeleteTrieSyncFlag(m_db->DiskDB(), new_hash);
    // TODO: should call outside with roles of node
    m_db->Commit(res.first, nullptr, ec);
    if (ec) {
        xwarn("xtop_state_mpt::commit db commit error, %s %s", ec.category().name(), ec.message().c_str());
        return {};
    }
    if (m_lru != nullptr) {
        xstate_mpt_cache_t::set(m_lru, res.first, m_cache_indexes);
    }
    return res.first;
}

}  // namespace state_mpt
}  // namespace top