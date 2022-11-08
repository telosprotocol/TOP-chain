// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <set>

namespace top {
namespace xtxpool_v2 {

class xtxpool_txcheck {
public:
    std::set<std::string>   refresh_and_get_new_black_addrs();

private:
    std::string             m_last_black_addr_config;
    std::set<std::string>   m_last_black_addrs;
};

}  // namespace xtxpool_v2
}  // namespace top
