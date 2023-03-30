// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "xcommon/xaddress.h"

#include <mutex>

namespace top {
namespace genesis {

class xgenesis_accounts_mpt_t {
public:
    static xgenesis_accounts_mpt_t & instance();

    void    add_account(common::xaccount_address_t const& address);
    std::set<common::xaccount_address_t>    get_not_in_mpt_accounts() const;
    void    delete_in_mpt_accounts(common::xaccount_address_t const& address);
    void    clear();

private:
    mutable std::mutex m_mutex;
    std::set<common::xaccount_address_t>    m_not_in_mpt_accounts;
};


}  // namespace genesis
}  // namespace top
