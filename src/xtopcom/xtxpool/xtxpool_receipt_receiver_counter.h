// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbase/xvledger.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xdata/xblocktool.h"
#include <string>

NS_BEG2(top, xtxpool)

#define recv_table_accounts_max_accounts (5)

class xtxpool_receipt_receiver_counter {
public:
    xtxpool_receipt_receiver_counter() {};
    virtual ~xtxpool_receipt_receiver_counter() {};

public:
    bool is_receiver_count_full_and_insert(const std::string recv_account) {
        auto xid = base::xvaccount_t::get_xid_from_account(recv_account);
        uint8_t zone = get_vledger_zone_index(xid);
        uint16_t table_id = get_vledger_subaddr(xid);
        uint32_t table = (zone << 16) + table_id;
        auto iter = receiver_table_accounts_map.find(table);
        if (iter == receiver_table_accounts_map.end()) {
            std::set<std::string> accounts;
            accounts.insert(recv_account);
            receiver_table_accounts_map[table] = accounts;
            return false;
        } else {
            auto & accounts_tmp = iter->second;
            if (accounts_tmp.size() >= recv_table_accounts_max_accounts) {
                return true;
            }
            auto accout = accounts_tmp.find(recv_account);
            if (accout != accounts_tmp.end()) {
                return true;
            }
            accounts_tmp.insert(recv_account);
            return false;
        }
    }

private:
    std::map<uint32_t, std::set<std::string>> receiver_table_accounts_map;
};

NS_END2
