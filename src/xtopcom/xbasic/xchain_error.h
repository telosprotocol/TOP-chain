// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xns_macro.h"

#include <stdexcept>
#include <system_error>

NS_BEG1(top)

class xtop_chain_error : public std::runtime_error {
    using base_t = std::runtime_error;

    std::error_code m_ec{};

public:
    xtop_chain_error()                                     = default;
    xtop_chain_error(xtop_chain_error const &)             = default;
    xtop_chain_error & operator=(xtop_chain_error const &) = default;
    xtop_chain_error(xtop_chain_error &&)                  = default;
    xtop_chain_error & operator=(xtop_chain_error &&)      = default;
    ~xtop_chain_error() override                           = default;

    explicit
    xtop_chain_error(std::error_code ec);
    xtop_chain_error(std::error_code ec, std::string const & extra_what);
    xtop_chain_error(std::error_code ec, char const * extra_what);
    xtop_chain_error(int const ev, std::error_category const & ecat);
    xtop_chain_error(int const ev, std::error_category const & ecat, std::string const & extra_what);
    xtop_chain_error(int const ev, std::error_category const & ecat, char const * extra_what);

    std::error_code const &
    code() const noexcept;
};
using xchain_error_t = xtop_chain_error;

NS_END1
