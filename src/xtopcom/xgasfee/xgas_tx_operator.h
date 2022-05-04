// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address.h"
#include "xdata/xcons_transaction.h"

namespace top {
namespace gasfee {

class xtop_gas_tx_operator {
public:
    explicit xtop_gas_tx_operator(xobject_ptr_t<data::xcons_transaction_t> const & tx);
    xtop_gas_tx_operator(xtop_gas_tx_operator const &) = delete;
    xtop_gas_tx_operator & operator=(xtop_gas_tx_operator const &) = delete;
    xtop_gas_tx_operator(xtop_gas_tx_operator &&) = default;
    xtop_gas_tx_operator & operator=(xtop_gas_tx_operator &&) = default;
    ~xtop_gas_tx_operator() = default;

public:
    common::xaccount_address_t sender() const;
    common::xaccount_address_t recver() const;
    data::enum_xtransaction_type tx_type() const;
    base::enum_transaction_subtype tx_subtype() const;
    data::enum_xtransaction_version tx_version() const;
    evm_common::u256 tx_gas_limit() const;
    evm_common::u256 tx_eth_fee_per_gas() const;
    evm_common::u256 tx_top_fee_per_gas() const;
    uint64_t deposit() const;
    uint64_t tx_used_tgas() const;
    uint64_t tx_last_action_used_deposit() const;
    uint64_t tx_last_action_recv_tx_use_send_tx_tgas() const;

    void tx_set_used_tgas(const uint64_t tgas);
    void tx_set_used_deposit(const uint64_t deposit);
    void tx_set_current_recv_tx_use_send_tx_tgas(const uint64_t tgas);

    uint64_t tx_fixed_tgas() const;
    uint64_t tx_bandwith_tgas() const;
    uint64_t tx_disk_tgas() const;
    evm_common::u256 tx_limited_tgas() const;

private:
    xobject_ptr_t<data::xcons_transaction_t> m_tx{nullptr};
};
using xgas_tx_operator_t = xtop_gas_tx_operator;

}  // namespace gasfee
}  // namespace top
