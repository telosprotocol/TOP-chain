// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "nlohmann/json.hpp"
#include "xbase/xns_macro.h"
#include "xvledger/xvaccount.h"  // todo use xtable_address_t

#include <string>

using json = nlohmann::json;

NS_BEG2(top, state_reset)

class xstate_json_parser {
public:
    xstate_json_parser(base::xvaccount_t const & table_account, std::string const & fork_name);

public:
    using iterator = json::iterator;
    using const_iterator = json::const_iterator;
    using key_type = std::string;  // each unit address
    using value_type = json;       // address' unit state properties json

    bool empty() const noexcept {
        return m_json_data.empty();
    }

    std::size_t size() const noexcept {
        return m_json_data.size();
    }

    iterator begin() noexcept {
        return m_json_data.begin();
    }

    const_iterator begin() const noexcept {
        return m_json_data.begin();
    }

    const_iterator cbegin() const noexcept {
        return m_json_data.cbegin();
    }

    iterator end() noexcept {
        return m_json_data.end();
    }

    const_iterator end() const noexcept {
        return m_json_data.end();
    }

    const_iterator cend() const noexcept {
        return m_json_data.cend();
    }

    const_iterator find(key_type const & address) const {
        return m_json_data.find(address);
    }

    const char * table_account_str() const {
        return m_table_account.get_address().c_str();
    }

    const char * fork_name_str() const {
        return m_fork_name.c_str();
    }

private:
    base::xvaccount_t m_table_account;  // todo use xtable_address_t
    std::string m_fork_name;
    json m_json_data;
};

NS_END2