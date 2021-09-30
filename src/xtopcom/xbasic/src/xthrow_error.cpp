// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xchain_error.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xerror/xthrow_exception.hpp"

NS_BEG2(top, error)

void do_throw_error(std::error_code const & ec) {
    xtop_error_t eh{ ec };
    throw_exception(std::move(eh));
}

void do_throw_error(std::error_code const & ec, char const * extra_what) {
    xtop_error_t eh{ ec, extra_what };
    throw_exception(std::move(eh));
}

void do_throw_error(std::error_code const & ec, std::string const & extra_what) {
    xtop_error_t eh{ ec, extra_what };
    throw_exception(std::move(eh));
}

void throw_error(std::error_code const & ec) {
    if (ec) {
        do_throw_error(ec);
    }
}

void throw_error(std::error_code const ec, char const * extra_what) {
    if (ec) {
        do_throw_error(ec, extra_what);
    }
}

void throw_error(std::error_code const ec, std::string const & extra_what) {
    if (ec) {
        do_throw_error(ec, extra_what);
    }
}

NS_END2
