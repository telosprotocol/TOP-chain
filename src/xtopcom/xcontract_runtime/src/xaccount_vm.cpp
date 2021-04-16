#include "xcontract_runtime/xaccount_vm.h"

#include "xcontract_runtime/xerror/xerror.h"
#include "xcontract_common/xcontract_state.h"
#include "xdata/xconsensus_action.h"

#include <memory>

NS_BEG2(top, contract_runtime)

xaccount_vm_execution_result_t xtop_account_vm::execute(std::vector<data::xcons_transaction_ptr_t> const & txs, xobject_ptr_t<base::xvbstate_t> block_state) {
    xaccount_vm_execution_result_t result;
    result.transaction_results.reserve(txs.size());

    contract_common::properties::xproperty_access_control_data_t ac_data; // final get from config or program initialization start
    contract_common::properties::xproperty_access_control_t ac{make_observer(block_state.get()), ac_data};

    for (auto i = 0u; i < txs.size(); ++i) {
        auto raw_tx = txs[i]->get_transaction();
        raw_tx->add_ref();
        data::xtransaction_ptr_t tx;
        tx.attach(raw_tx);

        contract_common::xcontract_state_t contract_state{common::xaccount_address_t{tx->get_target_addr()}, make_observer(std::addressof(ac))};

        switch (common::xaccount_address_t{raw_tx->get_target_addr()}.type()) {
        case base::enum_vaccount_addr_type::enum_vaccount_addr_type_native_contract:
            result.transaction_results[i] = system_contract_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_transaction(txs[i]);
            break;

        case base::enum_vaccount_addr_type::enum_vaccount_addr_type_custom_contract:
            result.transaction_results[i] = user_contract_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_transaction(txs[i]);
            break;

        default:
            assert(false);
            result.status.ec = error::xerrc_t::invalid_contract_type;
            break;
        }

        if (result.transaction_results[i].status.ec) {
            for (auto j = i + 1; j < result.transaction_results.size(); ++j) {
                result.transaction_results[j].status.ec = contract_runtime::error::xerrc_t::transaction_execution_abort;
            }

            result.status.ec = contract_runtime::error::xerrc_t::transaction_execution_abort;
            break;
        }
    }

    return result;
}

xaccount_vm_execution_result_t xtop_account_vm::execute(std::vector<data::xtop_action_t> const & actions, xobject_ptr_t<base::xvbstate_t> block_state) {
    xaccount_vm_execution_result_t result;
    result.transaction_results.reserve(actions.size());

    contract_common::properties::xproperty_access_control_data_t ac_data; // final get from config or program initialization start
    contract_common::properties::xproperty_access_control_t ac{ make_observer(block_state.get()), ac_data };

    for (auto i = 0u; i < actions.size(); ++i) {
        auto const & action = actions[i];
        switch (action.type()) {
        case data::xtop_action_type_t::system_contract: {
            auto const & consensus_action = dynamic_cast<data::xconsensus_action_t const &>(action);
            contract_common::xcontract_state_t contract_state{ consensus_action.execution_address(), make_observer(std::addressof(ac)) };

            result.transaction_results[i] = system_contract_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_transaction();
            break;
        }

        case data::xtop_action_type_t::user_contract: {
            auto const & consensus_action = dynamic_cast<data::xconsensus_action_t const &>(action);
            contract_common::xcontract_state_t contract_state{ consensus_action.execution_address(), make_observer(std::addressof(ac)) };

            result.transaction_results[i] = user_contract_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_transaction(txs[i]);
            break;
        }

        default: {
            assert(false);
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

    return result;
}

NS_END2
