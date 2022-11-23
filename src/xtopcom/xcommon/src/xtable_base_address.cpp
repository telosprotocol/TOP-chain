// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xtable_base_address.h"

#include "xbasic/xarray_aux.h"
#include "xbasic/xerror/xerror.h"
#include "xcommon/xerror/xerror.h"

#include "xutility/xhash.h"

#include <cassert>

NS_BEG2(top, common)

xtable_base_address_t const rec_table_base_address = xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_beacon_index);
xtable_base_address_t const zec_table_base_address = xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_zec_index);
xtable_base_address_t const con_table_base_address = xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_consensus_index);
xtable_base_address_t const eth_table_base_address = xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_evm_index);
xtable_base_address_t const relay_table_base_address = xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_relay_index);
xtable_base_address_t const cross_chain_table_base_address = xtable_base_address_t::build_from(base::enum_vaccount_addr_type_relay_block, base::enum_chain_zone_relay_index);

static std::string const con_table_base_address_string{"Ta0000"};
static std::string const rec_table_base_address_string{"Ta0001"};
static std::string const zec_table_base_address_string{"Ta0002"};
static std::string const eth_table_base_address_string{"Ta0004"};
static std::string const relay_table_base_address_string{"Ta0005"};
static std::string const cross_chain_table_base_address_string{"Tb0005"};
static std::string const empty_table_base_address_string;

NS_BEG1(details)

constexpr size_t xtop_table_base_address::table_base_address_length;

xtop_table_base_address::xtop_table_base_address(base::enum_vaccount_addr_type const table_type, base::enum_xchain_zone_index const table_zone_index)
  : type_and_zone_id_{static_cast<char>(table_type), static_cast<char>(table_zone_index)} {
    assert(base::enum_chain_zone_consensus_index <= table_zone_index && table_zone_index <= base::enum_chain_zone_relay_index && table_zone_index != base::enum_chain_zone_frozen_index);
    assert(base::enum_vaccount_addr_type_block_contract == table_type || table_type == base::enum_vaccount_addr_type_relay_block);
}

xtop_table_base_address xtop_table_base_address::build_from(xstring_view_t const input, std::error_code & ec) {
    assert(!ec);
    if (input.empty()) {
        ec = error::xerrc_t::table_base_address_is_empty;
        return xtop_table_base_address{};
    }

    if (input.size() != table_base_address_length) {
        ec = error::xerrc_t::invalid_table_base_address;
        return xtop_table_base_address{};
    }

    if (input[0] != 'T' || input[2] != '0' || input[3] != '0' || input[4] != '0') {
        ec = error::xerrc_t::invalid_table_base_address;
        return xtop_table_base_address{};
    }

    switch (input[1]) {
    case base::enum_vaccount_addr_type_block_contract:
        if (input[5] > static_cast<char>(base::enum_chain_zone_relay_index + '0')) {
            ec = error::xerrc_t::invalid_table_base_address;
            break;
        }

        if (input[5] < static_cast<char>(base::enum_chain_zone_consensus_index + '0')) {
            ec = error::xerrc_t::invalid_table_base_address;
            break;
        }

        if (input[5] == static_cast<char>(base::enum_chain_zone_frozen_index + '0')) {
            ec = error::xerrc_t::invalid_table_base_address;
            break;
        }

        return xtop_table_base_address{static_cast<base::enum_vaccount_addr_type>(input[1]), static_cast<base::enum_xchain_zone_index>(input[5] - '0')};

    case base::enum_vaccount_addr_type_relay_block:
        if (input[5] != static_cast<char>(base::enum_chain_zone_relay_index + '0')) {
            ec = error::xerrc_t::invalid_table_base_address;
            break;
        }

        return xtop_table_base_address{static_cast<base::enum_vaccount_addr_type>(input[1]), static_cast<base::enum_xchain_zone_index>(input[5] - '0')};

    default:
        ec = error::xerrc_t::invalid_table_base_address;
        break;
    }

    return {};
}

xtop_table_base_address xtop_table_base_address::build_from(base::enum_vaccount_addr_type const table_type,
                                                            base::enum_xchain_zone_index const table_zone_index,
                                                            std::error_code & ec) {
    if (table_zone_index == base::enum_chain_zone_frozen_index || table_zone_index > base::enum_chain_zone_relay_index) {
        ec = error::xerrc_t::invalid_zone_index;
        return xtop_table_base_address{};
    }

    if (table_type != base::enum_vaccount_addr_type_block_contract && table_type != base::enum_vaccount_addr_type_relay_block) {
        ec = error::xerrc_t::invalid_table_type;
        return xtop_table_base_address{};
    }

    return xtop_table_base_address{table_type, table_zone_index};
}

xtop_table_base_address xtop_table_base_address::build_from(base::enum_vaccount_addr_type const table_type, base::enum_xchain_zone_index const table_zone_index) {
    std::error_code ec;
    auto const r = build_from(table_type, table_zone_index, ec);
    top::error::throw_error(ec);
    return r;
}

xtop_table_base_address xtop_table_base_address::build_from(base::enum_vaccount_addr_type const table_type, xzone_id_t const table_zone_id, std::error_code & ec) {
    assert(!ec);
    return build_from(table_type, static_cast<base::enum_xchain_zone_index>(table_zone_id.value()), ec);
}

