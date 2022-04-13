// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_runtime.h"

#include "xevm_contract_runtime/xevm_action_session.h"
#include "xevm_contract_runtime/xevm_logic.h"
#include "xevm_contract_runtime/xevm_storage.h"
#include "xevm_contract_runtime/xevm_type.h"
#include "xevm_runner/evm_engine_interface.h"
#include "xevm_runner/evm_import_instance.h"
NS_BEG2(top, contract_runtime)

xtop_action_runtime<data::xevm_consensus_action_t>::xtop_action_runtime(observer_ptr<evm::xevm_contract_manager_t> const evm_contract_manager,
                                                                        statectx::xstatectx_face_ptr_t const statectx) noexcept
  : evm_contract_manager_{evm_contract_manager}, m_evm_statectx{statectx} {
}

// std::unique_ptr<xaction_session_t<data::xevm_consensus_action_t>> xtop_action_runtime<data::xevm_consensus_action_t>::new_session(observer_ptr<evm_runtime::xevm_state_t>
// sender_state) {
std::unique_ptr<xaction_session_t<data::xevm_consensus_action_t>> xtop_action_runtime<data::xevm_consensus_action_t>::new_session() {
    return top::make_unique<xaction_session_t<data::xevm_consensus_action_t>>(top::make_observer(this));
}

xtransaction_execution_result_t xtop_action_runtime<data::xevm_consensus_action_t>::execute(observer_ptr<evm_runtime::xevm_context_t> tx_ctx) {
    xtransaction_execution_result_t result;

    try {
        // mock:
        auto storage = std::make_shared<evm::xevm_storage>(m_evm_statectx);
        // auto tx_type = tx_ctx->type();
        // 1. get action type: deploy/call/transfer
        // 2. if deploy, get code and src from action, set_evm_logic, call 'deploy_code()'
        if (tx_ctx->action_type() == evm_runtime::xtop_evm_action_type::deploy_contract) {
            std::unique_ptr<top::evm::xevm_logic_face_t> logic_ptr = top::make_unique<top::contract_runtime::evm::xevm_logic_t>(storage, tx_ctx);
            top::evm::evm_import_instance::instance()->set_evm_logic(std::move(logic_ptr));
            deploy_code();
        }
        // 3. if call, get code from evm manager(lru_cache) or state(state_accessor), get src and target address, set_evm_logic, call 'call_contract()'
        else if (tx_ctx->action_type() == evm_runtime::xtop_evm_action_type::call_contract) {
            std::unique_ptr<top::evm::xevm_logic_face_t> logic_ptr = top::make_unique<top::contract_runtime::evm::xevm_logic_t>(storage, tx_ctx);
            top::evm::evm_import_instance::instance()->set_evm_logic(std::move(logic_ptr));
            call_contract();
        } else {
            xassert(false);
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