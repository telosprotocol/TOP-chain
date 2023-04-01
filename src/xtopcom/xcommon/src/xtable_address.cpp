// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xtable_address.h"

#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xstring_utility.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xeth_address.h"
#include "xmetrics/xmetrics.h"
#include "xutility/xhash.h"

#include <cassert>
#include <string>
#include <vector>

NS_BEG3(top, common, details)

static constexpr size_t min_table_address_length{8};
static constexpr size_t max_table_address_length{9};

xtop_table_address::xtop_table_address(xtable_base_address_t const base_address, xtable_id_t const table_id) : base_address_{base_address}, assigned_table_id_{table_id} {
}

xtop_table_address xtop_table_address::build_from(std::string const & table_address_string, std::error_code & ec) {
    assert(!ec);
    return build_from(xstring_view_t{table_address_string.data(), table_address_string.size()}, ec);
}

xtop_table_address xtop_table_address::build_from(std::string const & table_address_string) {
    return build_from(xstring_view_t{table_address_string.data(), table_address_string.size()});
}

xtop_table_address xtop_table_address::build_from(xstring_view_t const table_address_string, std::error_code & ec) {
    if (table_address_string.length() < min_table_address_length || table_address_string.length() > max_table_address_length) {
        ec = error::xerrc_t::invalid_table_address;
        xwarn("build table address with invalid input %s", std::string{std::begin(table_address_string), std::end(table_address_string)}.c_str());
        return {};
    }

    auto const parts = split(table_address_string, '@');
    if (parts.size() != 2) {
        ec = error::xerrc_t::invalid_table_address;
        xwarn("build table address with invalid input %s", std::string{std::begin(table_address_string), std::end(table_address_string)}.c_str());
        return {};
    }

    if (parts[0].empty() || parts[1].empty() || parts[1].size() > 2) {  // something like Ta0000@ or @Ta0000 or Ta000@123
        ec = error::xerrc_t::invalid_table_address;
        xwarn("build table address with invalid input %s", std::string{std::begin(table_address_string), std::end(table_address_string)}.c_str());
        return {};
    }

    auto const table_id_value = std::stoull(std::string{std::begin(parts[1]), std::end(parts[1])});
    if (table_id_value != static_cast<uint16_t>(table_id_value)) {
        ec = error::xerrc_t::invalid_table_id;
        xwarn("build table address with invalid input %s", std::string{std::begin(table_address_string), std::end(table_address_string)}.c_str());
        return {};
    }

    auto const table_id = xtable_id_t{static_cast<uint16_t>(table_id_value)};
    auto const table_base_address = xtable_base_address_t::build_from(parts[0], ec);
    if (ec) {
        xwarn("build table address with invalid input %s", std::string{std::begin(table_address_string), std::end(table_address_string)}.c_str());
        return {};
    }

    return xtop_table_address{table_base_address, table_id};
}

xtop_table_address xtop_table_address::build_from(xstring_view_t const table_address_string) {
    std::error_code ec;
    auto const r = build_from(table_address_string, ec);
    top::error::throw_error(ec);
    return r;
}

xtop_table_address xtop_table_address::build_from(xtable_base_address_t const table_base_address, xtable_id_t const table_id, std::error_code & ec) {
    assert(!ec);
    if (table_base_address.empty()) {
        ec = error::xerrc_t::table_base_address_is_empty;
        return {};
    }

    return xtop_table_address{table_base_address, table_id};
}

xtop_table_address xtop_table_address::build_from(xtable_base_address_t table_base_address, xtable_id_t table_id) {
    std::error_code ec;
    auto r = build_from(table_base_address, table_id, ec);
    top::error::throw_error(ec);
    return r;
}

bool xtop_table_address::empty() const noexcept {
    return base_address_.empty() || assigned_table_id_.empty();
}

bool xtop_table_address::has_value() const noexcept {
    return !empty();
}

xtable_base_address_t xtop_table_address::base_address() const noexcept {
    return base_address_;
}

uint64_t xtop_table_address::hash() const {
    if (has_value()) {
        // auto const & account_string = to_string();
        utl::xxh64_t hasher;
        hasher.update(base_address_.type_and_zone_id_.data(), base_address_.type_and_zone_id_.size());
        auto const table_id_value = assigned_table_id_.value();
        hasher.update(std::addressof(table_id_value), sizeof(table_id_value));

        return hasher.get_hash();
    }

    return 0;
}

std::string xtop_table_address::to_string() const {
    if (empty()) {
        return {};
    }

    return base_address_.to_string() + '@' + top::to_string(assigned_table_id_);
}

