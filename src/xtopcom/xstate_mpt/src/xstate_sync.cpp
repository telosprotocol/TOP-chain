// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_sync.h"

#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_mpt_db.h"
#include "xvledger/xaccountindex.h"

namespace top {
namespace state_mpt {

std::shared_ptr<evm_common::trie::Sync> new_state_sync(const std::string & table, const xhash256_t & root, base::xvdbstore_t * db, bool sync_unit) {
    auto syncer = evm_common::trie::Sync::NewSync(std::make_shared<xstate_mpt_db_t>(db, table));
    auto callback = [=](std::vector<xbytes_t> const & path, xbytes_t const & hexpath, xbytes_t const & value, xhash256_t const & parent, std::error_code & ec) {
        if (value.empty()) {
            ec = error::xerrc_t::state_mpt_leaf_empty;
            return;
        }
        if (sync_unit) {
            base::xaccount_index_t index;
            index.serialize_from({value.begin(), value.end()});
            auto state_hash = index.get_latest_state_hash();
            auto hash = static_cast<xhash256_t>(xbytes_t{state_hash.begin(), state_hash.end()});
            syncer->AddUnitEntry(hash, hexpath, parent);
            xdbg("state_mpt::new_state_sync value: %s, req: %s, index_hash: %s", to_hex(value).c_str(), parent.as_hex_str().c_str(), hash.as_hex_str().c_str());
        }
    };
    syncer->Init(root, callback);
    return syncer;
}

}  // namespace state_mpt
}  // namespace top