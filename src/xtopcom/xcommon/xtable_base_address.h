// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xstring_view.h"
#include "xcommon/xtable_base_address_fwd.h"
#include "xcommon/xip.h"
#include "xvledger/xvaccount.h"
#include "xcommon/xtable_address_fwd.h"

#include <array>
#include <string>
#include <system_error>

NS_BEG2(top, common)

extern xtable_base_address_t const rec_table_base_address;
extern xtable_base_address_t const zec_table_base_address;
extern xtable_base_address_t const con_table_base_address;
extern xtable_base_address_t const eth_table_base_address;
extern xtable_base_address_t const relay_table_base_address;
extern xtable_base_address_t const cross_chain_table_base_address;

NS_BEG1(details)
class xtop_table_base_address {
    std::array<char, 2> type_and_zone_id_{{0, 0}};

    friend struct std::hash<xtop_table_base_address>;
    friend xtable_address_t;

public:
    constexpr xtop_table_base_address() = default;
    xtop_table_base_address(xtop_table_base_address const &) = default;
    xtop_table_base_address & operator=(xtop_table_base_address const &) = default;
    xtop_table_base_address(xtop_table_base_address &&) = default;
    xtop_table_base_address & operator=(xtop_table_base_address &&) = default;
    ~xtop_table_base_address() = default;

private:
    explicit xtop_table_base_address(base::enum_vaccount_addr_type table_type, base::enum_xchain_zone_index table_zone_index);

    char const * data() const noexcept;

public:
    static constexpr size_t table_base_address_length{6};

    static xtop_table_base_address build_from(xstring_view_t input, std::error_code & ec);
    static xtop_table_base_address build_from(base::enum_vaccount_addr_type table_type, base::enum_xchain_zone_index table_zone_index, std::error_code & ec);
    static xtop_table_base_address build_from(base::enum_vaccount_addr_type table_type, base::enum_xchain_zone_index table_zone_index);
    static xtop_table_base_address build_from(base::enum_vaccount_addr_type table_type, xzone_id_t table_zone_id, std::error_code & ec);
    static xtop_table_base_address build_from(base::enum_vaccount_addr_type table_type, xzone_id_t table_zone_id);

    std::string const & to_string(std::error_code & ec) const;
    std::string const & to_string() const;

    bool empty() const noexcept;

    base::enum_vaccount_addr_type type(std::error_code & ec) const;
    base::enum_vaccount_addr_type type() const;
    base::enum_xchain_zone_index zone_index(std::error_code & ec) const;
    base::enum_xchain_zone_index zone_index() const;

    xzone_id_t zone_id(std::error_code & ec) const;
    xzone_id_t zone_id() const;

    size_t size() const noexcept;

    void clear() noexcept;

    bool operator==(xtop_table_base_address const & other) const noexcept;
    bool operator<(xtop_table_base_address const & other) const noexcept;
    bool operator>(xtop_table_base_address const & other) const noexcept;
    bool operator<=(xtop_table_base_address const & other) const noexcept;
    bool operator>=(xtop_table_base_address const & other) const noexcept;
    bool operator!=(xtop_table_base_address const & other) const noexcept;
};
NS_END1

using xtable_base_address_t = details::xtop_table_base_address;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xtable_base_address_t> {
    size_t operator()(top::common::xtable_base_address_t const & input) const;
};

NS_END1
