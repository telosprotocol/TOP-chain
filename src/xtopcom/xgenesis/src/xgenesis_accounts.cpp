// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgenesis/xgenesis_accounts.h"

namespace top {
namespace genesis {

xgenesis_accounts_mpt_t & xgenesis_accounts_mpt_t::instance() {
    static xgenesis_accounts_mpt_t _instance;
    return _instance;
}

void xgenesis_accounts_mpt_t::add_account(common::xaccount_address_t const& address) {
    std::lock_guard<std::mutex> _l(m_mutex);
    if (address.vaccount().is_unit_address()) { // only add unit type address
        m_not_in_mpt_accounts.insert(address);
        xinfo("xgenesis_accounts_mpt_t::add_account account=%s,size=%zu",address.to_string().c_str(),m_not_in_mpt_accounts.size());
    }
}
std::set<common::xaccount_address_t> xgenesis_accounts_mpt_t::get_not_in_mpt_accounts() const {
    std::lock_guard<std::mutex> _l(m_mutex);
    return m_not_in_mpt_accounts;
}
void xgenesis_accounts_mpt_t::delete_in_mpt_accounts(common::xaccount_address_t const& address) {
    std::lock_guard<std::mutex> _l(m_mutex);
    m_not_in_mpt_accounts.erase(address);
    xinfo("xgenesis_accounts_mpt_t::delete_in_mpt_accounts account=%s,size=%zu",address.to_string().c_str(),m_not_in_mpt_accounts.size());
}

void xgenesis_accounts_mpt_t::clear() {
    std::lock_guard<std::mutex> _l(m_mutex);
    m_not_in_mpt_accounts.clear();
}

}  // namespace genesis
}  // namespace top