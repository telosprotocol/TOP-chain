// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbasic/xbyte_buffer.h"
#include "xcommon/xaddress.h"
#include "xdata/xaction.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xconsensus_action_fwd.h"
#include "xdata/xconsensus_action_stage.h"
#include "xdata/xtop_action.h"

#include <cstdint>
#include <string>

NS_BEG2(top, data)

template <xtop_action_type_t ActionTypeV>
class xtop_consensus_action : public xtop_action_t<ActionTypeV> {
public:
    xtop_consensus_action(xtop_consensus_action const &) = default;
    xtop_consensus_action & operator=(xtop_consensus_action const &) = default;
    xtop_consensus_action(xtop_consensus_action &&) = default;
    xtop_consensus_action & operator=(xtop_consensus_action &&) = default;
    ~xtop_consensus_action() override = default;

    explicit xtop_consensus_action(xobject_ptr_t<data::xcons_transaction_t> const & tx) noexcept;

    xconsensus_action_stage_t stage() const noexcept;
    common::xaccount_address_t from_address() const;
    common::xaccount_address_t to_address() const;
    common::xaccount_address_t contract_address() const;
    common::xaccount_address_t execution_address() const;
    uint64_t max_gas_amount() const;
    uint64_t last_nonce() const noexcept;
    uint64_t nonce() const noexcept;
    uint256_t hash() const noexcept;
    std::string action_name() const;
    xbyte_buffer_t action_data() const;
    std::map<std::string, xbyte_buffer_t> receipt_data() const;
    std::string transaction_source_action_data() const;
    std::string transaction_target_action_data() const;
    data::enum_xtransaction_type transaction_type() const;
    data::enum_xaction_type transaction_source_action_type() const;
    data::enum_xaction_type transaction_target_action_type() const;
    uint32_t size() const;
    uint32_t deposit() const;
    std::string digest_hex() const;
    uint32_t used_tgas() const;
    uint32_t used_disk() const;
    uint32_t last_action_send_tx_lock_tgas() const;
    uint32_t last_action_used_deposit() const;
    uint32_t last_action_recv_tx_use_send_tx_tgas() const;
    enum_xunit_tx_exec_status last_action_exec_status() const;
};

NS_END2

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xvledger/xvblock.h"

NS_BEG2(top, data)

template <xtop_action_type_t ActionTypeV>
xtop_consensus_action<ActionTypeV>::xtop_consensus_action(xobject_ptr_t<data::xcons_transaction_t> const & tx) noexcept : xtop_top_action<ActionTypeV>{ tx, tx->is_send_tx() ? static_cast<common::xlogic_time_t>((tx->get_transaction()->get_fire_timestamp() + tx->get_transaction()->get_expire_duration() + XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance)) / XGLOBAL_TIMER_INTERVAL_IN_SECONDS) : common::xjudgement_day } {
}

template <xtop_action_type_t ActionTypeV>
xconsensus_action_stage_t xtop_consensus_action<ActionTypeV>::stage() const noexcept {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);
    switch (tx->get_tx_subtype()) {  // NOLINT(clang-diagnostic-switch-enum)
    case base::enum_transaction_subtype_send:
        return xconsensus_action_stage_t::send;

    case base::enum_transaction_subtype_recv:
        return xconsensus_action_stage_t::recv;

    case base::enum_transaction_subtype_confirm:
        return xconsensus_action_stage_t::confirm;

    case base::enum_transaction_subtype_self:
        return xconsensus_action_stage_t::self;

    default:
        assert(false);
        return xconsensus_action_stage_t::invalid;
    }
}

template <xtop_action_type_t ActionTypeV>
common::xaccount_address_t xtop_consensus_action<ActionTypeV>::from_address() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);
    return common::xaccount_address_t{ tx->get_source_addr() };
}

template <xtop_action_type_t ActionTypeV>
common::xaccount_address_t xtop_consensus_action<ActionTypeV>::to_address() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);
    return common::xaccount_address_t{ tx->get_target_addr() };
}

template <xtop_action_type_t ActionTypeV>
common::xaccount_address_t xtop_consensus_action<ActionTypeV>::contract_address() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);
    return common::xaccount_address_t{ tx->get_target_addr() };
}

template <xtop_action_type_t ActionTypeV>
common::xaccount_address_t xtop_consensus_action<ActionTypeV>::execution_address() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    switch (stage()) {
    case xconsensus_action_stage_t::send:
        XATTRIBUTE_FALLTHROUGH;
    case xconsensus_action_stage_t::confirm:
        return common::xaccount_address_t{ tx->get_source_addr() };

    case xconsensus_action_stage_t::recv:
        return common::xaccount_address_t{ tx->get_target_addr() };

    case xconsensus_action_stage_t::self:
        return common::xaccount_address_t{ tx->get_target_addr() };

    default:
        assert(false);
        return common::xaccount_address_t{};
    }
}

