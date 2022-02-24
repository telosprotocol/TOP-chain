// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_base_address_fwd.h"
#include "xcommon/xaccount_id_fwd.h"
#include "xcommon/xip.h"
#include "xcommon/xledger_id_fwd.h"
#include "xvledger/xvaccount.h"

#include <cstdint>
#include <limits>

NS_BEG2(top, common)

class xtop_ledger_id {
    friend xaccount_base_address_t;
    friend xaccount_id_t;

    uint16_t m_value{std::numeric_limits<uint16_t>::max()};

public:
    xtop_ledger_id() = default;
    xtop_ledger_id(xtop_ledger_id const &) = default;
    xtop_ledger_id & operator=(xtop_ledger_id const &) = default;
    xtop_ledger_id(xtop_ledger_id &&) = default;
    xtop_ledger_id & operator=(xtop_ledger_id &&) = default;
    ~xtop_ledger_id() = default;

private:
    explicit xtop_ledger_id(uint16_t const input) noexcept;
    explicit xtop_ledger_id(std::string const & ledger_id_string);

public:
    bool empty() const noexcept;

    void swap(xtop_ledger_id & other) noexcept;
    void clear() noexcept;

    xzone_id_t zone_id() const noexcept;

    uint16_t value(std::error_code & ec) const noexcept;
    uint16_t value() const;
};
using xledger_id_t = xtop_ledger_id;

NS_END2

NS_BEG1(std)



NS_END1
