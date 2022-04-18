// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_context.h"

#include "xevm_runner/proto/proto_parameters.pb.h"

NS_BEG2(top, evm_runtime)

xtop_evm_context::xtop_evm_context(std::unique_ptr<data::xbasic_top_action_t const> action) noexcept : m_action{std::move(action)} {
    assert(m_action->type() == data::xtop_action_type_t::evm);
    // m_evm_action_type = deploy/call/..
    // auto const * evm_action = static_cast<data::xevm_consensus_action_t const *>(m_action.get());
    // m_evm_action_type = evm_action->evm_action();
    // m_input_data = evm_action->data();

    // todo // get action_type/sender/recever/gas/value/data.... from action
    // - [x] action_type
    // - [x] sender
    // - [x] recever
    // - [] gas xxx
    // - [] value
    // - [x] data
    if (action_type() == data::xtop_evm_action_type::deploy_contract) {
        // byte code is all evm need.
        m_input_data = static_cast<data::xevm_consensus_action_t const *>(m_action.get())->data();
        // return static_cast<data::xevm_consensus_action_t const *>(m_action.get())->data();
    } else if (action_type() == data::xtop_evm_action_type::call_contract) {
        evm_engine::parameters::FunctionCallArgs call_args;
        call_args.set_version(CURRENT_CALL_ARGS_VERSION);

        call_args.set_input(top::to_string(static_cast<data::xevm_consensus_action_t const *>(m_action.get())->data()));

        assert(sender().value().substr(0, 6) == "T60004");
        auto address = call_args.mutable_address(); // contract address
        address->set_value(recver().value().substr(6));

        // todo value: call_args.value(WeiU256)

        m_input_data = top::to_bytes(call_args.SerializeAsString());

    } else {
        xassert(false);
    }
}

data::xtop_evm_action_type xtop_evm_context::action_type() const {
    assert(m_action->type() == data::xtop_action_type_t::evm);
    return static_cast<data::xevm_consensus_action_t const *>(m_action.get())->evm_action();
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
    // todo
    //  if (m_state_ctx != nullptr) {
    //      return m_state_ctx->get_ctx_para().m_random_seed;
    //  }
    //  return m_param.random_seed;
}

NS_END2