// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xchain_error.h"

#include <string>
#include <system_error>

NS_BEG2(top, error)

void do_throw_error(std::error_code const & ec);
void do_throw_error(std::error_code const & ec, char const * extra_what);
void do_throw_error(std::error_code const & ec, std::string const & extra_what);

void throw_error(std::error_code const & ec);
void throw_error(std::error_code const ec, char const * extra_what);
void throw_error(std::error_code const ec, std::string const & extra_what);

NS_END2
