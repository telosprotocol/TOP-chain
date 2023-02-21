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

/*
 /////////////////////////////////XID defintion/////////////////////////////////////
 //XID : ID of the logic & virtual account/user# at overlay network
 XID  definition as total 64bit =
 {
    -[32bit]    //index(could be as hash32(account_address)
    -[26bit]    //prefix  e.g. xledger defined as below
        -[16bit:ledger_id]
            -[12bit:net#/chain#]
            -[4 bit:zone#/bucket-index]
        -[10bit:subaddr_of_ledger]
            -[7 bit:book-index]
            -[3 bit:table-index]
    -[enum_xid_level :3bit]
    -[enum_xid_type  :3bit]
 }
 */

struct xtop_virtual_id {
    uint32_t index;
    uint32_t chain_id : 12;
    uint32_t zone_index : 4;
    uint32_t book_index : 7;
    uint32_t table_index : 3;
    uint32_t level : 3;
    uint32_t type : 3;
};
using xvirtual_id_t = xtop_virtual_id;

class xtop_account_id {
    friend xaccount_address_t;

    union {
        xvid_t m_value{std::numeric_limits<xvid_t>::max()};
        xvirtual_id_t m_vid;
    };

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
