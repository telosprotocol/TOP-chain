// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_context.h"

#if defined(XCXX20)
#include "xevm_runner/proto/ubuntu/proto_parameters.pb.h"
#else
#include "xevm_runner/proto/centos/proto_parameters.pb.h"
#endif
#include "xcommon/common_data.h"


NS_BEG2(top, evm_runtime)

xtop_evm_context::xtop_evm_context(std::unique_ptr<data::xbasic_top_action_t const> action, txexecutor::xvm_para_t const & vm_para) noexcept : m_action{std::move(action)} {
    assert(m_action->type() == data::xtop_action_type_t::evm);

    // todo(jimmy) get chain id from? , default is 1023 in xevm_context.h:23
    // m_chain_id = ...
    m_block_coinbase = vm_para.get_block_coinbase();
    assert(common::is_t8_address(m_block_coinbase));// XTODO only support T8 miner address
    m_block_height = vm_para.get_block_height();
    m_block_timestamp = vm_para.get_timestamp();

    m_gas_limit = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->gas_limit();
    m_random_seed = vm_para.get_random_seed();

    evm_engine::parameters::FunctionCallArgs call_args;
    call_args.set_version(CURRENT_CALL_ARGS_VERSION);
    call_args.set_input(top::to_string(static_cast<data::xevm_consensus_action_t const *>(m_action.get())->data()));
    call_args.set_gas_limit(m_gas_limit);
    if (action_type() == data::xtop_evm_action_type::deploy_contract) {
        // byte code is all evm need.
        // m_input_data = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->data();
    } else if (action_type() == data::xtop_evm_action_type::call_contract) {
        assert(sender().to_string().substr(0, 6) == base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN);
        auto address = call_args.mutable_address();  // contract address
        address->set_value(recver().to_string().substr(6));
    } else {
        xassert(false);
    }

    evm_common::u256 value_u256 = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->value();
    auto value = call_args.mutable_value();
    value->set_data(evm_common::toBigEndianString(value_u256));

    m_input_data = top::to_bytes(call_args.SerializeAsString());
}

data::xtop_evm_action_type xtop_evm_context::action_type() const {
    assert(m_action->type() == data::xtop_action_type_t::evm);
    return static_cast<data::xevm_consensus_action_t const *>(m_action.get())->evm_action_type();
}

xbytes_t const & xtop_evm_context::input_data() const {
    return m_input_data;
}

common::xaccount_address_t xtop_evm_context::sender() const {
    assert(m_action->type() == data::xtop_action_type_t::evm);
    common::xaccount_address_t ret = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->sender();
    return ret;
}

common::xaccount_address_t xtop_evm_context::recver() const {
    assert(m_action->type() == data::xtop_action_type_t::evm);
    common::xaccount_address_t ret = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->recver();
    return ret;
}

std::string const & xtop_evm_context::random_seed() const noexcept {
    return m_random_seed;
}

uint64_t xtop_evm_context::gas_limit() const noexcept {
    return m_gas_limit;
}

// EVM API:
uint64_t xtop_evm_context::chain_id() const noexcept {
    return m_chain_id;
}
std::string xtop_evm_context::block_coinbase() const noexcept {
    return m_block_coinbase.to_string();
}
uint64_t xtop_evm_context::block_height() const noexcept {
    return m_block_height;
}
uint64_t xtop_evm_context::block_timestamp() const noexcept {
    return m_block_timestamp;
}

NS_END2
