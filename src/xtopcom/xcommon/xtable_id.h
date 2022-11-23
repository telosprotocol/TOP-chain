// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xstring.h"
#include "xcommon/xaccount_address_fwd.h"
#include "xcommon/xaccount_base_address_fwd.h"
#include "xcommon/xaccount_id_fwd.h"
#include "xcommon/xtable_id_fwd.h"
#include "xbase/xbase.h"

#include <cstdint>
#include <system_error>

NS_BEG2(top, common)

class xtop_table_id {
    friend xaccount_id_t;
    friend xaccount_address_t;
    friend xaccount_base_address_t;

    uint16_t m_value{enum_vbucket_has_tables_count};

public:
    xtop_table_id() = default;
    xtop_table_id(xtop_table_id const &) = default;
    xtop_table_id & operator=(xtop_table_id const &) = default;
    xtop_table_id(xtop_table_id &&) = default;
    xtop_table_id & operator=(xtop_table_id &&) = default;
    ~xtop_table_id() = default;

    explicit xtop_table_id(uint16_t value);

private:
    explicit xtop_table_id(xaccount_id_t const & account_id);

public:
    void swap(xtop_table_id & other) noexcept;
    void clear() noexcept;

    bool empty() const noexcept;
    uint16_t value(std::error_code & ec) const noexcept;
    uint16_t value() const;

    bool operator==(xtop_table_id const & other) const noexcept;
    bool operator<(xtop_table_id const & other) const noexcept;
    bool operator>(xtop_table_id const & other) const noexcept;
    bool operator!=(xtop_table_id const & other) const noexcept;
    bool operator<=(xtop_table_id const & other) const noexcept;
    bool operator>=(xtop_table_id const & other) const noexcept;
};

NS_END2

NS_BEG1(top)

template <>
std::string to_string<common::xtable_id_t>(common::xtable_id_t const & table_id);

template <>
common::xtable_id_t from_string<common::xtable_id_t>(std::string const & input, std::error_code & ec);

template <>
common::xtable_id_t from_string<common::xtable_id_t>(std::string const & input);

NS_END1
