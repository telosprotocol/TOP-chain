// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xstate_accessor/xtoken.h"

#include "xbasic/xerror/xthrow_error.h"
#include "xstate_accessor/xerror/xerror.h"
#include "xutility/xhash.h"

#include <cassert>
#include <memory>

namespace top {
namespace state_accessor {

xtop_token::xtop_token(xtop_token && other) noexcept : value_{ other.value_ }, symbol_{ std::move(other.symbol_) } {
    other.value_ = 0;
}

xtop_token::xtop_token(std::string symbol) noexcept : symbol_{ std::move(symbol) } {
}

xtop_token::xtop_token(std::uint64_t const amount, std::string symbol) noexcept : value_{ amount }, symbol_ { std::move(symbol) } {
}

xtop_token::~xtop_token() noexcept {
    assert(value_ == 0);
    assert(symbol_.empty());
}

bool xtop_token::operator==(xtop_token const & other) const noexcept {
    if (symbol() != other.symbol()) {
        return false;
    }

    if (invalid()) {
        assert(other.invalid());
        return true;
    }

    return value_ == other.value_;
}

bool xtop_token::operator!=(xtop_token const & other) const noexcept {
    return !(*this == other);
}

bool xtop_token::operator<(xtop_token const & other) const {
    if (other.invalid()) {
        return false;
    }

    if (invalid()) {
        return true;
    }

    if (symbol() != other.symbol()) {
        std::error_code ec{ error::xerrc_t::invalid_property_type };
        top::error::throw_error(ec, "xtoken_t::operator<=>(xtoken_t const & other)");
    }

    return value_ < other.value_;
}

bool xtop_token::operator>(xtop_token const & other) const {
    return other < *this;
}

bool xtop_token::operator<=(xtop_token const & other) const {
    return !(*this > other);
}

bool xtop_token::operator>=(xtop_token const & other) const {
    return !(*this < other);
}

xtop_token & xtop_token::operator+=(xtop_token & other) noexcept {
    if (std::addressof(other) == std::addressof(*this)) {
        return *this;
    }

    if (symbol() != other.symbol()) {
        return *this;
    }

    if (invalid()) {
        assert(other.invalid());
        return *this;
    }

    value_ += other.value_;
    other.value_ = 0;

    return *this;
}

bool xtop_token::invalid() const noexcept {
    return symbol_.empty();
}

uint64_t xtop_token::value() const noexcept {
    return value_;
}

std::string const & xtop_token::symbol() const noexcept {
    return symbol_;
}

void xtop_token::clear() noexcept {
    value_ = 0;
    symbol_.clear();
}

}
}

namespace std {

size_t hash<top::state_accessor::xtoken_t>::operator()(top::state_accessor::xtoken_t const & amount) const noexcept {
    top::utl::xxh64_t xxhash;
    auto const value = amount.value();

    xxhash.update(std::addressof(value), sizeof(value));
    xxhash.update(amount.symbol());

    return static_cast<size_t>(xxhash.get_hash());
}

}
