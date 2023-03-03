// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xcommon/xtoken.h"

#include "xbasic/xerror/xerror.h"
#include "xcommon/xerror/xerror.h"
#include "xcontract_common/xerror/xerror.h"
#include "xcommon/common_data.h"
#include "xutility/xhash.h"

#include <cassert>
#include <memory>

NS_BEG2(top, common)

xtop_token::xtop_token(xtop_token && other) noexcept : amount_{ other.amount_ }, symbol_{ std::move(other.symbol_) } {
    other.amount_ = 0;
}

xtop_token & xtop_token::operator=(xtop_token && other) {
    if (amount_ != 0) {
        top::error::throw_error(common::error::xerrc_t::token_not_used);
    }

    assert(amount_ == 0);

    amount_ = other.amount_;
    symbol_ = std::move(other.symbol_);

    other.amount_ = 0;

    return *this;
}


xtop_token::xtop_token(common::xsymbol_t symbol) : symbol_{ std::move(symbol) } {
}

xtop_token::xtop_token(evm_common::u256 const amount, common::xsymbol_t symbol) : amount_{ amount }, symbol_ { std::move(symbol) } {
}

xtop_token::xtop_token(evm_common::u256 const amount) : amount_{amount} {
}

xtop_token::~xtop_token() noexcept {
    assert(amount_ == 0);
}

bool xtop_token::operator==(xtop_token const & other) const noexcept {
    if (symbol() != other.symbol()) {
        return false;
    }

    if (invalid()) {
        assert(other.invalid());
        return true;
    }

    return amount_ == other.amount_;
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
        top::error::throw_error(error::xerrc_t::token_symbol_not_matched, "xtoken_t::operator<=>(xtoken_t const & other)");
    }

    return amount_ < other.amount_;
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

    amount_ += other.amount_;
    other.amount_ = 0;

    return *this;
}

bool xtop_token::invalid() const noexcept {
    return symbol_.empty();
}



evm_common::u256 xtop_token::amount() const noexcept {
    return amount_;
}

common::xsymbol_t const & xtop_token::symbol() const noexcept {
    return symbol_;
}

void xtop_token::clear() noexcept {
    amount_ = 0;
    symbol_.clear();
}

void xtop_token::move_to(base::xstream_t& stream) noexcept {
    serialize_to(stream);
    clear();  // move serialize, clear token
}
void xtop_token::move_from(base::xstream_t& stream) noexcept {
    serialize_from(stream);
    stream.reset();
}

std::int32_t xtop_token::serialize_to(base::xstream_t & stream) const {
    return do_write(stream);
}

std::int32_t xtop_token::serialize_from(base::xstream_t & stream) {
    return do_read(stream);
}

std::int32_t xtop_token::serialize_to(base::xbuffer_t & buffer) const {
    auto const size = buffer.size();
    buffer << evm_common::toBigEndianString(amount_);
    buffer << symbol_;
    return buffer.size() - size;
}

std::int32_t xtop_token::serialize_from(base::xbuffer_t & buffer) {
    auto const size = buffer.size();
    std::string amount;
    buffer >> amount;
    buffer >> symbol_;
    amount_ = evm_common::fromBigEndian<evm_common::u256>(amount);
    return size - buffer.size();
}

int32_t xtop_token::do_read(base::xstream_t& stream) {
    auto const size = stream.size();
    std::string amount;
    stream >> amount;
    stream >> symbol_;
    amount_ = evm_common::fromBigEndian<evm_common::u256>(amount);
    return size - stream.size();
}

int32_t xtop_token::do_write(base::xstream_t& stream) const {
    auto const size = stream.size();
    stream.operator<<(evm_common::toBigEndianString(amount_));
    stream << symbol_;
    return stream.size() - size;
}

NS_END2

NS_BEG1(std)

size_t hash<top::common::xtoken_t>::operator()(top::common::xtoken_t const & amount) const noexcept {
    top::utl::xxh64_t xxhash;
    auto const value = std::hash<top::evm_common::u256>{}(amount.amount());

    xxhash.update(std::addressof(value), sizeof(value));
    xxhash.update(amount.symbol().to_string());

    return static_cast<size_t>(xxhash.get_hash());
}

NS_END1
