// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include <string>
#include <set>
#include <vector>

NS_BEG2(top, xverifier)
/**
 * @brief utl for blacklist
 *
 * simple interface for realize the blocklist function
 *
 */
class xtop_blacklist_utl {
public:
    static xtop_blacklist_utl & instance() {
        static xtop_blacklist_utl _instance;
        return _instance;
    }
    static bool  is_black_address(std::string const& addr);
    static bool  is_black_address(std::string const& tx_source_addr, std::string const& tx_target_addr);
    static std::set<std::string>     black_config();

    std::vector<std::string>    refresh_and_get_new_addrs();

private:
    std::set<std::string>    m_last_black_addrs;
};

using xblacklist_utl_t = xtop_blacklist_utl;

NS_END2
