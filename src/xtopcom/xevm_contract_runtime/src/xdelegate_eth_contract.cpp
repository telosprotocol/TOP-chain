// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xdelegate_eth_contract.h"

#include "xbasic/xfixed_hash.h"
#include "xcommon/common_data.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xchain_uuid.h"
#include "xcommon/xeth_address.h"
#include "xdata/xnative_contract_address.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_contract_runtime/sys_contract/xdelegate_erc20_contract.h"
#include "xevm_contract_runtime/xerror/xerror.h"
#include "xevm_runner/evm_engine_interface.h"

#include <cinttypes>

NS_BEG4(top, contract_runtime, evm, sys_contract)

bool xtop_delegate_eth_contract::execute(xbytes_t input,
                                         uint64_t /*target_gas*/,
                                         sys_contract_context const & context,
                                         bool is_static,
                                         observer_ptr<statectx::xstatectx_face_t> state_ctx,
                                         sys_contract_precompile_output & output,
                                         sys_contract_precompile_error & err) {
    assert(state_ctx);

    // chain_uuid (1 byte) | erc20_method_id (4 bytes) | parameters (depends)
    if (input.empty()) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

        xwarn("precompiled eth contract: invalid input");

        return false;
    }

    common::xchain_uuid_t const chain_uuid{top::from_byte<common::xchain_uuid_t>(input.front())};
    if (chain_uuid != common::xchain_uuid_t::eth) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

        xwarn("precompiled eth contract: not supported token: %d", static_cast<int>(chain_uuid));

        return false;
    }

    std::error_code ec;
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::next(std::begin(input), 1), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

        xwarn("precompiled eth contract: illegal input data");

        return false;
    }

    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

        xwarn("precompiled eth contract: illegal input function selector");

        return false;
    }

    switch (function_selector.method_id) {
    case method_id_decimals: {
        xdbg("precompiled eth contract: decimals");

        output.exit_status = Returned;
        output.cost = 0;
        output.output = top::to_bytes(evm_common::u256{18});

        return true;
    }

    case method_id_mint: {
        xdbg("precompiled eth contract: mint");

        uint64_t constexpr mint_gas_cost = 3155;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = mint_gas_cost;
            err.output = result;

            xwarn("precompiled eth contract: mint is not allowed in static context");

            return false;
        }

        // Only controller can mint tokens.
        auto const & contract_state = state_ctx->load_unit_state(evm_eth_contract_address);
        auto const & msg_sender = common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & token_controller = contract_state->tep_token_controller(chain_uuid);
        if (msg_sender != token_controller) {
             err.fail_status = precompile_error::fatal;
             err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

             xwarn("precompiled eth contract: mint called by non-admin account %s", context.caller.c_str());

             return false;
        }

        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: mint with invalid parameter");

            return false;
        }

        auto const recver = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: mint with invalid receiver address");

            return false;
        }
        auto const recver_address = common::xaccount_address_t::build_from(recver, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 const value = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: mint with invalid value");

            return false;
        }

        if (!mint(recver, value)) {
            xwarn("mint eth failed");
            ec = evm_runtime::error::xerrc_t::precompiled_contract_erc20_burn;
        }

        if (!ec) {
            auto const & contract_address = context.address;
            auto const & recipient_address = recver;

            xh256s_t topics;
            topics.push_back(xh256_t(event_hex_string_transfer));
            topics.push_back(xh256_t(common::xeth_address_t::zero().to_h256()));
            topics.push_back(xh256_t(recipient_address.to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(value));
            result[31] = 1;

            output.cost = 0;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = mint_gas_cost;
            err.output = result;

            xwarn("precompiled eth contract: mint reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_burn_from: {
        xdbg("precompiled eth contract: burnFrom");

        uint64_t constexpr burn_gas_cost = 3155;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = burn_gas_cost;
            err.output = result;

            xwarn("precompiled eth contract: burnFrom is not allowed in static context");

            return false;
        }

        auto const & contract_state = state_ctx->load_unit_state(evm_eth_contract_address);
        auto const & msg_sender = common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & token_controller = contract_state->tep_token_controller(chain_uuid);
        if (msg_sender != token_controller) {
             err.fail_status = precompile_error::fatal;
             err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

             xwarn("precompiled eth contract: burnFrom called by non-admin account %s", context.caller.c_str());

             return false;
        }

        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: burnFrom with invalid parameter");

            return false;
        }

        auto const & burn_from = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: burnFrom with invalid burn from address");

            return false;
        }

        evm_common::u256 const value = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: burnFrom with invalid value");

            return false;
        }

        if (!burn(burn_from, value)) {
            xwarn("burn eth failed");
            ec = evm_runtime::error::xerrc_t::precompiled_contract_erc20_burn;
        }

        if (!ec) {
            auto const & contract_address = context.address;

            xh256s_t topics;
            topics.push_back(xh256_t(event_hex_string_transfer));
            topics.push_back(xh256_t(burn_from.to_h256()));
            topics.push_back(xh256_t(common::xeth_address_t::zero().to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(value));
            result[31] = 1;

            output.cost = 0;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = burn_gas_cost;
            err.output = result;

            xwarn("precompiled eth contract: burnFrom reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_transfer_ownership: {
        xdbg("precompiled eth contract: transferOwnership");

        uint64_t constexpr transfer_ownership_gas_cost = 3155;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_ownership_gas_cost;
            err.output = result;

            xwarn("precompiled eth contract: transferOwnership is not allowed in static context");

            return false;
        }

        assert(chain_uuid == common::xchain_uuid_t::eth);

        auto contract_state = state_ctx->load_unit_state(evm_eth_contract_address);
        auto const & msg_sender = common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & token_owner = contract_state->tep_token_owner(chain_uuid);
        if (msg_sender != token_owner) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: transferOwnership called by non-admin account %s", context.caller.c_str());

            return false;
        }
        ec.clear();

        if (abi_decoder.size() != 1) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: transferOwnership with invalid parameter");

            return false;
        }

        auto const new_owner = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: transferOwnership with invalid burn from address");

            return false;
        }

        contract_state->tep_token_owner(chain_uuid, common::xaccount_address_t::build_from(new_owner, base::enum_vaccount_addr_type_secp256k1_evm_user_account), ec);
        if (!ec) {
            auto const & contract_address = context.address;

            xh256s_t topics;
            topics.push_back(xh256_t(event_hex_string_ownership_transferred));
            topics.push_back(xh256_t(context.caller.to_h256()));
            topics.push_back(xh256_t(new_owner.to_h256()));
            evm_common::xevm_log_t log{contract_address, topics};
            result[31] = 1;

            output.cost = 0;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(std::move(log));
        } else {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_ownership_gas_cost;
            err.output = result;

            xwarn("precompiled eth contract: transferOwnership reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_set_controller: {
        xdbg("precompiled eth contract: setController");

        uint64_t constexpr set_controller_gas_cost = 3155;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = set_controller_gas_cost;
            err.output = result;

            xwarn("precompiled eth contract: setController is not allowed in static context");

            return false;
        }

        assert(chain_uuid == common::xchain_uuid_t::eth);

        // only contract owner can set controller.
        auto contract_state = state_ctx->load_unit_state(evm_eth_contract_address);
        auto const & msg_sender = common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & token_owner = contract_state->tep_token_owner(chain_uuid);
        if (msg_sender != token_owner) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: setController called by non-admin account %s", context.caller.c_str());

            return false;
        }
        ec.clear();

        if (abi_decoder.size() != 1) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: setController with invalid parameter");

            return false;
        }

        auto const new_controller = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled eth contract: setController with invalid burn from address");

            return false;
        }

        auto const & old_controller = common::xeth_address_t::build_from(contract_state->tep_token_controller(chain_uuid));
        contract_state->tep_token_controller(chain_uuid, common::xaccount_address_t::build_from(new_controller, base::enum_vaccount_addr_type_secp256k1_evm_user_account), ec);

        if (!ec) {
            auto const & contract_address = context.address;

            xh256s_t topics;
            topics.push_back(xh256_t(event_hex_string_controller_set));
            topics.push_back(xh256_t(old_controller.to_h256()));
            topics.push_back(xh256_t(new_controller.to_h256()));
            evm_common::xevm_log_t log{contract_address, topics};
            result[31] = 1;

            output.cost = 0;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(std::move(log));
        } else {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = set_controller_gas_cost;
            err.output = result;

            xwarn("precompiled eth contract: setController reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_owner: {
        xdbg("precompiled eth contract: owner");

        xbytes_t result(32, 0);

        auto contract_state = state_ctx->load_unit_state(evm_eth_contract_address);
        auto const & token_owner = contract_state->tep_token_owner(chain_uuid);
        auto const owner = common::xeth_address_t::build_from(token_owner);

        output.cost = 0;
        output.exit_status = Returned;
        output.output = owner.to_h256();

        return true;
    }

    case method_id_controller: {
        xdbg("precompiled eth contract: controller");

        xbytes_t result(32, 0);

        auto contract_state = state_ctx->load_unit_state(evm_eth_contract_address);
        auto const & controller = common::xeth_address_t::build_from(contract_state->tep_token_controller(chain_uuid));

        output.cost = 0;
        output.exit_status = Returned;
        output.output = controller.to_h256();

        return true;
    }

    default: {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

        xwarn("precompiled eth contract: not supported method_id: %" PRIx32, function_selector.method_id);

        return false;
    }
    }
}

bool xtop_delegate_eth_contract::mint(common::xeth_address_t const & mint_to, evm_common::u256 const & value) {
    auto * engine_ptr = ::evm_engine();
    auto * executor_ptr = ::evm_executor();
    assert(engine_ptr != nullptr && executor_ptr != nullptr);
    return unsafe_mint(engine_ptr, executor_ptr, mint_to.data(), mint_to.size(), value.str().c_str());
}

bool xtop_delegate_eth_contract::burn(common::xeth_address_t const & burn_from, evm_common::u256 const & value) {
    auto * engine_ptr = ::evm_engine();
    auto * executor_ptr = ::evm_executor();
    assert(engine_ptr != nullptr && executor_ptr != nullptr);
    return unsafe_burn(engine_ptr, executor_ptr, burn_from.data(), burn_from.size(), value.str().c_str());
}

NS_END4
