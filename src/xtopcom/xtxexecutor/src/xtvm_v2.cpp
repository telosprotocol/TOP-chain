// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxexecutor/xtvm_v2.h"

#include "xevm_common/common_data.h"
#include "xtxexecutor/xcontract/xtransfer_contract.h"
#include "xtxexecutor/xerror/xerror.h"
#include "xtxexecutor/xunit_service_error.h"

#include <string>
#include <vector>

NS_BEG2(top, txexecutor)

enum_execute_result_type xtvm_v2_t::execute(const xvm_input_t & input, xvm_output_t & output) {
    const statectx::xstatectx_face_ptr_t & statectx = input.get_statectx();
    const xcons_transaction_ptr_t & tx = input.get_tx();
    xassert(tx->get_tx_type() == data::xtransaction_type_transfer);
    xassert(tx->get_tx_version() == data::xtransaction_version_3);
    xassert(base::xvaccount_t::get_addrtype_from_account(tx->get_source_addr()) == base::enum_vaccount_addr_type_secp256k1_evm_user_account);
    xassert(base::xvaccount_t::get_addrtype_from_account(tx->get_target_addr()) == base::enum_vaccount_addr_type_secp256k1_evm_user_account);

    data::xunitstate_ptr_t unitstate = statectx->load_unit_state(tx->get_account_addr());
    if (nullptr == unitstate) {
        xwarn("[xtvm_v2_t::execute] fail-load unit state. tx=%s", tx->dump().c_str());
        return enum_exec_error_load_state;
    }
    if (unitstate->is_state_readonly()) {
        xerror("[xtvm_v2_t::execute] readonly unit state. tx=%s", tx->dump().c_str());
        return enum_exec_error_load_state;
    }
    auto ret = execute_impl(input, output);
    if (!ret) {
        xwarn("[xtvm_v2_t::execute] fail-vm execute, error code: %d, error msg: %s. tx=%s", output.m_ec.value(), output.m_ec.message().c_str(), tx->dump().c_str());
        return enum_exec_error_vm_execute;
    }
    xdbg("[xtvm_v2_t::execute] succ vm execute. tx=%s", tx->dump().c_str());

    return enum_exec_success;
}

bool xtvm_v2_t::execute_impl(const xvm_input_t & input, xvm_output_t & output) {
    const statectx::xstatectx_face_ptr_t & statectx = input.get_statectx();
    const xcons_transaction_ptr_t & tx = input.get_tx();

    if (base::xvaccount_t::get_addrtype_from_account(tx->get_source_addr()) == base::enum_vaccount_addr_type_secp256k1_evm_user_account &&
        base::xvaccount_t::get_addrtype_from_account(tx->get_target_addr()) == base::enum_vaccount_addr_type_secp256k1_evm_user_account) {
        output.m_tx_result.used_gas = default_eth_tx_gas;
    }
    auto result = execute_tx(statectx, tx);
    if (result.status.ec) {
        output.m_ec = result.status.ec;
        return false;
    }
    output.m_tgas_balance_change = result.output.tgas_balance_change;
    for (auto followup_tx : result.output.followup_transaction_data) {
        output.m_contract_create_txs.emplace_back(followup_tx.followed_transaction);
    }

    return true;
}

contract_runtime::xtransaction_execution_result_t xtvm_v2_t::execute_tx(const statectx::xstatectx_face_ptr_t & statectx, const xcons_transaction_ptr_t & tx) {
    auto result = execute_one_tx(statectx, tx);
    if (result.status.ec) {
        xwarn("[xtvm_v2_t::execute_tx] input tx executed failed, category: %s, msg: %s", result.status.ec.category().name(), result.status.ec.message().c_str());
        return result;
    }

    if (!statectx->is_state_dirty()) {
        if (tx->is_send_or_self_tx()) {
            xwarn("[xtvm_v2_t::execute_tx] input send tx no property change. tx=%s", tx->dump().c_str());
            result.status.ec = error::xenum_errc::xunit_contract_exec_no_property_change;
            return result;
        }
    }
    xinfo("[xtvm_v2_t::execute_tx] succ. tx=%s, tx_state=%s", tx->dump().c_str(), tx->dump_execute_state().c_str());

    // copy create txs from account context
    std::vector<xcons_transaction_ptr_t> create_txs;
    for (auto const & followup_tx_data : result.output.followup_transaction_data) {
        auto const & followup_tx = followup_tx_data.followed_transaction;
        auto followup_tx_result = execute_one_tx(statectx, tx);
        if (followup_tx_result.status.ec) {  // contract create tx send action may not change property, it's ok
            xwarn("[xtvm_v2_t::execute_tx] contract create tx fail. input_tx:%s, new_tx:%s category: %s, msg: %s",
                  tx->dump().c_str(),
                  followup_tx->dump().c_str(),
                  followup_tx_result.status.ec.category().name(),
                  followup_tx_result.status.ec.message().c_str());
            result.status.ec = followup_tx_result.status.ec;
            return result;
        } else {
            xinfo("[xtvm_v2_t::execute_tx] contract create tx succ. input_tx:%s, new_tx:%s", tx->dump().c_str(), followup_tx->dump(true).c_str());
        }
        // new_tx->set_push_pool_timestamp(now);
    }
    xinfo("[xtvm_v2_t::execute_tx] create_txs succ. tx=%s, tx_state=%s", tx->dump().c_str(), tx->dump_execute_state().c_str());
    return result;
}

