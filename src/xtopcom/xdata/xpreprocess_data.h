// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address_fwd.h"
#include "xdata/xcons_transaction.h"

#include <functional>
#include <string>
NS_BEG2(top, data)

class xmessage {
public:
    xmessage(common::xaccount_address_t account, xcons_transaction_ptr_t transaction, std::function<bool(void)> callback);
    xmessage(common::xaccount_address_t account, xcons_transaction_ptr_t transaction);
    xmessage() = default;
    ~xmessage() = default;
    common::xaccount_address_t const & account() const noexcept;
    xcons_transaction_ptr_t const & transaction() const noexcept;
    std::function<bool(void)> callback();

private:
    common::xaccount_address_t m_account;
    xcons_transaction_ptr_t m_transaction;
    std::function<bool(void)> m_callback;
};
using xmessage_t = xmessage;

class xhandler_face {
public:
    virtual ~xhandler_face() = default;
    virtual bool handle(xmessage_t & msg);
    virtual void handle(xmessage_t & msg, std::error_code & ec);
};
using xhandler_face_t = xhandler_face;

NS_END2