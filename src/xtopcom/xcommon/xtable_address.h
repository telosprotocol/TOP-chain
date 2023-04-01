// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xmem.h"
#include "xvledger/xvaccount.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xstring.h"
#include "xbasic/xstring_view.h"
#include "xcommon/xtable_address_fwd.h"
#include "xcommon/xtable_base_address.h"
#include "xcommon/xtable_id.h"

#include <cstdint>
#include <string>

NS_BEG2(top, common)

std::int32_t operator<<(top::base::xstream_t & stream, xtable_address_t const & table_address);
std::int32_t operator>>(top::base::xstream_t & stream, xtable_address_t & table_address);
std::int32_t operator<<(top::base::xbuffer_t & buffer, xtable_address_t const & table_address);
std::int32_t operator>>(top::base::xbuffer_t & buffer, xtable_address_t & table_address);

NS_BEG1(details)

class xtop_table_address final {
private:
    xtable_base_address_t base_address_{};
    xtable_id_t assigned_table_id_;

public:
    xtop_table_address() = default;
    xtop_table_address(xtop_table_address const &) = default;
    xtop_table_address & operator=(xtop_table_address const &) = default;
    xtop_table_address(xtop_table_address &&) = default;
    xtop_table_address & operator=(xtop_table_address &&) = default;
    ~xtop_table_address() = default;

private:
    explicit xtop_table_address(xtable_base_address_t base_address, xtable_id_t table_id);

public:
    static xtop_table_address build_from(std::string const & table_address_string, std::error_code & ec);
    static xtop_table_address build_from(std::string const & table_address_string);
    static xtop_table_address build_from(xstring_view_t table_address_string, std::error_code & ec);
    static xtop_table_address build_from(xstring_view_t table_address_string);
    static xtop_table_address build_from(xtable_base_address_t table_base_address, xtable_id_t table_id, std::error_code & ec);
    static xtop_table_address build_from(xtable_base_address_t table_base_address, xtable_id_t table_id);

    bool empty() const noexcept;
    bool has_value() const noexcept;
    xtable_base_address_t base_address() const noexcept;
    uint64_t hash() const;
    std::string to_string() const;
    void clear();

    bool operator==(xtop_table_address const & other) const noexcept;
    bool operator<(xtop_table_address const & other) const noexcept;
    bool operator>(xtop_table_address const & other) const noexcept;
    bool operator!=(xtop_table_address const & other) const noexcept;
    bool operator>=(xtop_table_address const & other) const noexcept;
    bool operator<=(xtop_table_address const & other) const noexcept;

    std::size_t length() const noexcept;
    std::size_t size() const noexcept;

    base::enum_vaccount_addr_type type(std::error_code & ec) const;
    base::enum_vaccount_addr_type type() const;

    //xaccount_id_t const & account_id() const noexcept;

    //xledger_id_t const & ledger_id() const noexcept;
    xtable_id_t const & table_id() const noexcept;

    base::xvaccount_t vaccount() const;

    friend std::int32_t operator<<(base::xstream_t & stream, xtable_address_t const & table_address);
    friend std::int32_t operator>>(base::xstream_t & stream, xtable_address_t & table_address);
    friend std::int32_t operator<<(base::xbuffer_t & buffer, xtable_address_t const & table_address);
    friend std::int32_t operator>>(base::xbuffer_t & buffer, xtable_address_t & table_address);

    std::int32_t serialize_to(base::xstream_t & stream) const;
    std::int32_t serialize_from(base::xstream_t & stream);
    std::int32_t serialize_to(base::xbuffer_t & buffer) const;
    std::int32_t serialize_from(base::xbuffer_t & buffer);

private:
    std::int32_t do_read(base::xstream_t & stream);

    std::int32_t do_write(base::xstream_t & stream) const;
};

NS_END1

using xtable_address_t = details::xtop_table_address;

base::enum_xchain_zone_index zone_index(xtable_address_t table_address, std::error_code & ec);
base::enum_xchain_zone_index zone_index(xtable_address_t table_address);

xzone_id_t zone_id(xtable_address_t table_address, std::error_code & ec);
xzone_id_t zone_id(xtable_address_t table_address);
xtable_id_t table_id(xtable_address_t table_address);

NS_END2

NS_BEG1(top)

template <>
xbytes_t to_bytes<common::xtable_address_t>(common::xtable_address_t const & input);

template <>
std::string to_string<common::xtable_address_t>(common::xtable_address_t const & input);

NS_END1

NS_BEG1(std)

template <>
struct hash<top::common::xtable_address_t> final {
    std::size_t operator()(top::common::xtable_address_t const & table_address) const noexcept;
};

NS_END1
