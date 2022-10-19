// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xmemory.hpp"

#include <map>
#include <mutex>

NS_BEG2(top, xunit_service)

class xindex_upgrade_t {
public:
    bool is_turn_to_upgrade(const std::string & table_addr);
    void set_upgrade_succ(const std::string & table_addr);

private:
    void init();

private:
    mutable std::mutex m_mutex;
    bool inited{false};
    std::map<std::string, std::pair<std::string, bool>> m_table_upgrade_flags; // key: table addr, value: first:next table addr, second: turn to upgrade
};

NS_END2
