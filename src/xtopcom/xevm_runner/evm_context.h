#pragma once
#include "xcontract_common/xcontract_execution_param.h"
#include "xdata/xtop_action.h"
#include "xevm_runner/evm_util.h"

#include <string>
namespace top {
namespace evm {

class xtop_evm_context {
public:
    // TODO: delete
    bytes m_random_seed;
    bytes m_predecessor_account_id;

    std::unique_ptr<data::xbasic_top_action_t const> m_action;
    contract_common::xcontract_execution_param_t m_param;
    bytes m_input;

public:
    xtop_evm_context(bytes const & random_seed, bytes const & input, bytes const & predecessor_account_id)
      : m_random_seed{random_seed}, m_input{input}, m_predecessor_account_id{predecessor_account_id} {
    }

    xtop_evm_context(std::unique_ptr<data::xbasic_top_action_t const> action, contract_common::xcontract_execution_param_t const & param)
      : m_action{std::move(action)}, m_param{param} {
    }

    void set_input(xbytes_t const & input) {
        m_input = input;
    }

    xbytes_t input() {
        return m_input;
    }

    xbytes_t random_seed() {
        return top::to_bytes(m_param.random_seed);
    }
};
using xevm_context_t = xtop_evm_context;
}  // namespace evm
}  // namespace top