void xtop_table_address::clear() {
    base_address_.clear();
    assigned_table_id_.clear();
}

bool
xtop_table_address::operator==(xtop_table_address const & other) const noexcept {
    return base_address_ == other.base_address_ && assigned_table_id_ == other.assigned_table_id_;
}

bool xtop_table_address::operator<(xtop_table_address const & other) const noexcept {
    if (base_address_ != other.base_address_) {
        return base_address_ < other.base_address_;
    }

    return assigned_table_id_ < other.assigned_table_id_;
}

bool xtop_table_address::operator>(xtop_table_address const & other) const noexcept {
    return other < *this;
}

bool xtop_table_address::operator!=(xtop_table_address const & other) const noexcept {
    return !(*this == other);
}

bool xtop_table_address::operator>=(xtop_table_address const & other) const noexcept {
    return !(*this < other);
}

bool xtop_table_address::operator<=(xtop_table_address const & other) const noexcept {
    return !(*this > other);
}

std::size_t xtop_table_address::length() const noexcept {
    if (empty()) {
        return 0;
    }

    return base_address_.size() + 1 + top::to_string(assigned_table_id_).size();
}

std::size_t xtop_table_address::size() const noexcept {
    return length();
}

base::enum_vaccount_addr_type xtop_table_address::type() const {
    return base_address_.type();
}

base::enum_vaccount_addr_type xtop_table_address::type(std::error_code & ec) const {
    assert(!ec);
    return base_address_.type(ec);
}

xtable_id_t const & xtop_table_address::table_id() const noexcept {
    return assigned_table_id_;
}

base::xvaccount_t xtop_table_address::vaccount() const {
    return base::xvaccount_t{to_string()};
}

int32_t xtop_table_address::serialize_to(base::xstream_t & stream) const {
    return do_write(stream);
}

int32_t xtop_table_address::serialize_from(base::xstream_t & stream) {
    return do_read(stream);
}

int32_t xtop_table_address::serialize_to(base::xbuffer_t & buffer) const {
    return buffer << to_string();
}

int32_t xtop_table_address::serialize_from(base::xbuffer_t & buffer) {
    std::string account_string;
    auto const r = buffer >> account_string;
    *this = build_from(xstring_view_t{account_string.data(), account_string.size()});
    return r;
}

std::int32_t xtop_table_address::do_read(base::xstream_t & stream) {
    auto const begin_size = stream.size();
    std::string account_string;
    stream >> account_string;
    *this = build_from(xstring_view_t{account_string.data(), account_string.size()});
    return begin_size - stream.size();
}

std::int32_t xtop_table_address::do_write(base::xstream_t & stream) const {
    auto const begin_size = stream.size();
    stream << to_string();
    return stream.size() - begin_size;
}

NS_END3

NS_BEG2(top, common)

std::int32_t operator <<(top::base::xstream_t & stream, xtable_address_t const & table_address) {
    return table_address.serialize_to(stream);
}

std::int32_t operator >>(top::base::xstream_t & stream, xtable_address_t & table_address) {
    return table_address.serialize_from(stream);
}

std::int32_t operator<<(top::base::xbuffer_t & buffer, xtable_address_t const & table_address) {
    return table_address.serialize_to(buffer);
}

std::int32_t operator>>(top::base::xbuffer_t & buffer, xtable_address_t & table_address) {
    return table_address.serialize_from(buffer);
}

base::enum_xchain_zone_index zone_index(xtable_address_t const table_address, std::error_code & ec) {
    assert(!ec);
    return table_address.base_address().zone_index(ec);
}

base::enum_xchain_zone_index zone_index(xtable_address_t const table_address) {
    return table_address.base_address().zone_index();
}

xzone_id_t zone_id(xtable_address_t const table_address, std::error_code & ec) {
    return table_address.base_address().zone_id(ec);
}

xzone_id_t zone_id(xtable_address_t const table_address) {
    return table_address.base_address().zone_id();
}

xtable_id_t table_id(xtable_address_t const table_address) {
    return table_address.table_id();
}

NS_END2

NS_BEG1(std)

std::size_t hash<top::common::xtable_address_t>::operator()(top::common::xtable_address_t const & table_address) const noexcept {
    return table_address.hash();
}

NS_END1

NS_BEG1(top)

template <>
xbytes_t to_bytes<common::xtable_address_t>(common::xtable_address_t const & input) {
    auto const & string = input.to_string();
    return {string.begin(), string.end()};
}

template <>
std::string to_string<common::xtable_address_t>(common::xtable_address_t const & input) {
    return input.to_string();
}

NS_END1
