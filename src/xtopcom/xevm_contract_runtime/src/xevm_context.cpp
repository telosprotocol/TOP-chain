// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_context.h"

NS_BEG2(top, evm_runtime)

xtop_evm_context::xtop_evm_context(std::unique_ptr<data::xbasic_top_action_t const> action) noexcept : m_action{std::move(action)} {
    assert(m_action->type() == data::xtop_action_type_t::evm);
    // m_evm_action_type = deploy/call/..
}

xtop_evm_action_type xtop_evm_context::action_type() const {
    return m_action_type;
}

void xtop_evm_context::input_data(xbytes_t const & data) {
    m_input_data = data;
}
xbytes_t const & xtop_evm_context::input_data() const {
    return m_input_data;
}

common::xaccount_address_t xtop_evm_context::sender() const {
    assert(m_action->type() == data::xtop_action_type_t::evm);
    common::xaccount_address_t ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->sender();
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