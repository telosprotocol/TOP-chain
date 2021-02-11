// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xchain_error.h"
#include "xbasic/xthrow_error.h"
#include "xbasic/xthrow_exception.hpp"

NS_BEG1(top)

void
do_throw_error(std::error_code const & ec) {
    xchain_error_t eh{ ec };
    throw_exception(eh);
}

void
do_throw_error(std::error_code const & ec, char const * extra_what) {
    xchain_error_t eh{ ec, extra_what };
    throw_exception(eh);
}

void
do_throw_error(std::error_code const & ec, std::string const & extra_what) {
    xchain_error_t eh{ ec, extra_what };
    throw_exception(eh);
}

NS_END1
