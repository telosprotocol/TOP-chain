// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xcommon/xaccount_base_address_fwd.h"
#include "xcommon/xledger_id.h"
#include "xcommon/xtable_id.h"
#include "xvledger/xvaccount.h"

#include <functional>
#include <string>
#include <system_error>

NS_BEG2(top, common)

class xtop_account_base_address;
using xaccount_base_address_t = xtop_account_base_address;

int32_t operator>>(base::xstream_t & stream, xaccount_base_address_t & account_base_address);
int32_t operator>>(base::xbuffer_t & buffer, xaccount_base_address_t & account_base_address);
int32_t operator<<(base::xstream_t & stream, xaccount_base_address_t const & account_base_address);
int32_t operator<<(base::xbuffer_t & buffer, xaccount_base_address_t const & account_base_address);

class xtop_account_base_address {
    std::string m_base_address_str;
    base::enum_vaccount_addr_type m_account_type{base::enum_vaccount_addr_type::enum_vaccount_addr_type_invalid};
    xledger_id_t m_ledger_id;
    xtable_id_t m_default_table_id;

public:
    xtop_account_base_address() = default;
    xtop_account_base_address(xtop_account_base_address const &) = default;
    xtop_account_base_address & operator=(xtop_account_base_address const &) = default;
    xtop_account_base_address(xtop_account_base_address &&) = default;
    xtop_account_base_address & operator=(xtop_account_base_address &&) = default;
    ~xtop_account_base_address() = default;

private:
    explicit xtop_account_base_address(std::string const & base_address);

public:
    static xtop_account_base_address build_from(std::string const & input, std::error_code & ec);
    static xtop_account_base_address build_from(std::string const & input);

    void swap(xtop_account_base_address & other) noexcept;

    std::string const & to_string() const noexcept;
    char const * c_str() const noexcept;

    bool empty() const noexcept;
    void clear();

    base::enum_vaccount_addr_type type(std::error_code & ec) const;
    base::enum_vaccount_addr_type type() const;

    xledger_id_t const & ledger_id() const noexcept;
    xtable_id_t const & default_table_id() const noexcept;

    bool operator==(xtop_account_base_address const & other) const noexcept;
    bool operator<(xtop_account_base_address const & other) const noexcept;
    bool operator>(xtop_account_base_address const & other) const noexcept;
    bool operator<=(xtop_account_base_address const & other) const noexcept;
    bool operator>=(xtop_account_base_address const & other) const noexcept;
    bool operator!=(xtop_account_base_address const & other) const noexcept;

    int32_t serialize_to(base::xstream_t & stream) const;
    int32_t serialize_from(base::xstream_t & stream);

    friend int32_t operator>>(base::xstream_t & stream, xaccount_base_address_t & account_base_address);
    friend int32_t operator>>(base::xbuffer_t & buffer, xaccount_base_address_t & account_base_address);
    friend int32_t operator<<(base::xstream_t & stream, xaccount_base_address_t const & account_base_address);
    friend int32_t operator<<(base::xbuffer_t & buffer, xaccount_base_address_t const & account_base_address);

private:
    int32_t do_write(base::xstream_t & stream) const;
    int32_t do_read(base::xstream_t & stream);
};
using xaccount_base_address_t = xtop_account_base_address;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xaccount_base_address_t> {
    size_t operator()(top::common::xaccount_base_address_t const & input) const;
};

NS_END1
