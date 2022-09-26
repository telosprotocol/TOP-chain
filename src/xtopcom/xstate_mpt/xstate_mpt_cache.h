// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlru_cache.h"
#include "xbasic/xhash.hpp"
#include "xcommon/xaccount_address_fwd.h"

namespace top {
namespace state_mpt {
class xtop_state_mpt_cache {
public:
    ~xtop_state_mpt_cache() = default;

    static xtop_state_mpt_cache * instance();

    std::string get(const std::string & table, const xhash256_t & root, const std::string & account);
    void set(const std::string & table, const xhash256_t & root, const std::string & account, const std::string & value);
    void set(const std::string & table, const xhash256_t & root, const std::map<std::string, xbytes_t> & batch);

    std::shared_ptr<base::xlru_cache<std::string, std::string>> get_lru(const std::string & table);
    static std::string get(std::shared_ptr<base::xlru_cache<std::string, std::string>> lru, const xhash256_t & root, const std::string & account);
    static void set(std::shared_ptr<base::xlru_cache<std::string, std::string>> lru, const xhash256_t & root, common::xaccount_address_t const & account, const std::string & value);
    static void set(std::shared_ptr<base::xlru_cache<std::string, std::string>> lru, const xhash256_t & root, const std::map<common::xaccount_address_t, xbytes_t> & batch);
    
private:
    xtop_state_mpt_cache() = default;

    std::map<std::string, std::shared_ptr<base::xlru_cache<std::string, std::string>>> m_cache;
};
using xstate_mpt_cache_t = xtop_state_mpt_cache;

}
}