template <xtop_action_type_t ActionTypeV>
uint64_t xtop_consensus_action<ActionTypeV>::max_gas_amount() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);
    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio) * tx->get_transaction()->get_deposit(); // TODO free tgas is missing here.
}

template <xtop_action_type_t ActionTypeV>
uint64_t xtop_consensus_action<ActionTypeV>::nonce() const noexcept {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_transaction()->get_tx_nonce();
}

template <xtop_action_type_t ActionTypeV>
uint64_t xtop_consensus_action<ActionTypeV>::last_nonce() const noexcept {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_transaction()->get_last_nonce();
}

template <xtop_action_type_t ActionTypeV>
uint256_t xtop_consensus_action<ActionTypeV>::hash() const noexcept {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_tx_hash_256();
}

template <xtop_action_type_t ActionTypeV>
std::string xtop_consensus_action<ActionTypeV>::action_name() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_transaction()->get_target_action().get_action_name();
}

template <xtop_action_type_t ActionTypeV>
xbyte_buffer_t xtop_consensus_action<ActionTypeV>::action_data() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return { std::begin(tx->get_transaction()->get_target_action().get_action_param()), std::end(tx->get_transaction()->get_target_action().get_action_param()) };
}

template <xtop_action_type_t ActionTypeV>
std::map<std::string, xbyte_buffer_t> xtop_consensus_action<ActionTypeV>::receipt_data() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);
    return tx->get_last_action_receipt_data();
}


template <xtop_action_type_t ActionTypeV>
std::string xtop_consensus_action<ActionTypeV>::transaction_source_action_data() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    auto temp = tx->get_transaction()->get_source_action().get_action_param();
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)temp.data(), temp.size());
    data::xproperty_asset asset_out{0};
    stream >> asset_out.m_token_name;
    stream >> asset_out.m_amount;

    return tx->get_transaction()->get_source_action().get_action_param();
}

template <xtop_action_type_t ActionTypeV>
std::string xtop_consensus_action<ActionTypeV>::transaction_target_action_data() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_transaction()->get_target_action().get_action_param();
}

template <xtop_action_type_t ActionTypeV>
data::enum_xtransaction_type xtop_consensus_action<ActionTypeV>::transaction_type() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return data::enum_xtransaction_type(tx->get_tx_type());
}

template <xtop_action_type_t ActionTypeV>
data::enum_xaction_type xtop_consensus_action<ActionTypeV>::transaction_source_action_type() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_transaction()->get_source_action().get_action_type();
}

template <xtop_action_type_t ActionTypeV>
data::enum_xaction_type xtop_consensus_action<ActionTypeV>::transaction_target_action_type() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_transaction()->get_target_action().get_action_type();
}

template <xtop_action_type_t ActionTypeV>
uint32_t xtop_consensus_action<ActionTypeV>::size() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_transaction()->get_tx_len();
}

template <xtop_action_type_t ActionTypeV>
uint32_t xtop_consensus_action<ActionTypeV>::deposit() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_transaction()->get_deposit();
}

template <xtop_action_type_t ActionTypeV>
std::string xtop_consensus_action<ActionTypeV>::digest_hex() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_digest_hex_str();
}

template <xtop_action_type_t ActionTypeV>
uint32_t xtop_consensus_action<ActionTypeV>::used_tgas() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_current_used_tgas();
}

template <xtop_action_type_t ActionTypeV>
uint32_t xtop_consensus_action<ActionTypeV>::used_disk() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_current_used_disk();
}

template <xtop_action_type_t ActionTypeV>
uint32_t xtop_consensus_action<ActionTypeV>::last_action_send_tx_lock_tgas() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_last_action_send_tx_lock_tgas();
}

template <xtop_action_type_t ActionTypeV>
uint32_t xtop_consensus_action<ActionTypeV>::last_action_used_deposit() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_last_action_used_deposit();
}

template <xtop_action_type_t ActionTypeV>
uint32_t xtop_consensus_action<ActionTypeV>::last_action_recv_tx_use_send_tx_tgas() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_last_action_recv_tx_use_send_tx_tgas();
}

template <xtop_action_type_t ActionTypeV>
enum_xunit_tx_exec_status xtop_consensus_action<ActionTypeV>::last_action_exec_status() const {
    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(this->m_action_src);
    assert(tx != nullptr);

    return tx->get_last_action_exec_status();
}



NS_END2
