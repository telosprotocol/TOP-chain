// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate_accessor/xerror/xerror.h"

#include <cstdint>
#include <system_error>

namespace top {
namespace state_accessor {

enum class xenum_token_type {
    invalid,
    TOP,
    VPN
};
using xtoken_type_t = xenum_token_type;

class xtop_token {
    uint64_t value_{ 0 };
    std::string symbol_{};

public:
    xtop_token() = default;
    xtop_token(xtop_token const &) = delete;
    xtop_token & operator=(xtop_token const &) = delete;
    xtop_token & operator=(xtop_token &&) = delete;

    xtop_token(xtop_token && other) noexcept;
    explicit xtop_token(std::string symbol) noexcept;
    ~xtop_token() noexcept;

    xtop_token(std::uint64_t const amount, std::string symbol) noexcept;

public:

    bool operator==(xtop_token const & other) const noexcept;
    bool operator!=(xtop_token const & other) const noexcept;
    bool operator<(xtop_token const & other) const;
    bool operator>(xtop_token const & other) const;
    bool operator<=(xtop_token const & other) const;
    bool operator>=(xtop_token const & other) const;
    xtop_token & operator+=(xtop_token & other) noexcept;

    bool invalid() const noexcept;
    uint64_t value() const noexcept;
    std::string const & symbol() const noexcept;
    void clear() noexcept;
};
using xtoken_t = xtop_token;

}
}

namespace std {

template <>
struct hash<top::state_accessor::xtoken_t> {
    size_t operator()(top::state_accessor::xtoken_t const & amount) const noexcept;
};

}
