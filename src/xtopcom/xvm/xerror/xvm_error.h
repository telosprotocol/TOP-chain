// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <stdexcept>
#include <string>
#include "xvm_error_code.h"
NS_BEG2(top, xvm)
class xvm_error final : public std::runtime_error
{
public:
    xvm_error(const enum_xvm_error_code error_code);
    xvm_error(const enum_xvm_error_code error_code, const std::string& message);

    std::error_code const & code() const noexcept;

    char const * what() const noexcept override;

private:
    xvm_error(std::error_code const & ec);
    xvm_error(std::error_code const & ec, const std::string& message);
private:
    std::error_code m_ec;
};
NS_END2


