// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_erc20_contract.h"

#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xevm_common/common_data.h"

#include <cinttypes>

NS_BEG4(top, contract_runtime, evm, sys_contract)

bool xtop_evm_erc20_sys_contract::execute(xbytes_t input,
                                          uint64_t target_gas,
                                          sys_contract_context const & context,
                                          bool is_static,
                                          observer_ptr<statectx::xstatectx_face_t> state_ctx,
                                          sys_contract_precompile_output & output,
                                          sys_contract_precompile_error & err) {
    // ERC20 method ids:
    //--------------------------------------------------
    // decimals()                            => 313ce567
    // totalSupply()                         => 18160ddd
    // balanceOf(address)                    => 70a08231
    // transfer(address,uint256)             => a9059cbb
    // transferFrom(address,address,uint256) => 23b872dd
    // approve(address,uint256)              => 095ea7b3
    // allowance(address,address)            => dd62ed3e
    // approveTOP(bytes32,uint64)            => 24655e23
    //--------------------------------------------------
#if defined(__LITTLE_ENDIAN__)
    constexpr uint32_t method_id_decimals{0x67e53c31};
    constexpr uint32_t method_id_total_supply{0xdd0d1618};
    constexpr uint32_t method_id_balance_of{0x3182a070};
    constexpr uint32_t method_id_transfer{0xbb9c05a9};
    constexpr uint32_t method_id_transfer_from{0xdd72b823};
    constexpr uint32_t method_id_approve{0xb3a75e09};
    constexpr uint32_t method_id_allowance{0x3eed62dd};
#elif defined(__BIG_ENDIAN__)
    constexpr uint32_t method_id_decimals{0x313ce567};
    constexpr uint32_t method_id_total_supply{0x18160ddd};
    constexpr uint32_t method_id_balance_of{0x70a08231};
    constexpr uint32_t method_id_transfer{0xa9059cbb};
    constexpr uint32_t method_id_transfer_from{0x23b872dd};
    constexpr uint32_t method_id_approve{0x095ea7b3};
    constexpr uint32_t method_id_allowance{0xdd62ed3e};
#else
#    error "I don't know what architecture this is!"
#endif
    assert(state_ctx);

    // erc20_uuid (1 byte) | erc20_method_id (4 bytes) | parameters (depends)
    if (input.size() < 5) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = precompile_error_ExitFatal::Other;

        xwarn("predefined erc20 contract: invalid input");

        return false;
    }

    common::xtoken_id_t const erc20_token_id{top::from_byte<common::xtoken_id_t>(input.front())};

    xbytes_t const data{std::next(std::begin(input), 1), std::end(input)};
    xbytes_t const method_id_bytes{std::begin(data), std::next(std::begin(data), 4)};
    xbytes_t const parameters{std::next(std::begin(data), 4), std::end(data)};

    uint32_t method_id;
    std::memcpy(&method_id, method_id_bytes.data(), 4);

    switch (method_id) {
    case method_id_decimals: {
        uint64_t const decimals_gas_cost = 2535;

        if (!parameters.empty()) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: decimals with non-empty parameter");

            return false;
        }

        if (target_gas < decimals_gas_cost) {
            err.fail_status = Error;
            err.minor_status = precompile_error_ExitError::OutOfGas;

            xwarn("predefined erc20 contract: decimals out of gas, gas_limit %" PRIu64 " gas required %" PRIu64, target_gas, decimals_gas_cost);

            return false;
        }

        xbytes_t decimals;
        decimals.resize(32);
        decimals[31] = static_cast<uint8_t>(18);

        output.exit_status = Returned;
        output.cost = decimals_gas_cost;
        output.output = decimals;

        return true;
    }

    case method_id_total_supply: {
        if (!parameters.empty()) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: total supply with non-empty parameter");

            return false;
        }

        evm_common::u256 supply{0};
        output.exit_status = Returned;
        output.cost = 0;
        output.output = top::to_bytes(supply);

        return true;
    }

    case method_id_balance_of: {
        if (parameters.size() != 32) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: balance_of with invalid parameter (length not 32)");

            return false;
        }

        xbytes_t const prefix{std::begin(parameters), std::next(std::begin(parameters), 12)};
        if (std::any_of(std::begin(prefix), std::end(prefix), [](xbyte_t byte) { return byte != 0; })) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: balance_of invalid account");

            return false;
        }

        uint64_t const balance_of_gas_cost = 3268;
        if (target_gas < balance_of_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = precompile_error_ExitError::OutOfGas;

            xwarn("predefined erc20 contract: balance_of out of gas, gas_limit %" PRIu64 " gas required %" PRIu64, target_gas, balance_of_gas_cost);

            return false;
        }

        xbytes_t const account_address_bytes{std::next(std::begin(parameters), 12), std::next(std::begin(parameters), 32)};
        assert(account_address_bytes.size() == 20);

        common::xeth_address_t const eth_address = common::xeth_address_t::build_from(account_address_bytes);
        auto state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(eth_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());

        evm_common::u256 value{0};
        switch (erc20_token_id) {
        case common::xtoken_id_t::top: {
            value = state->balance();
            break;
        }

        case common::xtoken_id_t::usdt: {
            value = state->tep_token_balance("USDT");
            break;
        }

        case common::xtoken_id_t::usdc: {
            value = state->tep_token_balance("USDC");
            break;
        }

        default:
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: balance_of invalid token id %d", static_cast<int>(erc20_token_id));

            return false;
        }

        output.cost = balance_of_gas_cost;
        output.exit_status = Returned;
        output.output = top::to_bytes(value);

        return true;
    }

    case method_id_transfer: {
        if (parameters.size() != 32 * 2) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: transfer with invalid parameter");

            return false;
        }

        xbytes_t const prefix{std::begin(parameters), std::next(std::begin(parameters), 12)};
        if (std::any_of(std::begin(prefix), std::end(prefix), [](xbyte_t byte) { return byte != 0; })) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: transfer with invalid account");

            return false;
        }

        uint64_t const transfer_gas_cost = 18446;
        if (target_gas < transfer_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = precompile_error_ExitError::OutOfGas;

            xwarn("predefined erc20 contract: transfer out of gas, gas_limit %" PRIu64 " gas required %" PRIu64, target_gas, transfer_gas_cost);

            return false;
        }

        xbytes_t const to_account_address_bytes{std::next(std::begin(parameters), 12), std::next(std::begin(parameters), 32)};
        assert(to_account_address_bytes.size() == 20);
        xbytes_t const value_bytes{std::next(std::begin(parameters), 32), std::next(std::begin(parameters), 32 + 32)};
        assert(value_bytes.size() == 32);

        common::xeth_address_t recipient_address = common::xeth_address_t::build_from(to_account_address_bytes);
        common::xaccount_address_t recipient_account_address = common::xaccount_address_t::build_from(recipient_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 const value = top::from_bytes<evm_common::u256>(value_bytes);

        auto sender_state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());
        auto recver_state = state_ctx->load_unit_state(recipient_account_address.vaccount());

        xbytes_t result(32, 0);
        std::error_code ec;
        sender_state->transfer(erc20_token_id, top::make_observer(recver_state.get()), value, ec);

        if (!ec) {
            auto const & contract_address = context.address;
            assert(!ec);
            auto const & caller_address = context.caller;
            assert(!ec);

            evm_common::xevm_log_t log;
            log.address = top::to_string(contract_address.to_h160());
            assert(log.address.size() == 20);
            log.data = top::to_string(value);
            assert(log.data.size() == 32);
            log.topics.push_back(top::to_string(evm_common::fromHex("0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef", evm_common::WhenError::Throw)));
            assert(log.topics.back().size() == 32);
            log.topics.push_back(top::to_string(caller_address.to_h256()));
            assert(log.topics.back().size() == 32);
            log.topics.push_back(top::to_string(recipient_address.to_h256()));
            assert(log.topics.back().size() == 32);

            result[31] = 1;

            output.cost = transfer_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.cost = transfer_gas_cost / 2;
            err.fail_status = Revert;
            err.minor_status = precompile_error_ExitRevert::Reverted;
            err.output = result;
        }

        return !ec;
    }

    case method_id_transfer_from: {
        if (parameters.size() != 32 * 3) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: transfer_from with invalid parameters");

            return false;
        }

        xbytes_t prefix{std::begin(parameters), std::next(std::begin(parameters), 12)};
        if (std::any_of(std::begin(prefix), std::end(prefix), [](xbyte_t byte) { return byte != 0; })) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: transfer_from invalid owner account");

            return false;
        }

        prefix = xbytes_t{std::next(std::begin(parameters), 32), std::next(std::begin(parameters), 32 + 12)};
        if (std::any_of(std::begin(prefix), std::end(prefix), [](xbyte_t byte) { return byte != 0; })) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: transfer_from invalid recipient account");

            return false;
        }

        uint64_t const transfer_from_gas_cost = 37839;
        if (target_gas < transfer_from_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = precompile_error_ExitError::OutOfGas;

            xwarn("predefined erc20 contract: transfer_from out of gas, gas_limit %" PRIu64 " gas required %" PRIu64, target_gas, transfer_from_gas_cost);

            return false;
        }

        xbytes_t const owner_account_address_bytes{std::next(std::begin(parameters), 12), std::next(std::begin(parameters), 32)};
        assert(owner_account_address_bytes.size() == 20);
        xbytes_t const to_account_address_bytes{std::next(std::begin(parameters), 32 + 12), std::next(std::begin(parameters), 32 + 32)};
        assert(to_account_address_bytes.size() == 20);
        xbytes_t const value_bytes{std::next(std::begin(parameters), 32 * 2), std::next(std::begin(parameters), 32 * 3)};
        assert(value_bytes.size() == 32);

        common::xeth_address_t owner_address = common::xeth_address_t::build_from(owner_account_address_bytes);
        common::xeth_address_t recipient_address = common::xeth_address_t::build_from(to_account_address_bytes);

        common::xaccount_address_t owner_account_address = common::xaccount_address_t::build_from(owner_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        common::xaccount_address_t recipient_account_address = common::xaccount_address_t::build_from(recipient_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 value = top::from_bytes<evm_common::u256>(value_bytes);

        xbytes_t result(32, 0);
        std::error_code ec;

        auto owner_state = state_ctx->load_unit_state(owner_account_address.vaccount());
        owner_state->update_allowance(erc20_token_id,
                                      common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account),
                                      value,
                                      data::xallowance_update_op_t::decrease,
                                      ec);
        if (!ec) {
            auto recver_state = state_ctx->load_unit_state(recipient_account_address.vaccount());
            owner_state->transfer(erc20_token_id, top::make_observer(recver_state.get()), value, ec);
            if (!ec) {
                result[31] = 1;
            }
        }

        if (!ec) {
            auto const & contract_address = context.address;

            evm_common::xevm_log_t log;
            log.address = top::to_string(contract_address.to_h160());
            assert(log.address.size() == 20);
            log.data = top::to_string(value);
            assert(log.data.size() == 32);
            log.topics.push_back(top::to_string(evm_common::fromHex("0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef", evm_common::WhenError::Throw)));
            assert(log.topics.back().size() == 32);
            log.topics.push_back(top::to_string(owner_address.to_h256()));
            assert(log.topics.back().size() == 32);
            log.topics.push_back(top::to_string(recipient_address.to_h256()));
            assert(log.topics.back().size() == 32);

            result[31] = 1;

            output.cost = transfer_from_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.cost = transfer_from_gas_cost / 2;
            err.fail_status = Revert;
            err.output = result;
        }

        return !ec;
    }

    case method_id_approve: {
        if (parameters.size() != 32 * 2) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: approve with invalid parameter");

            return false;
        }

        xbytes_t const prefix{std::begin(parameters), std::next(std::begin(parameters), 12)};
        if (std::any_of(std::begin(prefix), std::end(prefix), [](xbyte_t byte) { return byte != 0; })) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: approve invalid spender account");

            return false;
        }

        uint64_t const approve_gas_cost = 18599;
        if (target_gas < approve_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = precompile_error_ExitError::OutOfGas;

            xwarn("predefined erc20 contract: approve out of gas, gas_limit %" PRIu64 " gas required %" PRIu64, target_gas, approve_gas_cost);

            return false;
        }

        xbytes_t const spender_account_bytes{std::next(std::begin(parameters), 12), std::next(std::begin(parameters), 32)};
        assert(spender_account_bytes.size() == 20);
        xbytes_t const amount_bytes{std::next(std::begin(parameters), 32), std::next(std::begin(parameters), 32 + 32)};
        assert(amount_bytes.size() == 32);

        common::xeth_address_t spender_address = common::xeth_address_t::build_from(spender_account_bytes);
        common::xaccount_address_t spender_account_address = common::xaccount_address_t::build_from(spender_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        evm_common::u256 amount = top::from_bytes<evm_common::u256>(amount_bytes);

        xbytes_t result(32, 0);
        std::error_code ec;

        auto sender_state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());
        sender_state->approve(erc20_token_id, spender_account_address, amount, ec);

        if (!ec) {
            auto const & contract_address = context.address;
            auto const & caller_address = context.caller;

            evm_common::xevm_log_t log;
            log.address = top::to_string(contract_address.to_h160());
            assert(log.address.size() == 20);
            log.data = top::to_string(amount);
            assert(log.data.size() == 32);
            log.topics.push_back(top::to_string(evm_common::fromHex("0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925", evm_common::WhenError::Throw)));
            assert(log.topics.back().size() == 32);
            log.topics.push_back(top::to_string(caller_address.to_h256()));
            assert(log.topics.back().size() == 32);
            log.topics.push_back(top::to_string(spender_address.to_h256()));
            assert(log.topics.back().size() == 32);

            result[31] = 1;

            output.cost = approve_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.cost = approve_gas_cost / 2;
            err.fail_status = Revert;
            err.output = result;

            xwarn("predefined erc20 contract: approve failed. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_allowance: {
        if (parameters.size() != 32 * 2) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: allowance with invalid parameter");

            return false;
        }

        xbytes_t prefix{std::begin(parameters), std::next(std::begin(parameters), 12)};
        if (std::any_of(std::begin(prefix), std::end(prefix), [](xbyte_t byte) { return byte != 0; })) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: allowance invalid owner account");

            return false;
        }

        prefix = xbytes_t{std::next(std::begin(parameters), 32), std::next(std::begin(parameters), 32 + 12)};
        if (std::any_of(std::begin(prefix), std::end(prefix), [](xbyte_t byte) { return byte != 0; })) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = precompile_error_ExitFatal::Other;

            xwarn("predefined erc20 contract: allowance invalid spender account");

            return false;
        }

        xbytes_t const owner_account_bytes{std::next(std::begin(parameters), 12), std::next(std::begin(parameters), 32)};
        assert(owner_account_bytes.size() == 20);
        xbytes_t const spender_account_bytes{std::next(std::begin(parameters), 32 + 12), std::next(std::begin(parameters), 32 + 32)};
        assert(spender_account_bytes.size() == 20);

        common::xeth_address_t owner_address = common::xeth_address_t::build_from(owner_account_bytes);
        common::xeth_address_t spender_address = common::xeth_address_t::build_from(spender_account_bytes);

        common::xaccount_address_t owner_account_address = common::xaccount_address_t::build_from(owner_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        common::xaccount_address_t spender_account_address = common::xaccount_address_t::build_from(spender_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        std::error_code ec;
        auto owner_state = state_ctx->load_unit_state(owner_account_address.vaccount());
        auto amount = owner_state->allowance(erc20_token_id, spender_account_address, ec);

        output.cost = 1;
        output.output = top::to_bytes(amount);
        output.exit_status = Returned;

        return true;
    }

    default: {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = precompile_error_ExitFatal::NotSupported;

        return false;
    }
    }
}

NS_END4
