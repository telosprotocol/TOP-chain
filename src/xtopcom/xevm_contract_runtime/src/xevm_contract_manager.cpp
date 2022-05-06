// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_contract_manager.h"

#include "xbasic/xmemory.hpp"
#include "xevm_contract_runtime/sys_contract/xevm_erc20_contract.h"
#include "xevm_contract_runtime/xevm_variant_bytes.h"
#include "xevm_runner/proto/proto_precompile.pb.h"

#include <cinttypes>

NS_BEG3(top, contract_runtime, evm)

xtop_evm_contract_manager::xtop_evm_contract_manager() {
    add_sys_contract(common::xaccount_address_t{"T60004ff00000000000000000000000000000000000001"}, top::make_unique<sys_contract::xtop_evm_erc20_sys_contract>());
}

void xtop_evm_contract_manager::add_sys_contract(common::xaccount_address_t const & contract_address, std::unique_ptr<xevm_syscontract_face_t> contract) {
    m_sys_contract.insert(std::make_pair(contract_address, std::move(contract)));
}

bool xtop_evm_contract_manager::execute_sys_contract(xbytes_t const & input, observer_ptr<statectx::xstatectx_face_t> state_ctx, xbytes_t & output) {
    top::evm_engine::precompile::ContractBridgeArgs call_args;

    auto ret = call_args.ParseFromString(top::to_string(input));
    if (!ret) {
        // todo need to add default return err into output;
        return false;
    }
    std::string contract_address_str = xvariant_bytes{call_args.contract_address().value(), false}.to_hex_string("T60004");
    // todo might check address.
    common::xaccount_address_t sys_contract_address{contract_address_str};
    if (m_sys_contract.find(sys_contract_address) == m_sys_contract.end()) {
        // todo need to add return err into output;
        return false;
    }

    sys_contract_precompile_output contract_output;
    sys_contract_precompile_error contract_err;
    try {
        auto result = m_sys_contract.at(sys_contract_address)
                          ->execute(top::to_bytes(call_args.input()),           // NOLINIT
                                    call_args.target_gas(),                     // NOLINIT
                                    sys_contract_context{call_args.context()},  // NOLINIT
                                    call_args.is_static(),                      // NOLINIT
                                    state_ctx,
                                    contract_output,  // NOLINIT
                                    contract_err);    // NOLINIT

        if (result) {
            top::evm_engine::precompile::PrecompileOutput return_output;
            return_output.set_exit_status(static_cast<uint32_t>(contract_output.exit_status));
            return_output.set_cost(contract_output.cost);
            return_output.set_output(top::to_string(contract_output.output));
            for (std::size_t i = 0; i < contract_output.logs.size(); ++i) {
                auto * log = return_output.add_logs();
                auto address = log->mutable_address();
                address->set_value(contract_output.logs[i].address);

                log->set_data(contract_output.logs[i].data);

                for (std::size_t j = 0; j < contract_output.logs[i].topics.size(); ++j) {
                    auto topic = log->add_topics();
                    topic->set_data(contract_output.logs[i].topics[j]);
                }
            }
            output = top::to_bytes(return_output.SerializeAsString());
            return true;
        } else {
            top::evm_engine::precompile::PrecompileFailure return_error;
            return_error.set_fail_status(static_cast<uint32_t>(contract_err.fail_status));
            return_error.set_minor_status(contract_err.minor_status);
            return_error.set_cost(contract_err.cost);
            return_error.set_output(top::to_string(contract_err.output));
            output = top::to_bytes(return_error.SerializeAsString());
            return false;
        }
    } catch (top::error::xtop_error_t const & eh) {
        top::evm_engine::precompile::PrecompileFailure return_error;
        return_error.set_fail_status(static_cast<uint32_t>(precompile_error::Fatal));
        return_error.set_minor_status(precompile_error_ExitFatal::Other);

        output = top::to_bytes(return_error.SerializeAsString());

        xerror("execute_sys_contract failed with err:%" PRIi32 " category:%s msg:%s", eh.code().value(), eh.category().name(), eh.what());

        return false;
    }
}

NS_END3
