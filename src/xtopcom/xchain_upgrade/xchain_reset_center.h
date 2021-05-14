// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xcommon/xaddress.h"

namespace top
{
    namespace chain_reset
    {
        class xtop_chain_reset_center
        {
        public:
            static void get_reset_stake_string_property(common::xaccount_address_t const &addr, std::string const &property, std::string &value);
            static void get_reset_stake_map_property(common::xaccount_address_t const &addr, std::string const &property, std::vector<std::pair<std::string, std::string>> &map);
            static void get_reset_property(common::xaccount_address_t const &addr, std::string const &property, std::string &value);
        };
        using xchain_reset_center_t = xtop_chain_reset_center;
    }
}
