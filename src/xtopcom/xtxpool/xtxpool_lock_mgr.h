// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xblock.h"
#include "xtxpool/xaccountobj.h"

#include <map>
#include <string>
#include <vector>

NS_BEG2(top, xtxpool)

class xtxpool_lock_mgr {
 public:
    xtxpool_lock_mgr() = default;
    virtual ~xtxpool_lock_mgr();

    void update_blocks(const base::xblock_mptrs & latest_blocks);
    bool is_account_lock(const std::string & account);

 private:
    std::map<std::string, uint64_t>         m_lock_accounts;
};

NS_END2
