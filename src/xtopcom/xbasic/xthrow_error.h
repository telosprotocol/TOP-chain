// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xns_macro.h"

#include <string>
#include <system_error>

NS_BEG1(top)

void
do_throw_error(std::error_code const & ec);

void
do_throw_error(std::error_code const & ec, char const * extra_what);

void
do_throw_error(std::error_code const & ec, std::string const & extra_what);

inline
void
throw_error(std::error_code const & ec) {
    if (ec) {
        do_throw_error(ec);
    }
}

inline
void
throw_error(std::error_code const ec, char const * extra_what) {
    if (ec) {
        do_throw_error(ec, extra_what);
    }
}

inline
void
throw_error(std::error_code const ec, std::string const & extra_what) {
    if (ec) {
        do_throw_error(ec, extra_what);
    }
}

NS_END1
