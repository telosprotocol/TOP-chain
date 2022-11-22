// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xnode_id.h"
#include "xevm_common/trie/xtrie_kv_db_face.h"
#include "xvledger/xvdbstore.h"

namespace top {
namespace evm_common {
namespace trie {

class xtop_kv_db : public xkv_db_face_t {
public:
    xtop_kv_db(base::xvdbstore_t * db, common::xaccount_address_t table);
    ~xtop_kv_db() = default;

    xbytes_t Get(xbytes_t const & key, std::error_code & ec) override;
    xbytes_t GetDirect(xbytes_t const & key, std::error_code & ec) override;

    bool Has(xbytes_t const & key, std::error_code & ec) override;
    bool HasDirect(xbytes_t const & key, std::error_code & ec) override;

    void Put(gsl::span<xbyte_t const> key, xbytes_t const & value, std::error_code & ec) override;
    void PutBatch(std::map<xbytes_t, xbytes_t> const & batch, std::error_code & ec) override;
    void PutDirect(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) override;
    void PutDirectBatch(std::map<xbytes_t, xbytes_t> const & batch, std::error_code & ec) override;

    void Delete(xbytes_t const & key, std::error_code & ec) override;
    void DeleteBatch(std::vector<xbytes_t> const & batch, std::error_code & ec) override;
    void DeleteDirect(xbytes_t const & key, std::error_code & ec) override;
    void DeleteDirectBatch(std::vector<xbytes_t> const & batch, std::error_code & ec) override;

private:
    std::string convert_key(gsl::span<xbyte_t const> key) const;

    base::xvdbstore_t * m_db{nullptr};
    std::mutex m_mutex;
    common::xaccount_address_t m_table;  // store key with table address
    std::string m_node_key_prefix;
};
using xkv_db_t = xtop_kv_db;

}  // namespace trie
}  // namespace state_mpt
}  // namespace top
