// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xconfig/xconfig_register.h"
#include "xverifier/xverifier_errors.h"

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
    static bool  is_black_address(std::string const& addr);

private:
    static std::vector<std::string>  black_config();

};

using xblacklist_utl_t = xtop_blacklist_utl;

NS_END2
