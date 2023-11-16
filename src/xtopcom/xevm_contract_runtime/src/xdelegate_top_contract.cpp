// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xdelegate_top_contract.h"

#include "xbasic/xfixed_hash.h"
#include "xcommon/common_data.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_contract_runtime/sys_contract/xdelegate_erc20_contract.h"
#include "xevm_contract_runtime/xerror/xerror.h"

#include <cinttypes>

NS_BEG4(top, contract_runtime, evm, sys_contract)

bool xtop_delegate_top_contract::execute(xbytes_t input,
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

        xwarn("precompiled top contract: invalid input");

        return false;
    }

    common::xchain_uuid_t const chain_uuid{top::from_byte<common::xchain_uuid_t>(input.front())};
    if (chain_uuid != common::xchain_uuid_t::eth) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

        xwarn("precompiled top contract: not supported token: %d", static_cast<int>(chain_uuid));

        return false;
    }

    std::error_code ec;
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::next(std::begin(input), 1), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

        xwarn("precompiled top contract: illegal input data");

        return false;
    }

    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

        xwarn("precompiled top contract: illegal input function selector");

        return false;
    }

    switch (function_selector.method_id) {
    case method_id_decimals: {
        xdbg("precompiled top contract: decimals");

        output.exit_status = Returned;
        output.cost = 0;
        output.output = top::to_bytes(evm_common::u256{6});

        return true;
    }

    case method_id_total_supply: {
        xdbg("precompiled top contract: totalSupply");

        if (!abi_decoder.empty()) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: totalSupply with non-empty parameter");

            return false;
        }

        output.exit_status = Returned;
        output.cost = 0;
        output.output = top::to_bytes(evm_common::u256{20000000000000000});

        return true;
    }

    case method_id_balance_of: {
        xdbg("precompiled top contract: balanceOf");

        if (abi_decoder.size() != 1) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: balance_of with invalid parameter (parameter count not one)");

            return false;
        }

        common::xeth_address_t const eth_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: balance_of invalid account");

            return false;
        }

        auto state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(eth_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account));

        evm_common::u256 value = state->balance();
        output.cost = 0;
        output.exit_status = Returned;
        output.output = top::to_bytes(value);
        assert(output.output.size() == 32);

        return true;
    }

    case method_id_transfer: {
        xdbg("precompiled top contract: transfer");

        uint64_t constexpr transfer_gas_cost = 18446;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_gas_cost;
            err.output = result;

            xwarn("precompiled top contract: transfer is not allowed in static context");

            return false;
        }

        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: transfer with invalid parameter");

            return false;
        }

        auto const & recipient_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: transfer with invalid account");

            return false;
        }
        common::xaccount_address_t recipient_account_address = common::xaccount_address_t::build_from(recipient_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 const value = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: transfer with invalid value");

            return false;
        }

        auto sender_state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account));
        auto recver_state = state_ctx->load_unit_state(recipient_account_address);

        sender_state->transfer(common::xtoken_id_t::top, top::make_observer(recver_state.get()), value, ec);

        if (!ec) {
            auto const & contract_address = context.address;
            auto const & caller_address = context.caller;

            xh256s_t topics;
            topics.push_back(xh256_t(event_hex_string_transfer));
            topics.push_back(xh256_t(caller_address.to_h256()));
            topics.push_back(xh256_t(recipient_address.to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(value));

            result[31] = 1;

            output.cost = 0;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            uint64_t constexpr transfer_reverted_gas_cost = 3662;

            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_reverted_gas_cost;
            err.output = result;

            xwarn("precompiled top contract: transfer reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_transfer_from: {
        xdbg("precompiled top contract: transferFrom");

        xbytes_t result(32, 0);

        if (is_static) {
            uint64_t constexpr transfer_from_gas_cost = 18190;
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_from_gas_cost;
            err.output = result;

            xwarn("precompiled top contract: transferFrom is not allowed in static context");

            return false;
        }

        if (abi_decoder.size() != 3) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: transferFrom with invalid parameters");

            return false;
        }

        auto const & owner_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: transferFrom invalid owner account");

            return false;
        }

        auto const recipient_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: transferFrom invalid recipient account");

            return false;
        }

        auto const & owner_account_address = common::xaccount_address_t::build_from(owner_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & recipient_account_address = common::xaccount_address_t::build_from(recipient_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        auto const value = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: transferFrom invalid value");

            return false;
        }

        auto owner_state = state_ctx->load_unit_state(owner_account_address);
        owner_state->update_allowance(common::xtoken_id_t::top,
                                      common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account),
                                      value,
                                      data::xallowance_update_op_t::decrease,
                                      ec);
        if (!ec) {
            auto recver_state = state_ctx->load_unit_state(recipient_account_address);
            owner_state->transfer(common::xtoken_id_t::top, top::make_observer(recver_state.get()), value, ec);
            if (!ec) {
                result[31] = 1;
            }
        }

        if (!ec) {
            auto const & contract_address = context.address;

            xh256s_t topics;
            topics.push_back(xh256_t(event_hex_string_transfer));
            topics.push_back(xh256_t(owner_address.to_h256()));
            topics.push_back(xh256_t(recipient_address.to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(value));

            result[31] = 1;

            output.cost = 0;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            uint64_t constexpr transfer_from_reverted_gas_cost = 4326;
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_from_reverted_gas_cost;
            err.output = result;

            xwarn("precompiled top contract: transferFrom reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_approve: {
        xdbg("precompiled top contract: approve");

        uint64_t constexpr approve_gas_cost = 18599;
        xbytes_t result(32, 0);
        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = approve_gas_cost;
            err.output = result;

            xwarn("precompiled top contract: approve is not allowed in static context");

            return false;
        }

        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: approve with invalid parameter");

            return false;
        }

        auto const & spender_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: approve invalid spender account");

            return false;
        }
        common::xaccount_address_t spender_account_address = common::xaccount_address_t::build_from(spender_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        auto const amount = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: approve invalid value");

            return false;
        }

        auto sender_state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account));
        sender_state->approve(common::xtoken_id_t::top, spender_account_address, amount, ec);

        auto const & contract_address = context.address;
        auto const & caller_address = context.caller;

        if (!ec) {
            xh256s_t topics;
            topics.push_back(xh256_t(event_hex_string_approve));
            topics.push_back(xh256_t(caller_address.to_h256()));
            topics.push_back(xh256_t(spender_address.to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(amount));
            result[31] = 1;

            output.cost = 0;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = approve_gas_cost / 2;
            err.output = result;

            xerror("precompiled top contract: approve reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return true;
    }

    case method_id_allowance: {
        xdbg("precompiled top contract: allowance");

        xbytes_t result(32, 0);
        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: allowance with invalid parameter");

            return false;
        }

        auto const & owner_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: allowance invalid owner account");

            return false;
        }

        auto const & spender_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("precompiled top contract: allowance invalid spender account");

            return false;
        }

        auto const & owner_account_address = common::xaccount_address_t::build_from(owner_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & spender_account_address = common::xaccount_address_t::build_from(spender_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        auto owner_state = state_ctx->load_unit_state(owner_account_address);
        result = top::to_bytes(owner_state->allowance(common::xtoken_id_t::top, spender_account_address, ec));
        assert(result.size() == 32);

        output.cost = 0;
        output.output = result;
        output.exit_status = Returned;

        return true;
    }

    default: {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

        xwarn("precompiled top contract: not supported method_id: %" PRIx32, function_selector.method_id);

        return false;
    }
    }
}

NS_END4
