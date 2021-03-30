// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelect_transaction.hpp"

NS_BEG2(top, data)

std::string xtcc_transaction_t::get_genesis_whitelist() const {
    std::string res = "";
    std::map<std::string, uint64_t> genesis_accounts = xrootblock_t::get_all_genesis_accounts();
    for (auto const& item : genesis_accounts) {
        res += item.first + ",";
    }

    return res.substr(0, res.size() - 1);
}

 std::string xtcc_transaction_t::get_tcc_onchain_committee_list() const {
    std::string res = "";

    std::vector<std::string> committee_addrs = xrootblock_t::get_tcc_initial_committee_addr();
    for (auto const& item : committee_addrs) {
        res += item + ",";
    }

    return res.substr(0, res.size() - 1);
 }

NS_END2
