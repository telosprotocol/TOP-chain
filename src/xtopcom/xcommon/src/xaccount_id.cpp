// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xaccount_id.h"

#include "xcommon/xerror/xerror.h"
#include "xvledger/xvaccount.h"

#include <cassert>

NS_BEG2(top, common)

xtop_account_id::xtop_account_id(std::string const & account_string) : m_value{base::xvaccount_t::get_xid_from_account(account_string)} {
}

xledger_id_t xtop_account_id::ledger_id() const {
    return xledger_id_t{static_cast<uint16_t>(get_vledger_ledger_id(m_value))};
}

//xzone_id_t xtop_account_id::zone_id() const {
//    return xzone_id_t{static_cast<uint8_t>(get_vledger_zone_index(m_value))};
//}

xtable_id_t xtop_account_id::table_id() const {
    return xtable_id_t{*this};
}

void xtop_account_id::swap(xtop_account_id & other) noexcept {
    std::swap(m_value, other.m_value);
}

bool xtop_account_id::empty() const noexcept {
    return m_value == std::numeric_limits<xvid_t>::max();
}

xvid_t xtop_account_id::value(std::error_code & ec) const noexcept {
    assert(!ec);

    if (empty()) {
        ec = error::xerrc_t::invalid_account_index;
    }

    return m_value;
}

xvid_t xtop_account_id::value() const {
    std::error_code ec;
    auto r = value(ec);
    top::error::throw_error(ec);
    return r;
}

NS_END2
