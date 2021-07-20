// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"

namespace top
{
    namespace chain_reset
    {
        struct reset_data_t {
            std::string address;
            int64_t top_balance{0};
            int64_t burn_balance{0};
            int64_t tgas_balance{0};
            int64_t vote_balance{0};
            int64_t lock_balance{0};
            int64_t lock_tgas{0};
            int64_t unvote_num{0};
            int64_t expire_vote{0};
            int64_t create_time{0};
            int64_t lock_token{0};
            std::vector<std::string> pledge_vote;
        };
        class xtop_chain_reset_center
        {
        public:
            static void get_reset_all_user_data(std::vector<reset_data_t> & reset_data);
            static void get_reset_all_contract_data(std::vector<reset_data_t> & reset_data);
            static void get_reset_user_data(common::xaccount_address_t const &addr, reset_data_t & reset_data);
            static void get_reset_contract_data(common::xaccount_address_t const &addr, reset_data_t & reset_data);
            static void get_reset_stake_string_property(common::xaccount_address_t const &addr, std::string const &property, std::string &value);
            static void get_reset_stake_map_property(common::xaccount_address_t const &addr, std::string const &property, std::vector<std::pair<std::string, std::string>> &map);
            static void release_reset_property();
        };
        using xchain_reset_center_t = xtop_chain_reset_center;
    }
}
