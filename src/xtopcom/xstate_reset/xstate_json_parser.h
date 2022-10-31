// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nlohmann/json.hpp"
#include "xbase/xns_macro.h"
#include "xvledger/xvaccount.h"  // todo use xtable_address_t

#include <string>

using json = nlohmann::json;

NS_BEG2(top, state_reset)

class xstate_json_parser {
public:
    xstate_json_parser(base::xvaccount_t const & table_account, std::string const & fork_name);

private:
    base::xvaccount_t m_table_account;  // todo use xtable_address_t
    std::string m_fork_name;
    json m_json_data;
};

NS_END2