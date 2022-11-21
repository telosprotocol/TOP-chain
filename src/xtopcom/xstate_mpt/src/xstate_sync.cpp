// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_sync.h"

#include "xcommon/xnode_id.h"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xvledger/xvdbkey.h"

namespace top {
namespace state_mpt {

std::shared_ptr<evm_common::trie::Sync> new_state_sync(const common::xaccount_address_t & table, evm_common::xh256_t const & root, base::xvdbstore_t * db, bool sync_unit) {
    auto syncer = evm_common::trie::Sync::NewSync(std::make_shared<evm_common::trie::xkv_db_t>(db, table));
    auto callback = [&table, &root, sync_unit, weak_syncer = std::weak_ptr<evm_common::trie::Sync>(syncer)](
                        std::vector<xbytes_t> const & path, xbytes_t const & hexpath, xbytes_t const & value, evm_common::xh256_t const & parent, std::error_code & ec) {
        if (value.empty()) {
            ec = error::xerrc_t::state_mpt_leaf_empty;
            return;
        }
        if (sync_unit) {
            xaccount_info_t info;
            info.decode({value.begin(), value.end()});
            auto const & state_hash_str = info.m_index.get_latest_state_hash();
            xassert(!info.m_index.get_latest_unit_hash().empty());
            auto const hash = evm_common::xh256_t(xbytes_t{state_hash_str.begin(), state_hash_str.end()});
            auto const state_key = base::xvdbkey_t::create_prunable_unit_state_key(info.m_account.vaccount(), info.m_index.get_latest_unit_height(), info.m_index.get_latest_unit_hash());
            auto const syncer = weak_syncer.lock();
            if (syncer == nullptr) {
                return;
            }
            syncer->AddUnitEntry(hash, hexpath, value, {state_key.begin(), state_key.end()}, parent);
            xinfo("state_mpt::new_state_sync table: %s, root: %s, value: %s, hash: %s, state_key: %s, index_dump: %s",
                  table.to_string().c_str(),
                  root.hex().c_str(),
                  to_hex(value).c_str(),
                  hash.hex().c_str(),
                  to_hex(state_key).c_str(),
                  info.m_index.dump().c_str());
        }
    };
    syncer->Init(root, callback);
    return syncer;
}

}  // namespace state_mpt
}  // namespace top
