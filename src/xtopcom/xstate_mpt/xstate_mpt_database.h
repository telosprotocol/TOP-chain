// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlru_cache.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"
#include "xcommon/xnode_id.h"
#include "xevm_common/trie/xsecure_trie.h"
#include "xevm_common/trie/xtrie_db.h"
#include "xstate_mpt/xerror.h"
#include "xvledger/xvdbstore.h"

namespace top {
namespace state_mpt {

class xtop_state_mpt_caching_db {
public:
    explicit xtop_state_mpt_caching_db(base::xvdbstore_t * db);
    ~xtop_state_mpt_caching_db() = default;

    std::shared_ptr<evm_common::trie::xtrie_face_t> open_trie(common::xtable_address_t const & table, xhash256_t const & hash, std::error_code & ec);
    std::shared_ptr<evm_common::trie::xtrie_db_t> trie_db(common::xtable_address_t const & table);

private:
    std::map<common::xtable_address_t, std::shared_ptr<evm_common::trie::xtrie_db_t>> m_table_caches;
    base::xvdbstore_t * m_db{nullptr};
    std::mutex m_mutex;
};
using xstate_mpt_caching_db_t = xtop_state_mpt_caching_db;

}  // namespace state_mpt
}  // namespace top