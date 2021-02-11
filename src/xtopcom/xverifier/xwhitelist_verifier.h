// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xcontext.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xverifier/xverifier_errors.h"

#include <string>
#include <tuple>
#include <vector>

namespace top {
namespace xverifier {

using onchain_whitelist = std::vector<std::string>; /// structure to store the whitelist
/// the limit interface
std::string const vote_interface = "voteNode";
std::string const claim_reward_interface = "claimVoterDividend";

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
    static bool include_in_whitelist(std::string const & addr);

    /**
     * @brief  judge limited tx
     *
     * @param tx  the tx to judge wheher limited
     * @return true  means tx is limited
     * @return false  means tx not limited
     */
    static bool check_whitelist_limit_tx(data::xtransaction_t const * tx);

    /**
     * @brief split the input string
     *
     * @param input  the input string
     * @param split_char  the seperated char to split the string
     * @param values  the splited result
     *
     * @return int  the size of the result
     */
    static int split_string(const std::string & input, const char split_char, std::vector<std::string> & values);

    /// helper
    static onchain_whitelist get_whitelist_from_config();

    /// the onchain whitelist
    static onchain_whitelist wl;
};

}  // namespace xverifier
}  // namespace top
