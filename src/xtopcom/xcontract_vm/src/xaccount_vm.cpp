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

std::vector<xaccount_vm_execution_result_t> xtop_account_vm::execute(std::vector<xaccount_vm_execution_assemble_t> & assemble) {
    std::vector<xaccount_vm_execution_result_t> result;
    // TODO lon: here can be parallel processing
    for (auto & item : assemble) {
        result.emplace_back(execute(item.txs, item.block_state, item.cs_para));
    }
    return result;
}

xaccount_vm_execution_result_t xtop_account_vm::execute(std::vector<data::xcons_transaction_ptr_t> const & txs,
                                                        xobject_ptr_t<base::xvbstate_t> block_state,
                                                        data::xblock_consensus_para_t const & cs_para) {
    xaccount_vm_execution_result_t result;
    const size_t result_size = txs.size();
    // result.transaction_results.resize(result_size);
    assert(result_size == 1);

    state_accessor::xstate_access_control_data_t ac_data;  // final get from config or program initialization start
    contract_common::xcontract_execution_param_t param(cs_para);
    contract_common::properties::xproperty_access_control_t ac{make_observer(block_state.get()), ac_data, param};

    auto actions = contract_runtime::xaction_generator_t::generate(txs);
    assert(actions.size() == result_size);

    size_t i = 0;
    try {
        for (i = 0; i < result_size; i++) {
            auto action_result = execute_action(std::move(actions[i]), ac, param);
            result.transaction_results.emplace_back(action_result);
            assert(result.transaction_results.size() == (i + 1));
            if (action_result.status.ec) {
                abort(i + 1, result_size, result);
                result.status.ec = action_result.status.ec;
                i = result_size;
                break;
            }
        }
    } catch (top::error::xtop_error_t & eh) {
        xerror("account_vm: caught chain error exception: category: %s msg: %s", eh.code().category().name(), eh.what());
    } catch (std::exception const & eh) {
        xerror("account_vm: caught exception: %s", eh.what());
    }
    // exception
    if (i < result_size) {
        abort(i, result_size, result);
        result.status.ec = error::xerrc_t::transaction_execution_abort;
    }
    ac.create_time();
    std::error_code ec;
    ac.update_latest_sendtx_hash(ec);
    ac.update_latest_sendtx_nonce(ec);
    result.binlog = ac.binlog();
    result.contract_state_snapshot = ac.fullstate_bin();

    return result;
}

contract_runtime::xtransaction_execution_result_t xtop_account_vm::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action,
                                                                                  contract_common::properties::xproperty_access_control_t & ac,
                                                                                  contract_common::xcontract_execution_param_t const & param) {
    contract_runtime::xtransaction_execution_result_t result;

    switch (action->type()) {
    case data::xtop_action_type_t::system:
    {
        auto const * cons_action = static_cast<data::xsystem_consensus_action_t const *>(action.get());
        contract_common::xcontract_state_t contract_state{ cons_action->contract_address(), make_observer(std::addressof(ac)) };
        result = sys_action_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_action(std::move(action), param);
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
        xerror("[xtop_account_vm::execute_action] error action type: %d", action->type());
        assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        result.status.ec = error::xerrc_t::invalid_contract_type;
        break;
    }
    }

    for (auto & follow_up : result.output.followup_transaction_data) {
        if (follow_up.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::immediately) {
            if (follow_up.execute_type == contract_common::xenum_followup_transaction_execute_type::unexecuted) {
                auto follow_up_action = contract_runtime::xaction_generator_t::generate(follow_up.followed_transaction);
                auto follow_up_result = execute_action(std::move(follow_up_action), ac, param);
                if (follow_up_result.status.ec) {
                    result.status.ec = follow_up_result.status.ec;
                    // clear up all follow up txs
                    result.output.followup_transaction_data.clear();
                    break;
                }
                follow_up.execute_type = contract_common::xenum_followup_transaction_execute_type::success;
                for (auto & double_follow_up : follow_up_result.output.followup_transaction_data) {
                    result.output.followup_transaction_data.emplace_back(std::move(double_follow_up));
                }
            } else if (follow_up.execute_type == contract_common::xenum_followup_transaction_execute_type::success) {
                result.output.followup_transaction_data.emplace_back(std::move(follow_up));
            } else {
                xerror("[xtop_account_vm::execute_action] error follow up tx execute type: %d", follow_up.execute_type);
                assert(false);
            }
        } else if (follow_up.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::delay) {
            if (follow_up.execute_type != contract_common::xenum_followup_transaction_execute_type::unexecuted) {
                xerror("[xtop_account_vm::execute_action] error follow up tx execute type: %d", follow_up.execute_type);
                assert(false);
            }
            result.output.followup_transaction_data.emplace_back(std::move(follow_up));
        } else {
            xerror("[xtop_account_vm::execute_action] error follow up tx schedule type: %d", follow_up.schedule_type);
            assert(false);
        }
    }

    return result;
}

void xtop_account_vm::abort(const size_t start_index, const size_t size, xaccount_vm_execution_result_t & result) {
    contract_runtime::xtransaction_execution_result_t abort_result;
    abort_result.status.ec = error::xerrc_t::transaction_execution_abort;
    for (auto i = start_index + 1; i < size; i++) {
        result.transaction_results.emplace_back(abort_result);
        assert(result.transaction_results.size() == (i + 1));
    }
}

NS_END2
