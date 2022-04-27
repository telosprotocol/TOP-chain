// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_runtime.h"

#include "xevm_contract_runtime/xevm_action_session.h"
#include "xevm_contract_runtime/xevm_context.h"
#include "xevm_contract_runtime/xevm_logic.h"
#include "xevm_contract_runtime/xevm_logic.h"
#include "xevm_contract_runtime/xevm_storage.h"
#include "xevm_contract_runtime/xevm_type.h"
#include "xevm_contract_runtime/xevm_variant_bytes.h"
#include "xevm_runner/evm_engine_interface.h"
#include "xevm_runner/evm_import_instance.h"
#include "xevm_runner/proto/proto_parameters.pb.h"

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

evm_common::xevm_transaction_result_t xtop_action_runtime<data::xevm_consensus_action_t>::execute(observer_ptr<evm_runtime::xevm_context_t> tx_ctx) {
    evm_common::xevm_transaction_result_t result;

    // try {

    auto storage = top::make_unique<evm::xevm_storage>(m_evm_statectx);
    std::unique_ptr<top::evm::xevm_logic_face_t> logic_ptr = top::make_unique<top::contract_runtime::evm::xevm_logic_t>(std::move(storage), m_evm_statectx, tx_ctx, evm_contract_manager_);
    top::evm::evm_import_instance::instance()->set_evm_logic(std::move(logic_ptr));

    bool evm_result{true};

    // if deploy, get code and src from action, set_evm_logic, call 'deploy_code()'
    if (tx_ctx->action_type() == data::xtop_evm_action_type::deploy_contract) {
        evm_result = deploy_code();
    }
    // if call, get code from evm manager(lru_cache) or state(state_accessor), get src and target address, set_evm_logic, call 'call_contract()'
    else if (tx_ctx->action_type() == data::xtop_evm_action_type::call_contract) {
        evm_result = call_contract();
    } else {
        xassert(false);
    }
    if (!evm_result) {
        auto error_result = top::evm::evm_import_instance::instance()->get_return_error();
        xinfo("[evm_action] evm execute fail. ec code %d  used_gas: %lu", error_result.first, error_result.second);

        result.used_gas = error_result.second;
        result.status = evm_common::xevm_transaction_status_t::OtherExecuteError;

    } else {
        top::evm_engine::parameters::SubmitResult return_result;

        auto ret = return_result.ParseFromString(evm::xvariant_bytes{top::evm::evm_import_instance::instance()->get_return_value()}.to_string());

        if (!ret || return_result.version() != evm_runtime::CURRENT_CALL_ARGS_VERSION) {
            top::error::throw_error(error::xerrc_t::evm_protobuf_serilized_error);
        }

        // status:
        result.set_status(return_result.transaction_status());

        // extra_msg:
        result.extra_msg = evm::xvariant_bytes{return_result.status_data(), false}.to_hex_string("0x");
        xdbg("xtop_action_runtime<data::xevm_consensus_action_t>::execute result.extra_msg:%s", result.extra_msg.c_str());

        // logs:
        for (int i = 0; i < return_result.logs_size(); ++i) {
            evm_common::xevm_log_t log;
            log.address = evm::xvariant_bytes{return_result.logs(i).address().value(), false}.to_hex_string("0x");
            log.data = evm::xvariant_bytes{return_result.logs(i).data(), false}.to_hex_string("0x");
            // log.data = top::to_bytes(return_result.logs(i).data());
            std::vector<std::string> topic;
            for (int j = 0; j < return_result.logs(i).topics_size(); ++j) {
                topic.push_back(evm::xvariant_bytes{return_result.logs(i).topics(j).data(), false}.to_hex_string("0x"));
                // topic.push_back(top::to_bytes(return_result.logs(i).topics(j).data()));
            }
            log.topics = topic;
            result.logs.push_back(log);
        }
        // used_gas:
        result.used_gas = return_result.gas_used();
    }

    return result;
}

NS_END2
