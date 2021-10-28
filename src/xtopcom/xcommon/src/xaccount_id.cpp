// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xaccount_id.h"

#include "xvledger/xvaccount.h"

NS_BEG2(top, common)

xtop_account_id::xtop_account_id(std::string const & account_string) : m_value{base::xvaccount_t::get_xid_from_account(account_string)} {
}

xledger_id_t xtop_account_id::ledger_id() const {
    return xledger_id_t{static_cast<uint16_t>(get_vledger_ledger_id(m_value))};
}

xzone_id_t xtop_account_id::zone_id() const {
    return xzone_id_t{static_cast<uint8_t>(get_vledger_zone_index(m_value))};
}

void xtop_account_id::swap(xtop_account_id & other) noexcept {
    std::swap(m_value, other.m_value);
}

bool xtop_account_id::empty() const noexcept {
    return m_value == std::numeric_limits<xvid_t>::max();
}

NS_END2
