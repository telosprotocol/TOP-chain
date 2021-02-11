// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <stdexcept>
#include <string>
#include "xrpc_error_code.h"
NS_BEG2(top, xrpc)
class xrpc_error final : public std::runtime_error
{
public:
    xrpc_error(const enum_xrpc_error_code error_code);
    xrpc_error(const enum_xrpc_error_code error_code, const std::string& message);

    std::error_code const & code() const noexcept;

    char const * what() const noexcept override;

private:
    xrpc_error(std::error_code const & ec);
    xrpc_error(std::error_code const & ec, const std::string& message);
private:
    std::error_code m_ec;
};
NS_END2


