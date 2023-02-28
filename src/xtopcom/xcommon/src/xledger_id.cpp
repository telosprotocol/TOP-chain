// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xledger_id.h"

#include "xbase/xutl.h"
#include "xcommon/xerror/xerror.h"

NS_BEG2(top, common)

xtop_ledger_id::xtop_ledger_id(uint16_t const input) noexcept : m_value {input} {
}

xtop_ledger_id::xtop_ledger_id(std::string const & ledger_id_string) : xtop_ledger_id{static_cast<uint16_t>(base::xstring_utl::hex2uint64(ledger_id_string))} {
}

bool xtop_ledger_id::empty() const noexcept {
    return m_value == std::numeric_limits<uint16_t>::max();
}

void xtop_ledger_id::swap(xtop_ledger_id & other) noexcept {
    std::swap(m_value, other.m_value);
}

void xtop_ledger_id::clear() noexcept {
    m_value = std::numeric_limits<uint16_t>::max();
}

xzone_id_t xtop_ledger_id::zone_id() const noexcept {
    return xzone_id_t{static_cast<uint8_t>(m_value & 0x0F)};
}

uint16_t xtop_ledger_id::value(std::error_code & ec) const noexcept {
    if (m_value == std::numeric_limits<uint16_t>::max()) {
        ec = error::xerrc_t::invalid_ledger_id;
    }

    return m_value;
}

uint16_t xtop_ledger_id::value() const {
    std::error_code ec;
    auto const r = value(ec);
    top::error::throw_error(ec);
    return r;
}

NS_END2
