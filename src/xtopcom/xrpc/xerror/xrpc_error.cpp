// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xrpc_error.h"

#include "xrpc/xerror/xrpc_error_code.h"

NS_BEG2(top, xrpc)
using std::string;

xrpc_error::xrpc_error(const enum_xrpc_error_code error_code)
    :xrpc_error{make_error_code(error_code)}
{
}

xrpc_error::xrpc_error(const enum_xrpc_error_code error_code, const string& message)
    :xrpc_error{make_error_code(error_code), message}
{
}

xrpc_error::xrpc_error(std::error_code const & ec)
    :std::runtime_error{ec.message()},m_ec(ec)
{
}

xrpc_error::xrpc_error(std::error_code const & ec, const std::string& message)
    :std::runtime_error{message},m_ec(ec)
{
}

const std::error_code & xrpc_error::code() const noexcept {
    return m_ec;
}

const char * xrpc_error::what() const noexcept {
    return std::runtime_error::what();
}
NS_END2
