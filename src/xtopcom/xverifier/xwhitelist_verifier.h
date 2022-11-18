// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <set>

namespace top {
namespace xverifier {

/**
 * @brief utl for whitelist
 *
 * simple interface for realize the whitelist function
 *
 */
class xwhitelist_utl {
public:
    /**
     * @brief judge account whether included in whiltelist
     *
     * @param addr    addr  the account to judge whether included in whitelist
     *
     * @return true  means account addr included in whitelist
     * @return false  means account addr not included in whitelist
     */
    static bool is_white_address_limit(std::string const& source_addr);

    /// helper
    static std::set<std::string>    whitelist_config();
    static bool                     is_whitelist_enable();
};

}  // namespace xverifier
}  // namespace top
