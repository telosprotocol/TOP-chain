// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xvm_error_code.h"
#include "xvm_error_category.h"
NS_BEG2(top, xvm)

std::error_code make_error_code(const enum_xvm_error_code errc) noexcept
{
    return std::error_code(static_cast<int>(errc), xvm_get_category());
}

std::error_condition make_error_condition(const enum_xvm_error_code errc) noexcept {
    return std::error_condition(static_cast<int>(errc), xvm_get_category());
}

NS_END2
