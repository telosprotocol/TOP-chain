// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_sync.h"

#include "xcommon/xnode_id.h"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xvledger/xaccountindex.h"

namespace top {
namespace state_mpt {

std::shared_ptr<evm_common::trie::Sync> new_state_sync(const common::xaccount_address_t & table, const xhash256_t & root, base::xvdbstore_t * db, bool sync_unit) {
    auto syncer = evm_common::trie::Sync::NewSync(std::make_shared<evm_common::trie::xkv_db_t>(db, table));
    auto callback = [=](std::vector<xbytes_t> const & path, xbytes_t const & hexpath, xbytes_t const & value, xhash256_t const & parent, std::error_code & ec) {
        if (value.empty()) {
            ec = error::xerrc_t::state_mpt_leaf_empty;
            return;
        }
        if (sync_unit) {
            xaccount_info_t info;
            info.decode({value.begin(), value.end()});
            auto state_hash_str = info.m_index.get_latest_state_hash();
            auto unit_hash_str = info.m_index.get_latest_unit_hash();
            xassert(!unit_hash_str.empty());
            auto hash = static_cast<xhash256_t>(xbytes_t{state_hash_str.begin(), state_hash_str.end()});
            auto key = static_cast<xhash256_t>(xbytes_t{unit_hash_str.begin(), unit_hash_str.end()});
            syncer->AddUnitEntry(hash, hexpath, key, parent);
            xinfo("state_mpt::new_state_sync table: %s, root: %s, value: %s, hash: %s, key: %s, index_dump: %s",
                  table.c_str(),
                  root.as_hex_str().c_str(),
                  to_hex(value).c_str(),
                  hash.as_hex_str().c_str(),
                  key.as_hex_str().c_str(),
                  info.m_index.dump().c_str());
        }
    };
    syncer->Init(root, callback);
    return syncer;
}

}  // namespace state_mpt
}  // namespace top