xtop_table_base_address xtop_table_base_address::build_from(base::enum_vaccount_addr_type const table_type, xzone_id_t table_zone_id) {
    return build_from(table_type, static_cast<base::enum_xchain_zone_index>(table_zone_id.value()));
}

std::string const & xtop_table_base_address::to_string(std::error_code & ec) const {
    if (empty()) {
        ec = error::xerrc_t::table_base_address_is_empty;
        return empty_table_base_address_string;
    }

    switch (static_cast<base::enum_vaccount_addr_type>(type_and_zone_id_[0])) {  // NOLINT(clang-diagnostic-switch-enum)
    case base::enum_vaccount_addr_type_block_contract:
        switch (static_cast<base::enum_xchain_zone_index>(type_and_zone_id_[1])) {  // NOLINT(clang-diagnostic-switch-enum)
        case base::enum_chain_zone_consensus_index:
            return con_table_base_address_string;

        case base::enum_chain_zone_beacon_index:
            return rec_table_base_address_string;

        case base::enum_chain_zone_zec_index:
            return zec_table_base_address_string;

        case base::enum_chain_zone_evm_index:
            return eth_table_base_address_string;

        case base::enum_chain_zone_relay_index:
            return relay_table_base_address_string;

        default:
            ec = error::xerrc_t::invalid_zone_index;
            return empty_table_base_address_string;
        }

    case base::enum_vaccount_addr_type_relay_block:
        if (type_and_zone_id_[1] == base::enum_chain_zone_relay_index) {
            return cross_chain_table_base_address_string;
        }

        ec = error::xerrc_t::invalid_zone_index;
        return empty_table_base_address_string;

    default:
        ec = error::xerrc_t::invalid_table_type;
        return empty_table_base_address_string;
    }
}

std::string const & xtop_table_base_address::to_string() const {
    std::error_code ec;
    auto const & r = to_string(ec);
    top::error::throw_error(ec);
    return r;
}

bool xtop_table_base_address::empty() const noexcept {
    return type_and_zone_id_[0] == 0;
}

base::enum_vaccount_addr_type xtop_table_base_address::type(std::error_code & ec) const {
    assert(!ec);

    if (empty()) {
        ec = error::xerrc_t::table_base_address_is_empty;
        return base::enum_vaccount_addr_type_invalid;
    }

    return static_cast<base::enum_vaccount_addr_type>(type_and_zone_id_[0]);
}

base::enum_vaccount_addr_type xtop_table_base_address::type() const {
    std::error_code ec;
    auto const r = type(ec);
    top::error::throw_error(ec);
    return r;
}

base::enum_xchain_zone_index xtop_table_base_address::zone_index(std::error_code & ec) const {
    assert(!ec);

    if (empty()) {
        ec = error::xerrc_t::table_base_address_is_empty;
        return {};
    }

    return static_cast<base::enum_xchain_zone_index>(type_and_zone_id_[1]);
}

base::enum_xchain_zone_index xtop_table_base_address::zone_index() const {
    std::error_code ec;
    auto const r = zone_index(ec);
    top::error::throw_error(ec);
    return r;
}

xzone_id_t xtop_table_base_address::zone_id(std::error_code & ec) const {
    assert(!ec);
    return xzone_id_t{static_cast<xzone_id_t::value_type>(zone_index(ec))};
}

xzone_id_t xtop_table_base_address::zone_id() const {
    std::error_code ec;
    auto const r = zone_id(ec);
    top::error::throw_error(ec);
    return r;
}

size_t xtop_table_base_address::size() const noexcept {
    if (empty()) {
        return 0;
    }

    return table_base_address_length;
}


void xtop_table_base_address::clear() noexcept {
    type_and_zone_id_[0] = 0;
    type_and_zone_id_[1] = 0;
}

bool xtop_table_base_address::operator==(xtop_table_base_address const & other) const noexcept {
    return type_and_zone_id_ == other.type_and_zone_id_;
}

bool xtop_table_base_address::operator<(xtop_table_base_address const & other) const noexcept {
    if (type_and_zone_id_[0] != other.type_and_zone_id_[0]) {
        return type_and_zone_id_[0] < other.type_and_zone_id_[0];
    }

    return type_and_zone_id_[1] < other.type_and_zone_id_[1];
}

bool xtop_table_base_address::operator>(xtop_table_base_address const & other) const noexcept {
    return other < *this;
}

bool xtop_table_base_address::operator<=(xtop_table_base_address const & other) const noexcept {
    return !(other < *this);
}

bool xtop_table_base_address::operator>=(xtop_table_base_address const & other) const noexcept {
    return !(*this < other);
}

bool xtop_table_base_address::operator!=(xtop_table_base_address const & other) const noexcept {
    return !(*this == other);
}

char const * xtop_table_base_address::data() const noexcept {
    return type_and_zone_id_.data();
}
NS_END1

NS_END2

NS_BEG1(std)  // NOLINT(cert-dcl58-cpp)

size_t hash<top::common::xtable_base_address_t>::operator()(top::common::xtable_base_address_t const & input) const {
    return top::utl::xxh64_t::digest(input.data(), 2);
}

NS_END1
