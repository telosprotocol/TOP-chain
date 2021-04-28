// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xchain_error.h"
#include <string>

NS_BEG2(top, error)

xtop_chain_error::xtop_chain_error(std::error_code ec)
    : base_t{ ec.message() }, m_ec{ std::move(ec) } {
}

xtop_chain_error::xtop_chain_error(std::error_code ec, char const * extra_what)
    : base_t{ ec.message() + ": " + extra_what }, m_ec{ std::move(ec) } {
}

xtop_chain_error::xtop_chain_error(std::error_code ec, std::string const & extra_what)
    : base_t{ ec.message() + ": " + extra_what}, m_ec{ std::move(ec) } {
}

xtop_chain_error::xtop_chain_error(int const ev, std::error_category const & ecat)
    : base_t{ std::error_code{ ev, ecat }.message() }, m_ec{ ev, ecat } {
}

xtop_chain_error::xtop_chain_error(int const ev, std::error_category const & ecat, char const * extra_what)
    : base_t{ std::error_code{ ev, ecat}.message() + ": " + extra_what }, m_ec{ ev, ecat } {
}

xtop_chain_error::xtop_chain_error(int const ev, std::error_category const & ecat, std::string const & extra_what)
    : base_t{ std::error_code{ ev, ecat}.message() + ": " + extra_what }, m_ec{ ev, ecat } {
}

std::error_code const & xtop_chain_error::code() const noexcept {
    return m_ec;
}

NS_END2
