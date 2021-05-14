// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchain_upgrade/xchain_reset_center.h"
#include "xchain_upgrade/xchain_reset_data.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace top
{
    namespace chain_reset
    {
        void xtop_chain_reset_center::get_reset_stake_string_property(common::xaccount_address_t const &addr, std::string const &property, std::string &value)
        {
            value = base::xstring_utl::base64_decode(stake_property.at(addr.to_string()).at(property));
        }

        void xtop_chain_reset_center::get_reset_stake_map_property(common::xaccount_address_t const &addr, std::string const &property, std::vector<std::pair<std::string, std::string>> &map)
        {
            auto data = stake_property.at(addr.to_string()).at(property);
            for (auto _p = data.begin(); _p != data.end(); ++_p)
            {
                map.push_back(std::make_pair(base::xstring_utl::base64_decode(_p.key()), base::xstring_utl::base64_decode(_p.value())));
            }
        }

        void xtop_chain_reset_center::get_reset_property(common::xaccount_address_t const &addr, std::string const &property, std::string &value)
        {
            value = base::xstring_utl::base64_decode(all_property.at(addr.to_string()).at("native_property").at(property));
        }
    }
}