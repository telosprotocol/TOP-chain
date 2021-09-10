// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_vm/xaccount_vm.h"

#include "xbase/xlog.h"
#include "xbasic/xerror/xchain_error.h"
#include "xcontract_common/xaction_execution_param.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xerror/xerror.h"
#include "xcontract_runtime/xaction_session.h"
#include "xcontract_runtime/xtop_action_generator.h"
#include "xcontract_vm/xerror/xerror.h"
#include "xdata/xconsensus_action.h"

#include <memory>

NS_BEG2(top, contract_vm)

xtop_account_vm::xtop_account_vm(observer_ptr<contract_runtime::system::xsystem_contract_manager_t> const & system_contract_manager)
  : sys_action_runtime_{top::make_unique<contract_runtime::system::xsystem_action_runtime_t>(system_contract_manager)} {
}

xaccount_vm_execution_result_t xtop_account_vm::execute(std::vector<data::xcons_transaction_ptr_t> const & txs,
                                                        xobject_ptr_t<base::xvbstate_t> block_state,
                                                        const data::xblock_consensus_para_t & cs_para) {
    xaccount_vm_execution_result_t result;
    result.transaction_results.reserve(txs.size());

    state_accessor::xstate_access_control_data_t ac_data;  // final get from config or program initialization start
    contract_common::properties::xproperty_access_control_t ac{make_observer(block_state.get()), ac_data};
    contract_common::xcontract_execution_param_t param(cs_para);

#if 0
    auto const & actions = contract_runtime::xaction_generator_t::generate(txs);
#else
    std::vector<std::unique_ptr<data::xbasic_top_action_t>> basic_action;
    for (auto const & tx : txs) {
        std::unique_ptr<data::xbasic_top_action_t> a = top::make_unique<data::xconsensus_action_t<data::xtop_action_type_t::system>>(tx);
        basic_action.push_back(std::move(a));
    }

#endif
    auto i = 0u;
    try {
        for (i = 0u; i < basic_action.size(); ++i) {
            switch (basic_action[i]->type()) {
            case data::xtop_action_type_t::system:
            {
                auto const & action = dynamic_cast<data::xsystem_consensus_action_t const &>(*basic_action[i]);
                contract_common::xcontract_state_t contract_state{ action.contract_address(), make_observer(std::addressof(ac)) };
                result.transaction_results[i] = sys_action_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_action(action, param);
                break;
            }

            case data::xtop_action_type_t::user:
            {
                // auto const & action = dynamic_cast<data::xuser_consensus_action_t const &>(actions[i]);
                // contract_common::xcontract_state_t contract_state{ action.contract_address(), make_observer(std::addressof(ac)) };
                // result.transaction_results[i] = user_action_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_action(action, param);
                break;
            }

            case data::xtop_action_type_t::event:
            {
                // TODO: we don't have event action either. this is just a placeholder.
                break;
            }

            default:
            {
                assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
                result.status.ec = error::xerrc_t::invalid_contract_type;
                break;
            }
            }

            if (result.transaction_results[i].status.ec) {
                for (auto j = i + 1; j < result.transaction_results.size(); ++j) {
                    result.transaction_results[j].status.ec = error::xerrc_t::transaction_execution_abort;
                }

                result.status.ec = error::xerrc_t::transaction_execution_abort;
                break;
            }
        }
    } catch (top::error::xtop_error_t & eh) {
        xerror("account_vm: caught chain error exception: category: %s msg: %s", eh.code().category().name(), eh.what());
    } catch (std::exception const & eh) {
        xerror("account_vm: caught exception: %s", eh.what());
    }

    for (auto j = i; j < result.transaction_results.size(); ++j) {
        result.transaction_results[j].status.ec = error::xerrc_t::transaction_execution_abort;
    }

    if (i < result.transaction_results.size()) {
        result.status.ec = error::xerrc_t::transaction_execution_abort;
    }

    return result;
}

NS_END2
