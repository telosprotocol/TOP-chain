// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_runtime.h"


#include "xevm_contract_runtime/xevm_action_session.h"
#include "xevm_contract_runtime/xevm_storage.h"
#include "xevm_contract_runtime/xevm_type.h"
#include "xevm_runner/evm_engine_interface.h"
#include "xevm_runner/evm_import_instance.h"
#include "xevm_runner/evm_logic.h"
NS_BEG2(top, contract_runtime)

xtop_action_runtime<data::xevm_consensus_action_t>::xtop_action_runtime(observer_ptr<evm::xevm_contract_manager_t> const evm_contract_manager,
                                                                        statectx::xstatectx_face_ptr_t const statectx) noexcept
  : evm_contract_manager_{evm_contract_manager}, evm_statectx_{statectx} {
}

// todo this state? should be user's or contract's. User's account state
std::unique_ptr<xaction_session_t<data::xevm_consensus_action_t>> xtop_action_runtime<data::xevm_consensus_action_t>::new_session(observer_ptr<evm_runtime::xevm_state_t> evm_state) {
    return top::make_unique<xaction_session_t<data::xevm_consensus_action_t>>(top::make_observer(this), evm_state);
}

xtransaction_execution_result_t xtop_action_runtime<data::xevm_consensus_action_t>::execute(observer_ptr<evm_runtime::xevm_context_t> tx_ctx) {
    xtransaction_execution_result_t result;

    try {
        // mock:
        auto storage = std::make_shared<evm::xevm_storage>(evm_statectx_);
        // auto tx_type = tx_ctx->type();
        // 1. get action type: deploy/call/transfer
        // 2. if deploy, get code and src from action, set_evm_logic, call 'deploy_code()'
        if (0) {
            // auto code = tx_ctx->data();
            // tx_ctx->set_input(code);
            top::evm::xtop_evm_logic evm_logic{storage, tx_ctx};
            top::evm::evm_import_instance::instance()->set_evm_logic(evm_logic);
            deploy_code();
        }
        // 3. if call, get code from evm manager(lru_cache) or state(state_accessor), get src and target address, set_evm_logic, call 'call_contract()'
        else if (0) {
            // auto code = evm_contract_manager_->code(tx_ctx->sender());
            // auto data = tx_ctx->data();
            // tx_ctx->set_input(data);
            top::evm::xtop_evm_logic evm_logic{storage, tx_ctx};
            top::evm::evm_import_instance::instance()->set_evm_logic(evm_logic);
            call_contract();
        }
        // 4. if transfer, ?
        else if (0) {

        // } else {
        //     xassert(false);
        }
    } catch (top::error::xtop_error_t const & eh) {
        result.status.ec = eh.code();
    } catch (std::exception const & eh) {
        result.status.ec = error::xerrc_t::unknown_error;
        result.status.extra_msg = eh.what();
    } catch (enum_xerror_code ec) {
        result.status.ec = ec;
    } catch (...) {
        result.status.ec = error::xerrc_t::unknown_error;
    }

    return result;
}

NS_END2