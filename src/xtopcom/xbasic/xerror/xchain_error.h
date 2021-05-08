// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <stdexcept>
#include <system_error>

NS_BEG2(top, error)

class xtop_top_error : public std::runtime_error {
    using base_t = std::runtime_error;

    std::error_code m_ec{};

public:
    xtop_top_error()                                   = default;
    xtop_top_error(xtop_top_error const &)             = default;
    xtop_top_error & operator=(xtop_top_error const &) = default;
    xtop_top_error(xtop_top_error &&)                  = default;
    xtop_top_error & operator=(xtop_top_error &&)      = default;
    ~xtop_top_error() override                         = default;

    explicit xtop_top_error(std::error_code ec);
    xtop_top_error(std::error_code ec, std::string const & extra_what);
    xtop_top_error(std::error_code ec, char const * extra_what);
    xtop_top_error(int const ev, std::error_category const & ecat);
    xtop_top_error(int const ev, std::error_category const & ecat, std::string const & extra_what);
    xtop_top_error(int const ev, std::error_category const & ecat, char const * extra_what);

    std::error_code const &
    code() const noexcept;
};
using xtop_error_t = xtop_top_error;

NS_END2
