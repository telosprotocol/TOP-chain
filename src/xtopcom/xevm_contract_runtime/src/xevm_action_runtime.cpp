// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_runtime.h"

#include "xbasic/xhex.h"
#include "xcommon/xeth_address.h"
#include "xevm_contract_runtime/xevm_action_session.h"
#include "xevm_contract_runtime/xevm_context.h"
#include "xevm_contract_runtime/xevm_logic.h"
#include "xevm_contract_runtime/xevm_storage.h"
#include "xevm_runner/evm_engine_interface.h"
#include "xevm_runner/evm_import_instance.h"
#if defined(XCXX20)
#include "xevm_runner/proto/ubuntu/proto_parameters.pb.h"
#else
#include "xevm_runner/proto/centos/proto_parameters.pb.h"
#endif

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
    auto default_token_type = XGET_CONFIG(evm_token_type);
    if(default_token_type.empty()) {
        xinfo("xtop_action_runtime<data::execute default_token_type is empty.");
        xassert(false);
    }
    auto storage = top::make_unique<evm::xevm_storage>(m_evm_statectx, default_token_type);
    std::shared_ptr<top::evm::xevm_logic_face_t> logic_ptr =
        std::make_shared<top::contract_runtime::evm::xevm_logic_t>(std::move(storage), m_evm_statectx, tx_ctx, evm_contract_manager_);
    top::evm::evm_import_instance::instance()->add_evm_logic(logic_ptr);

    bool evm_result{true};

    // if deploy, get code and src from action, call 'deploy_code()'
    if (tx_ctx->action_type() == data::xtop_evm_action_type::deploy_contract) {
        evm_result = deploy_code();
    }
    // if call, get code from evm manager(lru_cache) or state(state_accessor), get src and target address, call 'call_contract()'
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

        auto ret = return_result.ParseFromString(top::to_string(top::evm::evm_import_instance::instance()->get_return_value()));

        if (!ret || return_result.version() != evm_runtime::CURRENT_CALL_ARGS_VERSION) {
            top::error::throw_error(error::xerrc_t::evm_protobuf_serilized_error);
        }

        // status:
        result.set_status(return_result.transaction_status());

        // extra_msg:
        result.extra_msg = top::to_hex_prefixed(return_result.status_data());
        xdbg("xtop_action_runtime<data::xevm_consensus_action_t>::execute result.extra_msg:%s", result.extra_msg.c_str());

        // logs:
        for (int i = 0; i < return_result.logs_size(); ++i) {
            common::xeth_address_t address = common::xeth_address_t::build_from(top::to_bytes(return_result.logs(i).address().value()));
            xbytes_t data = top::to_bytes(return_result.logs(i).data());
            xh256s_t topics;
            for (int j = 0; j < return_result.logs(i).topics_size(); ++j) {
                topics.push_back(xh256_t(top::to_bytes(return_result.logs(i).topics(j).data())));
            }
            evm_common::xevm_log_t log(address, topics, data);
            result.logs.push_back(log);
        }
        // used_gas:
        result.used_gas = return_result.gas_used();
    }

    top::evm::evm_import_instance::instance()->remove_evm_logic();

    return result;
}

NS_END2
