// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xpreprocess_data.h"

NS_BEG2(top, data)

xmessage::xmessage(common::xaccount_address_t account, xcons_transaction_ptr_t transaction, std::function<bool(void)> callback)
  : m_account(account), m_transaction(transaction), m_callback(callback) {
}

xmessage::xmessage(common::xaccount_address_t account, xcons_transaction_ptr_t transaction) : m_account(account), m_transaction(transaction) {
}

common::xaccount_address_t const & xmessage::account() const noexcept {
    return m_account;
}

xcons_transaction_ptr_t const & xmessage::transaction() const noexcept {
    return m_transaction;
}

std::function<bool(void)> xmessage::callback() {
    return m_callback;
}

bool xhandler_face::handle(xmessage_t & msg) {
    xwarn("xhandler_face::handle default impl");
    return false;
}

void xhandler_face::handle(xmessage_t & msg, std::error_code & ec) {
    xwarn("xhandler_face::handle with error_code default impl");
}

NS_END2