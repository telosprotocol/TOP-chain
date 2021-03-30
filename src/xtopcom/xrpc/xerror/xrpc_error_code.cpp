// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xrpc_error_code.h"
#include "xrpc_error_category.h"
NS_BEG2(top, xrpc)

std::error_code make_error_code(const enum_xrpc_error_code errc) noexcept
{
    return std::error_code(static_cast<int>(errc), xrpc_get_category());
}

NS_END2
