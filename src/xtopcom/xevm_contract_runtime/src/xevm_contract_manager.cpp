// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_contract_manager.h"

#include "xbasic/xhex.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xnative_contract_address.h"
#include "xevm_contract_runtime/sys_contract/xdelegate_eth_contract.h"
#include "xevm_contract_runtime/sys_contract/xdelegate_top_contract.h"
#include "xevm_contract_runtime/sys_contract/xdelegate_usdc_contract.h"
#include "xevm_contract_runtime/sys_contract/xdelegate_usdt_contract.h"
#include "xevm_contract_runtime/sys_contract/xevm_bsc_client_contract.h"
#include "xevm_contract_runtime/sys_contract/xevm_eth2_client_contract.h"
#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"
#include "xevm_contract_runtime/sys_contract/xevm_heco_client_contract.h"
#if defined(XCXX20)
#include "xevm_runner/proto/ubuntu/proto_precompile.pb.h"
#else
#include "xevm_runner/proto/centos/proto_precompile.pb.h"
#endif

#include <cinttypes>

NS_BEG3(top, contract_runtime, evm)

xtop_evm_contract_manager::xtop_evm_contract_manager() {
    add_sys_contract(evm_top_contract_address, top::make_unique<sys_contract::xdelegate_top_contract_t>());
    add_sys_contract(evm_eth_contract_address, top::make_unique<sys_contract::xdelegate_eth_contract_t>());
    add_sys_contract(evm_usdc_contract_address, top::make_unique<sys_contract::xdelegate_usdc_contract_t>());
    add_sys_contract(evm_usdt_contract_address, top::make_unique<sys_contract::xdelegate_usdt_contract_t>());
    add_sys_contract(evm_eth_bridge_contract_address, top::make_unique<sys_contract::xtop_evm_eth_bridge_contract>());
    add_sys_contract(evm_bsc_client_contract_address, top::make_unique<sys_contract::xtop_evm_bsc_client_contract>());
    add_sys_contract(evm_heco_client_contract_address, top::make_unique<sys_contract::xtop_evm_heco_client_contract>());
#ifdef ETH2_SEPOLIA
    add_sys_contract(evm_eth2_client_contract_address, top::make_unique<sys_contract::xtop_evm_eth2_client_contract>(evm_common::eth2::xnetwork_id_t::sepolia));
#else
    add_sys_contract(evm_eth2_client_contract_address, top::make_unique<sys_contract::xtop_evm_eth2_client_contract>());
#endif
}

void xtop_evm_contract_manager::add_sys_contract(common::xaccount_address_t const & contract_address, std::unique_ptr<xevm_syscontract_face_t> contract) {
    m_sys_contract.insert(std::make_pair(contract_address, std::move(contract)));
}

const std::unordered_map<common::xaccount_address_t, std::unique_ptr<xevm_syscontract_face_t>> & xtop_evm_contract_manager::get_sys_contracts() const {
    return m_sys_contract;
}

bool xtop_evm_contract_manager::execute_sys_contract(xbytes_t const & input, observer_ptr<statectx::xstatectx_face_t> state_ctx, xbytes_t & output) {
    top::evm_engine::precompile::ContractBridgeArgs call_args;

    auto ret = call_args.ParseFromString(top::to_string(input));
    if (!ret) {
        // todo need to add default return err into output;
        xwarn("[xtop_evm_contract_manager::execute_sys_contract] parse input error");
        return false;
    }
    std::string contract_address_str = top::to_hex(call_args.contract_address().value().begin(), call_args.contract_address().value().end(), base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN);
    // todo might check address.
    common::xaccount_address_t sys_contract_address{contract_address_str};
    if (m_sys_contract.find(sys_contract_address) == m_sys_contract.end()) {
        // todo need to add return err into output;
        xwarn("[xtop_evm_contract_manager::execute_sys_contract] cannot find sys_contract: %s", sys_contract_address.to_string().c_str());
        return false;
    }

    sys_contract_precompile_output contract_output;
    sys_contract_precompile_error contract_err;
    try {
        xdbg("call contract %s with input %s", contract_address_str.c_str(), to_hex(call_args.input()).c_str());
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
                address->set_value(contract_output.logs[i].address.to_string());

                log->set_data(top::to_string(contract_output.logs[i].data));

                for (std::size_t j = 0; j < contract_output.logs[i].topics.size(); ++j) {
                    auto topic = log->add_topics();
                    topic->set_data(top::to_string(contract_output.logs[i].topics[j].asBytes()));
                }
            }
            output = top::to_bytes(return_output.SerializeAsString());
            xdbg("[xtop_evm_contract_manager::execute_sys_contract] sys_contract: %s execute success", sys_contract_address.to_string().c_str());
            return true;
        } else {
            top::evm_engine::precompile::PrecompileFailure return_error;
            return_error.set_fail_status(static_cast<uint32_t>(contract_err.fail_status));
            return_error.set_minor_status(contract_err.minor_status);
            return_error.set_cost(contract_err.cost);
            return_error.set_output(top::to_string(contract_err.output));
            output = top::to_bytes(return_error.SerializeAsString());
            xdbg("[xtop_evm_contract_manager::execute_sys_contract] sys_contract: %s execute error", sys_contract_address.to_string().c_str());
            return false;
        }
    } catch (top::error::xtop_error_t const & eh) {
        top::evm_engine::precompile::PrecompileFailure return_error;
        return_error.set_fail_status(static_cast<uint32_t>(precompile_error::fatal));
        return_error.set_minor_status(static_cast<uint32_t>(precompile_error_ExitFatal::Other));

        output = top::to_bytes(return_error.SerializeAsString());

        xerror("execute_sys_contract failed with err:%" PRIi32 " category:%s msg:%s", eh.code().value(), eh.category().name(), eh.what());

        return false;
    }
}

NS_END3
