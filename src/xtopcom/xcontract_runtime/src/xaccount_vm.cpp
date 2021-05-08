// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xaccount_vm.h"

#include "xbase/xlog.h"
#include "xbasic/xerror/xchain_error.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xerror/xerror.h"
#include "xcontract_runtime/xaction_session.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xcontract_runtime/xtop_action_generator.h"
#include "xdata/xconsensus_action.h"

#include <memory>

NS_BEG2(top, contract_runtime)

xaccount_vm_execution_result_t xtop_account_vm::execute(std::vector<data::xcons_transaction_ptr_t> const & txs, xobject_ptr_t<base::xvbstate_t> block_state) {
    xaccount_vm_execution_result_t result;
    result.transaction_results.reserve(txs.size());

    contract_common::properties::xproperty_access_control_data_t ac_data; // final get from config or program initialization start
    contract_common::properties::xproperty_access_control_t ac{ make_observer(block_state.get()), ac_data };

    auto const & actions = xaction_generator_t::generate(txs);
    auto i = 0u;
    try {
        for (i = 0u; i < actions.size(); ++i) {
            switch (actions[i].type()) {
            case data::xtop_action_type_t::system:
            {
                break;
            }

            case data::xtop_action_type_t::user:
            {
                auto const & action = dynamic_cast<data::xconsensus_action_t<data::xtop_action_type_t::user> const &>(actions[i]);
                contract_common::xcontract_state_t contract_state{ action.contract_address(), make_observer(std::addressof(ac)) };
                result.transaction_results[i] = user_action_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_action(action);
                break;
            }

            case data::xtop_action_type_t::kernel:
            {
                // TODO: in fact, we don't have kernel actions. this is just a placeholder.
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
                    result.transaction_results[j].status.ec = contract_runtime::error::xerrc_t::transaction_execution_abort;
                }

                result.status.ec = contract_runtime::error::xerrc_t::transaction_execution_abort;
                break;
            }
        }
    } catch (top::error::xtop_error_t & eh) {
        xerror("account_vm: caught chain error exception: category: %s msg: %s", eh.code().category().name(), eh.what());
    } catch (std::exception const & eh) {
        xerror("account_vm: caught exception: %s", eh.what());
    }

    for (auto j = i; j < result.transaction_results.size(); ++j) {
        result.transaction_results[j].status.ec = contract_runtime::error::xerrc_t::transaction_execution_abort;
    }

    if (i < result.transaction_results.size()) {
        result.status.ec = contract_runtime::error::xerrc_t::transaction_execution_abort;
    }

    return result;
}

NS_END2
