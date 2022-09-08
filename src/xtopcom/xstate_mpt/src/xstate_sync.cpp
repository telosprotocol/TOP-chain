// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_sync.h"
#include "xstate_mpt/xstate_mpt_db.h"

namespace top {
namespace state_mpt {

std::shared_ptr<evm_common::trie::Sync> new_state_sync(const std::string & table, const xhash256_t & root, base::xvdbstore_t * db) {
    std::shared_ptr<evm_common::trie::Sync> syncer{nullptr};
    auto callback = [&](std::vector<xbytes_t> const &, xbytes_t const &, xbytes_t const &, xhash256_t const &, std::error_code &) {
        // TODO: add unit tasks?
        // syncer->AddCodeEntry();
        // syncer->AddSubTrie();
    };
    syncer = evm_common::trie::Sync::NewSync(root, std::make_shared<xstate_mpt_db_t>(db, table), callback);
    return syncer;
}

}  // namespace state_mpt
}  // namespace top