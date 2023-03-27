// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xstatectx/xunitstate_ctx.h"

NS_BEG2(top, statestore)

class xblock_account_indexes_t {
public:
    xblock_account_indexes_t(const std::string & block_hash, const std::map<std::string, base::xaccount_index_t> & account_index_map);
    const std::string & get_block_hash() const {return m_block_hash;}
    bool get_account_index(const std::string & account, base::xaccount_index_t & account_index) const;
private:
    std::string m_block_hash;
    std::map<std::string, base::xaccount_index_t> m_account_index_map;
};

class xaccount_index_cache_t {
public:
    void update_new_cert_block(base::xvblock_t* cert_block, const std::map<std::string, base::xaccount_index_t> & account_index_map);
    bool get_account_index(base::xvblock_t* block, const std::string & account, base::xaccount_index_t & account_index);
    bool cache_unbroken(base::xvblock_t* block);
private:
    std::map<uint64_t, xblock_account_indexes_t> m_cache;
    mutable std::mutex m_mutex;
};

NS_END2
