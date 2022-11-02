// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlru_cache.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"
#include "xevm_common/trie/xsecure_trie.h"
#include "xevm_common/trie/xtrie_db.h"
#include "xstate_mpt/xerror.h"

namespace top {
namespace state_mpt {

static constexpr uint32_t code_cache_size = 8 * 1024 * 1024;

class xtop_state_mpt_database_face {
public:
    virtual evm_common::trie::xtrie_face_ptr_t open_trie(xhash256_t hash, std::error_code & ec) = 0;
    virtual xbytes_t unit(xhash256_t hash, std::error_code & ec) = 0;
    virtual evm_common::trie::xtrie_db_ptr_t trie_db() = 0;
};

class xtop_state_mpt_caching_db : public xtop_state_mpt_database_face {
public:
    xtop_state_mpt_caching_db(evm_common::trie::xkv_db_face_ptr_t db) : m_db(evm_common::trie::xtrie_db_t::NewDatabase(db)), m_code_cache(code_cache_size) {
    }
    ~xtop_state_mpt_caching_db() = default;

    evm_common::trie::xtrie_face_ptr_t open_trie(xhash256_t hash, std::error_code & ec) override {
        return evm_common::trie::xsecure_trie_t::NewSecure(hash, m_db, ec);
    }

    xbytes_t unit(xhash256_t hash, std::error_code & ec) override {
        xbytes_t v;
        auto exist = m_code_cache.get(hash, v);
        if (exist && !v.empty()) {
            return v;
        }
        v = ReadUnitWithPrefix(m_db->DiskDB(), hash);
        if (!v.empty()) {
            m_code_cache.put(hash, v);
        } else {
            ec = error::xerrc_t::state_mpt_db_not_found;
        }
        return v;
    }

    evm_common::trie::xtrie_db_ptr_t trie_db() override {
        return m_db;
    }

    static std::shared_ptr<xtop_state_mpt_caching_db> NewDatabase(evm_common::trie::xkv_db_face_ptr_t diskdb);
    static std::shared_ptr<xtop_state_mpt_caching_db> NewDatabaseWithConfig(evm_common::trie::xkv_db_face_ptr_t diskdb, evm_common::trie::xtrie_db_config_ptr_t config);

private:
    evm_common::trie::xtrie_db_ptr_t m_db;
    base::xlru_cache<xhash256_t, xbytes_t> m_code_cache;
};
using xstate_mpt_caching_db_t = xtop_state_mpt_caching_db;

}  // namespace state_mpt
}  // namespace top