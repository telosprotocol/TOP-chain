// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xaddress.h"
#include "xdata/xtop_action.h"
#include "xdata/xcons_transaction.h"

#include <cstdint>
#include <string>

NS_BEG2(top, data)

enum class xenum_consensus_action_stage : uint8_t{
    invalid,
    send,
    recv,
    confirm,
    self
};
using xconsensus_action_stage_t = xenum_consensus_action_stage;

class xtop_consensus_action : public xtop_action_t {
public:
    xtop_consensus_action(xtop_consensus_action const &) = default;
    xtop_consensus_action & operator=(xtop_consensus_action const &) = default;
    xtop_consensus_action(xtop_consensus_action &&) = default;
    xtop_consensus_action & operator=(xtop_consensus_action &&) = default;
    ~xtop_consensus_action() override = default;

    explicit xtop_consensus_action(xobject_ptr_t<data::xcons_transaction_t> tx) noexcept;

    xconsensus_action_stage_t stage() const noexcept;
    common::xaccount_address_t from_address() const;
    common::xaccount_address_t to_address() const;
    common::xaccount_address_t contract_address() const;
    common::xaccount_address_t execution_address() const;
    uint64_t max_gas_amount() const;
    uint64_t nonce() const noexcept;
    std::string action_name() const;
    xbyte_buffer_t action_data() const;
};
using xconsensus_action_t = xtop_consensus_action;

NS_END2
