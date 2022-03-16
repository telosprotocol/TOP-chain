// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xsymbol.h"

NS_BEG2(top, common)

xsymbol_t const SYMBOL_TOP_TOKEN{""};

xtop_symbol::xtop_symbol(std::string symbol) noexcept : symbol_{std::move(symbol)} {
}

void xtop_symbol::swap(xtop_symbol & other) noexcept {
    symbol_.swap(other.symbol_);
}

bool xtop_symbol::empty() const noexcept {
    return symbol_.empty();
}

void xtop_symbol::clear() noexcept {
    symbol_.clear();
}

std::string const & xtop_symbol::to_string() const noexcept {
    return symbol_;
}

char const * xtop_symbol::c_str() const noexcept {
    return symbol_.c_str();
}

size_t xtop_symbol::length() const noexcept {
    return symbol_.length();
}

bool xtop_symbol::operator==(xtop_symbol const & other) const noexcept {
    return symbol_ == other.symbol_;
}

bool xtop_symbol::operator<(xtop_symbol const & other) const noexcept {
    return symbol_ < other.symbol_;
}

bool xtop_symbol::operator!=(xtop_symbol const & other) const noexcept {
    return symbol_ != other.symbol_;
}

bool xtop_symbol::operator<=(xtop_symbol const & other) const noexcept {
    return !(*this > other);
}

bool xtop_symbol::operator>(xtop_symbol const & other) const noexcept {
    return other < *this;
}

bool xtop_symbol::operator>=(xtop_symbol const & other) const noexcept {
    return !(*this < other);
}

std::int32_t xtop_symbol::serialize_to(base::xstream_t & stream) const {
    return do_write(stream);
}

std::int32_t xtop_symbol::serialize_to(base::xbuffer_t & buffer) const {
    return buffer << symbol_;
}

std::int32_t xtop_symbol::serialize_from(base::xstream_t & stream) {
    return do_read(stream);
}

std::int32_t xtop_symbol::serialize_from(base::xbuffer_t & buffer) {
    return buffer >> symbol_;
}

std::int32_t xtop_symbol::do_read(base::xstream_t & stream) {
    auto const size_begin = stream.size();
    stream >> symbol_;
    return size_begin - stream.size();
}

std::int32_t xtop_symbol::do_write(base::xstream_t & stream) const {
    auto const size_begin = stream.size();
    // stream << this->symbol_;
    stream.operator<<(symbol_);
    return stream.size() - size_begin;
}

std::int32_t operator<<(top::base::xstream_t & stream, xtop_symbol const & symbol) {
    return symbol.serialize_to(stream);
}

std::int32_t operator>>(top::base::xstream_t & stream, xtop_symbol & symbol) {
    return symbol.serialize_from(stream);
}

std::int32_t operator<<(top::base::xbuffer_t & buffer, xtop_symbol const & symbol) {
    return symbol.serialize_to(buffer);
}
std::int32_t operator>>(top::base::xbuffer_t & buffer, xtop_symbol & symbol) {
    return symbol.serialize_from(buffer);
}

NS_END2

NS_BEG1(std)

std::size_t hash<top::common::xsymbol_t>::operator()(top::common::xsymbol_t const & symbol) const noexcept {
    return std::hash<std::string>{}(symbol.to_string());
}

NS_END1
