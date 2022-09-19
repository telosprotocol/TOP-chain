// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_kv_db_face.h"
#include "xvledger/xvdbstore.h"

namespace top {
namespace state_mpt {

class xtop_state_mpt_db : public evm_common::trie::xkv_db_face_t {
public:
    xtop_state_mpt_db(base::xvdbstore_t * db, std::string table);
    ~xtop_state_mpt_db() = default;

    xbytes_t Get(xbytes_t const & key, std::error_code & ec);
    void Put(xbytes_t const & key, xbytes_t const & value, std::error_code & ec);
    void Delete(xbytes_t const & key, std::error_code & ec);
    bool Has(xbytes_t const & key, std::error_code & ec);

private:
    base::xvdbstore_t * m_db{nullptr};
    std::mutex m_mutex;
    std::string m_table;    // store key with table address
};
using xstate_mpt_db_t = xtop_state_mpt_db;

}  // namespace state_mpt
}  // namespace top