contract_runtime::xtransaction_execution_result_t xtvm_v2_t::execute_one_tx(const statectx::xstatectx_face_ptr_t & statectx, const xcons_transaction_ptr_t & tx) {
    std::unique_ptr<contract_common::xbasic_stateless_contract_t> stateless_contract;
    contract_runtime::xtransaction_execution_result_t result;

    // build context
    auto unitstate = statectx->load_unit_state(tx->get_account_addr());
    contract_common::xstateless_contract_execution_context_t ctx{make_observer(unitstate.get())};
    // set context
    ctx.set_action_stage(tx->get_tx_subtype());
    if (tx->get_tx_type() == data::xtransaction_type_transfer) {
        fill_transfer_context(statectx, tx, ctx, result.status.ec);
        if (result.status.ec) {
            xwarn("[xtvm_v2_t::execute_one_tx] fill_transfer_context failed. category: %s, msg: %s", result.status.ec.category().name(), result.status.ec.message().c_str());
            return result;
        }
        stateless_contract = top::make_unique<contract::xtransfer_contract_t>();
    } else {
        xerror("[xtvm_v2_t::execute_one_tx] error tx_v3 type: %d", tx->get_tx_type());
        xassert(false);
        result.status.ec = error::xenum_errc::xtransaction_parse_type_invalid;
        return result;
    }

    if (ctx.action_name().empty()) {
        xdbg("[xtvm_v2_t::execute_one_tx] action do nothing. tx=%s", tx->dump().c_str());
        return result;
    }

    top::observer_ptr<contract_common::xstateless_contract_execution_context_t> ctx_observer{std::addressof(ctx)};

    try {
        result = stateless_contract->execute(ctx_observer);
    } catch (top::error::xtop_error_t const & eh) {
        result.status.ec = eh.code();
    } catch (std::exception const & eh) {
        result.status.ec = error::xenum_errc::enum_xtxexecutor_error_execute_fail;
        result.status.extra_msg = eh.what();
    } catch (enum_xerror_code ec) {
        result.status.ec = ec;
    } catch (...) {
        result.status.ec = error::xenum_errc::enum_xtxexecutor_error_execute_fail;
    }

    return result;
}

void xtvm_v2_t::fill_transfer_context(const statectx::xstatectx_face_ptr_t & statectx,
                                      const xcons_transaction_ptr_t & tx,
                                      contract_common::xstateless_contract_execution_context_t & ctx,
                                      std::error_code & ec) {
    if (tx->get_inner_table_flag() || tx->is_self_tx()) {
        // one stage
        if (ctx.action_stage() == data::xconsensus_action_stage_t::send || ctx.action_stage() == data::xconsensus_action_stage_t::self) {
            auto recver_unitstate = statectx->load_unit_state(tx->get_transaction()->get_target_addr());
            ctx.set_unitstate_other(make_observer(recver_unitstate.get()));
            ctx.set_action_name("transfer");
        } else {
            xerror("[xtvm_v2_t::fill_transfer_context] error tx_v3 stage: %d", ctx.action_stage());
            xassert(false);
            ec = error::xenum_errc::xtransaction_parse_type_invalid;
            return;
        }
    } else {
        // three stage
        if (ctx.action_stage() == data::xconsensus_action_stage_t::send) {
            ctx.set_action_name("withdraw");
        } else if (ctx.action_stage() == data::xconsensus_action_stage_t::recv) {
            ctx.set_action_name("deposit");
        } else if (ctx.action_stage() == data::xconsensus_action_stage_t::confirm) {
            // do nothing
            return;
        } else {
            xerror("[xtvm_v2_t::fill_transfer_context] error tx stage: %d", ctx.action_stage());
            xassert(false);
            ec = error::xenum_errc::xtransaction_parse_type_invalid;
            return;
        }
    }
    base::xstream_t param_stream(base::xcontext_t::instance());
    param_stream << std::string{data::XPROPERTY_ASSET_ETH};
    param_stream << evm_common::toBigEndianString(tx->get_transaction()->get_amount_256());
    std::string data{reinterpret_cast<char *>(param_stream.data()), static_cast<std::size_t>(param_stream.size())};
    ctx.set_action_data(xbytes_t{data.data(), data.data() + data.size()});

    return;
}

NS_END2
