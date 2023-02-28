// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtvm_runtime/xtvm_action_runner.h"

#include "protobuf_types/pparameters.pb.h"
#include "xcommon/xeth_address.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xtvm_engine_rs/tvm-c-api/tvm_engine_interface.h"
#include "xtvm_engine_rs/tvm-c-api/tvm_import_instance.h"
#include "xtvm_runtime/xerror.h"
#include "xtvm_runtime/xtvm_logic.h"
#include "xtvm_runtime/xtvm_storage.h"

NS_BEG2(top, tvm)

xtop_vm_action_runner::xtop_vm_action_runner(statectx::xstatectx_face_ptr_t const statectx) : m_statectx{statectx} {
}

evm_common::xevm_transaction_result_t xtop_vm_action_runner::execute_action(std::unique_ptr<xtvm_context_t> context) {
    evm_common::xevm_transaction_result_t result;

    auto storage = top::make_unique<xtvm_storage_t>(m_statectx);
    std::shared_ptr<xtvm_logic_t> logic_ptr = std::make_shared<xtvm_logic_t>(top::make_observer(storage.get()), top::make_observer(context.get()));

    tvm_import_instance::instance()->add_logic(logic_ptr);

    // call vm engine in rust.
    bool bool_result = call();

    // fetch result
    auto return_result_bytes = logic_ptr->get_return_value();

    xdbg("xtop_vm_action_runner::execute_action result %s return_bytes_size: %zu", bool_result ? "success" : "fail", return_result_bytes.size());

    tvm_import_instance::instance()->remove_logic();

    tvm_engine::parameters::PReturnResult return_result;
    auto ret = return_result.ParseFromString(top::to_string(return_result_bytes));

    if (!ret) {
        top::error::throw_error(top::tvm::error::xerrc_t::protobuf_serilized_error);
    }
    result.set_status(return_result.status());
    result.extra_msg = top::to_hex_prefixed(return_result.status_data());
    result.used_gas = return_result.gas_used();
    // logs:
    for (int i = 0; i < return_result.logs_size(); ++i) {
        common::xeth_address_t address = common::xeth_address_t::build_from(top::to_bytes(return_result.logs(i).address().value()));
        xbytes_t data = top::to_bytes(return_result.logs(i).data());
        evm_common::xh256s_t topics;
        for (int j = 0; j < return_result.logs(i).topics_size(); ++j) {
            topics.push_back(evm_common::xh256_t(top::to_bytes(return_result.logs(i).topics(j).data())));
        }
        evm_common::xevm_log_t log(address, topics, data);
        result.logs.push_back(log);
    }

    return result;
}

NS_END2