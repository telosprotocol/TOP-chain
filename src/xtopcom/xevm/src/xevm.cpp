// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm/xevm.h"

#include "assert.h"
#include "xdata/xconsensus_action.h"
#include "xdata/xtop_action.h"
#include "xdata/xtop_action_generator.h"
#include "xevm_contract_runtime/xevm_action_session.h"

NS_BEG2(top, evm)

xtop_evm::xtop_evm(observer_ptr<contract_runtime::evm::xevm_contract_manager_t> const evm_contract_manager,
                   statectx::xstatectx_face_ptr_t const evm_statectx)
  : evm_statectx_{evm_statectx}
  , evm_action_runtime_{top::make_unique<contract_runtime::evm::xevm_action_runtime_t>(evm_contract_manager, evm_statectx)} {
}

xevm_output_t xtop_evm::execute(std::vector<data::xcons_transaction_ptr_t> const & txs) {
    // only for single tx present
    assert(txs.size() == 1);

    const size_t result_size = txs.size();
    const std::vector<data::xcons_transaction_ptr_t> txs_for_actions(txs);
    auto action = contract_runtime::xaction_generator_t::generate(txs_for_actions.at(0));

    xevm_execution_result_t result;
    result.transaction_results.reserve(result_size);

    try {
        auto action_result = execute_action(std::move(action));
        result.transaction_results.emplace_back(action_result);
        if (!action_result.status.ec) {
            xwarn(
                "[xtop_evm::execute] tx failed, category: %s, msg: %s, abort all txs after!", action_result.status.ec.category().name(), action_result.status.ec.message().c_str());
            result.status.ec = action_result.status.ec;
        }
    } catch (top::error::xtop_error_t & eh) {
        xerror("xtop_evm: caught chain error exception: category: %s msg: %s", eh.code().category().name(), eh.what());
    } catch (std::exception const & eh) {
        xerror("xtop_evm: caught exception: %s", eh.what());
    }

    xevm_output_t output;
    std::vector<data::xcons_transaction_ptr_t> output_txs(txs);
    for (size_t i = 0; i < result.transaction_results.size(); i++) {
        auto const & r = result.transaction_results[i];
        auto & tx = output_txs[i];
        if (r.status.ec) {
            xwarn("[xtop_evm::execute] tx (%s) failed, category: %s, msg: %s, add to failed assemble", tx->get_tx_hash().c_str(), r.status.ec.category().name(), r.status.ec.message().c_str());
            tx->set_current_exec_status(data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_fail);
            output.failed_tx_assemble.emplace_back(tx);
        } else {
            if (!r.output.receipt_data.empty()) {
                tx->set_receipt_data(r.output.receipt_data);
            }
            xdbg("[xtop_evm::execute] tx (%s) add to success assemble", tx->get_tx_hash().c_str());
            tx->set_current_exec_status(data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
            output.success_tx_assemble.emplace_back(tx);
        }
    }

    // TODO: need to add gas output
    return output;
}

contract_runtime::xtransaction_execution_result_t xtop_evm::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action) {
    assert(action->type() == data::xtop_action_type_t::evm);
    auto const * cons_action = static_cast<data::xevm_consensus_action_t const *>(action.get());
    evm_runtime::xevm_state_t evm_state{cons_action->contract_address(), evm_statectx_};
    return evm_action_runtime_->new_session(make_observer(std::addressof(evm_state)))->execute_action(std::move(action));
}

NS_END2