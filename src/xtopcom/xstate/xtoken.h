// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate/xerror/xerror.h"

#include <cstdint>
#include <memory>
#include <system_error>

NS_BEG2(top, state)

enum class xenum_token_type {
    invalid,
    TOP,
    VPN
};
using xtoken_type_t = xenum_token_type;

template <xtoken_type_t TypeV>
class xtop_token {
    uint64_t value_{ 0 };

public:
    xtop_token() = default;
    xtop_token(xtop_token const &) = delete;
    xtop_token & operator=(xtop_token const &) = delete;
    xtop_token(xtop_token && other) noexcept : value_{ other.value_ } {
        other.value_ = 0;
    }
    xtop_token & operator=(xtop_token &&) = delete;
    ~xtop_token() = default;

protected:
    xtop_token(uint64_t const amount) noexcept : value_{ amount } {
    }

public:

    xtop_token withdraw(uint64_t const amount, std::error_code & ec) {
        if (value_ < amount) {
            ec = error::xerrc_t::token_insufficient;
            return;
        }

        value_ -= amount;
        return xtop_token{ amount };
    }

    void depoist(xtop_token & amount, std::error_code & ec) {
        *this += amount;
        return *this;
    }

    bool operator<(xtop_token const & other) const noexcept {
        return value_ < other.value_;
    }

    bool operator==(xtop_token const & other) const noexcept {
        return value_ == other.value_;
    }

    bool operator>(xtop_token const & other) const noexcept {
        return other < *this;
    }

    bool operator!=(xtop_token const & other) const noexcept {
        return !(*this == other);
    }

    bool operator<=(xtop_token const & other) const noexcept {
        return !(*this > other);
    }

    bool operator>=(xtop_token const & other) const noexcept {
        return !(*this < other);
    }

    xtop_token & operator+=(xtop_token & other) noexcept {
        if (std::addressof(other) == std::addressof(*this)) {
            return *this;
        }

        value_ += other.value_;
        other.value_ = 0;
    }
};

template <char const * SYMBOL>
using xtoken_t = xtop_token;

NS_END2
