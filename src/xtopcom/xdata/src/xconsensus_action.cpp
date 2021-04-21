//// Copyright (c) 2017-2018 Telos Foundation & contributors
//// Distributed under the MIT software license, see the accompanying
//// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
//#include "xdata/xconsensus_action.h"
//
//#include "xconfig/xconfig_register.h"
//#include "xconfig/xpredefined_configurations.h"
//#include "xvledger/xvblock.h"
//
//#include <cassert>
//#include <chrono>
//
//NS_BEG2(top, data)
//
//xtop_consensus_action::xtop_consensus_action(xobject_ptr_t<data::xcons_transaction_t> tx) noexcept
//    : xtop_action_t{
//        tx,
//        tx->is_send_tx() ? static_cast<common::xlogic_time_t>((tx->get_transaction()->get_fire_timestamp() +tx->get_transaction()->get_expire_duration() + XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance)) / XGLOBAL_TIMER_INTERVAL_IN_SECONDS) : common::xjudgement_day,
//        common::xaccount_address_t{tx->get_target_addr()}.type() == base::enum_vaccount_addr_type_native_contract ? xtop_action_type_t::system_contract : xtop_action_type_t::user_contract
//     } {
//}
//
//xconsensus_action_stage_t xtop_consensus_action::stage() const noexcept {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//    switch (tx->get_tx_subtype()) {
//    case base::enum_transaction_subtype_send:
//        return xconsensus_action_stage_t::send;
//
//    case base::enum_transaction_subtype_recv:
//        return xconsensus_action_stage_t::recv;
//
//    case base::enum_transaction_subtype_confirm:
//        return xconsensus_action_stage_t::confirm;
//
//    case base::enum_transaction_subtype_self:
//        return xconsensus_action_stage_t::self;
//
//    default:
//        assert(false);
//        return xconsensus_action_stage_t::invalid;
//    }
//}
//
//common::xaccount_address_t xtop_consensus_action::from_address() const {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//    return common::xaccount_address_t{ tx->get_source_addr() };
//}
//
//common::xaccount_address_t xtop_consensus_action::to_address() const {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//    return common::xaccount_address_t{ tx->get_target_addr() };
//}
//
//uint64_t xtop_consensus_action::max_gas_amount() const {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio) * tx->get_transaction()->get_deposit(); // TODO free tgas is missing here.
//}
//
//uint64_t xtop_consensus_action::nonce() const noexcept {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//
//    return tx->get_transaction()->get_tx_nonce();
//}
//
//common::xaccount_address_t xtop_consensus_action::contract_address() const {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//    return common::xaccount_address_t{ tx->get_target_addr() };
//}
//
//common::xaccount_address_t xtop_consensus_action::execution_address() const {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//
//    switch (stage()) {
//    case xconsensus_action_stage_t::send:
//        return common::xaccount_address_t{ tx->get_source_addr() };
//
//    case xconsensus_action_stage_t::recv:
//        return common::xaccount_address_t{ tx->get_target_addr() };
//
//    case xconsensus_action_stage_t::self:
//        return common::xaccount_address_t{ tx->get_target_addr() };
//
//    default:
//        assert(false);
//        return common::xaccount_address_t{};
//    }
//}
//
//std::string xtop_consensus_action::action_name() const {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//
//    return tx->get_target_action().get_action_name();
//}
//
//xbyte_buffer_t xtop_consensus_action::action_data() const {
//    auto const & tx = dynamic_xobject_ptr_cast<data::xcons_transaction_t>(m_action_src);
//    assert(tx != nullptr);
//
//    return { std::begin(tx->get_target_action().get_action_param()), std::end(tx->get_target_action().get_action_param()) };
//}
//
//NS_END2
