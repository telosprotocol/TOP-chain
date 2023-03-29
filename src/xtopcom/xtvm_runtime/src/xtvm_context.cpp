// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtvm_runtime/xtvm_context.h"

#include "xbasic/xhex.h"
#include "xdata/xconsensus_action.h"
// xtvm_engine_rs/tvm-c-api/
#include "protobuf_types/pbasic.pb.h"
#include "protobuf_types/pparameters.pb.h"

NS_BEG2(top, tvm)

// static const uint32_t CURRENT_CALL_ARGS_VERSION = 1;

xtop_vm_context::xtop_vm_context(std::unique_ptr<data::xbasic_top_action_t const> action, txexecutor::xvm_para_t const & vm_para) : m_action{std::move(action)} {
    // common usage:
    assert(m_action->type() == data::xtop_action_type_t::evm);

    // gas_limit:
    auto gas_limit = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->gas_limit();

    // action_type:
    // we don't really need to sense this, should also be deleted in `action`
    // m_action_type = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->evm_action_type();

    // todo! gas price
    // m_gas_price = 0;

    // sender:
    assert(m_action->type() == data::xtop_action_type_t::evm);
    m_sender = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->sender();

    //  recver:
    auto m_recver = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->recver();

    // block_height:
    m_block_height = vm_para.get_block_height();

    // coinbase:
    m_coinbase = vm_para.get_block_coinbase();

    // block ts:
    m_block_timestamp = vm_para.get_timestamp();

    // chain_id;
    m_chain_id = XGET_CONFIG(chain_id);

    // input_data;
    top::tvm_engine::parameters::PCallArgs call_args;
    call_args.set_gas_limit(gas_limit);
    call_args.set_input(top::to_string(static_cast<data::xevm_consensus_action_t const *>(m_action.get())->data()));
    auto sender_address = call_args.mutable_sender_address();
    sender_address->set_value(top::to_string(top::from_hex(m_sender.to_string().substr(6))));
    auto recver_address = call_args.mutable_recver_address();
    recver_address->set_value(top::to_string(top::from_hex(m_recver.to_string().substr(6))));
    evm_common::u256 value_u256 = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->value();  // utop , should not bigger than U256
    uint64_t value_u64 = value_u256.convert_to<uint64_t>();
    call_args.set_value(value_u64);
    m_input_data = top::to_bytes(call_args.SerializeAsString());
}

// data::xtop_evm_action_type xtop_vm_context::action_type() const{
//     return m_action_type;
// }
xbytes_t const & xtop_vm_context::input_data() const {
    return m_input_data;
}
uint64_t xtop_vm_context::gas_price() const {
    return m_gas_price;
}
common::xaccount_address_t xtop_vm_context::sender() const {
    return m_sender;
}
uint64_t xtop_vm_context::block_height() const {
    return m_block_height;
}
common::xaccount_address_t xtop_vm_context::coinbase() const {
    return m_coinbase;
}
uint64_t xtop_vm_context::block_timestamp() const {
    return m_block_timestamp;
}
uint64_t xtop_vm_context::chain_id() const {
    return m_chain_id;
}

NS_END2