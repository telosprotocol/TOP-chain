// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_runtime.h"

#include "xevm_contract_runtime/xevm_action_session.h"
#include "xevm_contract_runtime/xevm_context.h"
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

    auto storage = std::make_shared<evm::xevm_storage>(m_evm_statectx);
    std::unique_ptr<top::evm::xevm_logic_face_t> logic_ptr = top::make_unique<top::contract_runtime::evm::xevm_logic_t>(storage, tx_ctx);
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
        result.used_gas = error_result.second; // todo: now got it . but throw won't be able to pass this value.
        // todo check begin + error_result > max?
        auto ec = static_cast<error::xerrc_t>(error_result.first + static_cast<std::underlying_type<error::xerrc_t>::type>(error::xerrc_t::evm_vm_ec_begin));
        top::error::throw_error(ec);
    }

    top::evm_engine::parameters::SubmitResult return_result;

    auto ret = return_result.ParseFromString(evm::xvariant_bytes{top::evm::evm_import_instance::instance()->get_return_value()}.to_string());

    if (!ret || return_result.version() != evm_runtime::CURRENT_CALL_ARGS_VERSION) {
        top::error::throw_error(error::xerrc_t::evm_protobuf_serilized_error);
    }

    // status:
    result.set_status(return_result.transaction_status());

    // extra_msg:
    result.extra_msg = evm::xvariant_bytes{return_result.status_data(), false}.to_hex_string();

    // logs:
    for (int i = 0; i < return_result.logs_size(); ++i) {
        evm_common::xevm_log_t log;
        log.address = return_result.logs(i).address().value();
        log.data = top::to_bytes(return_result.logs(i).data());
        std::vector<xbytes_t> topic;
        for (int j = 0; j < return_result.logs(i).topics_size(); ++j) {
            topic.push_back(top::to_bytes(return_result.logs(i).topics(j).data()));
        }
        log.topics = topic;
        result.logs.push_back(log);
    }
    // used_gas:
    result.used_gas = return_result.gas_used();

    // } catch (top::error::xtop_error_t const & eh) {
    //     result.status.ec = eh.code();
    // } catch (std::exception const & eh) {
    //     result.status.ec = error::xerrc_t::unknown_error;
    //     result.status.extra_msg = eh.what();
    // } catch (enum_xerror_code ec) {
    //     result.status.ec = ec;
    // } catch (...) {
    //     result.status.ec = error::xerrc_t::unknown_error;
    // }

    return result;
}

NS_END2