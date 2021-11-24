// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xcommon/xaccount_address_fwd.h"
#include "xcommon/xaccount_base_address_fwd.h"
#include "xcommon/xip.h"
#include "xcommon/xledger_id.h"
#include "xcommon/xtable_id.h"

#include <limits>
#include <string>
#include <system_error>

NS_BEG2(top, common)

class xtop_account_id {
    friend xaccount_address_t;

    xvid_t m_value{std::numeric_limits<xvid_t>::max()};

public:
    xtop_account_id() = default;
    xtop_account_id(xtop_account_id const &) = default;
    xtop_account_id & operator=(xtop_account_id const &) = default;
    xtop_account_id(xtop_account_id &&) = default;
    xtop_account_id & operator=(xtop_account_id &&) = default;
    ~xtop_account_id() = default;

private:
    xtop_account_id(std::string const & account_string);

public:
    void swap(xtop_account_id & other) noexcept;
    bool empty() const noexcept;

    xledger_id_t ledger_id() const;
    // xzone_id_t zone_id() const;
    xtable_id_t table_id() const;

    xvid_t value(std::error_code & ec) const noexcept;
    xvid_t value() const;
};
using xaccount_id_t = xtop_account_id;

NS_END2
