// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_vm/xaccount_vm.h"

#include "xbase/xlog.h"
#include "xbasic/xerror/xerror.h"
#include "xcontract_common/xaction_execution_param.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xerror/xerror.h"
#include "xcontract_runtime/xaction_session.h"
#include "xcontract_runtime/xtop_action_generator.h"
#include "xcontract_vm/xerror/xerror.h"
#include "xdata/xconsensus_action.h"

#include <memory>

NS_BEG2(top, contract_vm)

xtop_account_vm::xtop_account_vm(observer_ptr<contract_runtime::system::xsystem_contract_manager_t> const & system_contract_manager)
  : sys_action_runtime_{top::make_unique<contract_runtime::system::xsystem_action_runtime_t>(system_contract_manager)} {
}

xaccount_vm_output_t xtop_account_vm::execute(std::vector<data::xcons_transaction_ptr_t> const & txs,
                                              std::map<common::xaccount_address_t, observer_ptr<base::xvbstate_t>> state_pack,
                                              data::xblock_consensus_para_t const & cs_para) {
    assert(txs.size() > 0);
    assert(state_pack.size() > 0);
    const size_t result_size = txs.size();
    const std::vector<data::xcons_transaction_ptr_t> txs_for_actions(txs);
    const contract_common::xcontract_execution_param_t param(cs_para);
    
    xaccount_vm_execution_result_t result;
    result.transaction_results.reserve(result_size);

    state_accessor::xstate_access_control_data_t ac_data;  // final get from config or program initialization start
    state_accessor::xstate_accessor_t sa{state_pack, ac_data};

    auto actions = contract_runtime::xaction_generator_t::generate(txs_for_actions);
    xdbg("wens_test, xtop_account_vm::execute, action size : %zu\n", actions.size());

    size_t i = 0;
    try {
        for (i = 0; i < result_size; i++) {
            auto action_result = execute_action(std::move(actions[i]), sa, param);
            xdbg("wens_test, xtop_account_vm::execute, receipt data, size : %zu\n", action_result.output.receipt_data.size());
            if (action_result.status.ec) {
                result.transaction_results.emplace_back(action_result);
                assert(result.transaction_results.size() == (i + 1));
                abort(i + 1, result_size, result);
                result.status.ec = action_result.status.ec;
                i = result_size;
                break;
            } else {
                std::error_code ec;
                result.binlog_pack = sa.binlog_pack(ec);
                top::error::throw_error(ec);
                result.bincode_pack = sa.fullstate_bin_pack(ec);
                top::error::throw_error(ec);
                result.transaction_results.emplace_back(action_result);
                assert(result.transaction_results.size() == (i + 1));
            }
        }
    } catch (top::error::xtop_error_t & eh) {
        xerror("account_vm: caught chain error exception: category: %s msg: %s", eh.code().category().name(), eh.what());
    } catch (std::exception const & eh) {
        xerror("account_vm: caught exception: %s", eh.what());
    }
    // exception
    if (i < result_size) {
        abort(i, result_size, result);
        result.status.ec = error::xerrc_t::transaction_execution_abort;
    }

    return pack(txs, result, sa);
}

xaccount_vm_output_t xtop_account_vm::execute(std::vector<data::xcons_transaction_ptr_t> const & txs,
                                              xobject_ptr_t<base::xvbstate_t> block_state,
                                              data::xblock_consensus_para_t const & cs_para) {
    xaccount_vm_execution_result_t result;
    const size_t result_size = txs.size();
    result.transaction_results.reserve(result_size);

    const std::vector<data::xcons_transaction_ptr_t> txs_for_actions(txs);

    state_accessor::xstate_access_control_data_t ac_data;  // final get from config or program initialization start
    contract_common::xcontract_execution_param_t param(cs_para);
    state_accessor::xstate_accessor_t sa{make_observer(block_state.get()), ac_data};

    auto actions = contract_runtime::xaction_generator_t::generate(txs_for_actions);
    xdbg("wens_test, xtop_account_vm::execute, action size : %zu", actions.size());
    assert(actions.size() == result_size);

    size_t i = 0;
    try {
        for (i = 0; i < result_size; i++) {
            auto action_result = execute_action(std::move(actions[i]), sa, param);
            xdbg("wens_test, xtop_account_vm::execute, receipt data, size : %zu", action_result.output.receipt_data.size());
            if (action_result.status.ec) {
                result.transaction_results.emplace_back(action_result);
                abort(i + 1, result_size, result);
                result.status.ec = action_result.status.ec;
                i = result_size;
                break;
            } else {
                std::error_code ec;
                result.binlog = sa.binlog(ec);
                top::error::throw_error(ec);
                result.bincode = sa.fullstate_bin(ec);
                top::error::throw_error(ec);
                result.transaction_results.emplace_back(action_result);
            }
        }
    } catch (top::error::xtop_error_t & eh) {
        xerror("account_vm: caught chain error exception: category: %s msg: %s", eh.code().category().name(), eh.what());
    } catch (std::exception const & eh) {
        xerror("account_vm: caught exception: %s", eh.what());
    }
    // exception
    if (i < result_size) {
        abort(i, result_size, result);
        result.status.ec = error::xerrc_t::transaction_execution_abort;
    }

    return pack(txs, result, sa);
}

contract_runtime::xtransaction_execution_result_t xtop_account_vm::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action,
                                                                                  state_accessor::xstate_accessor_t & sa,
                                                                                  contract_common::xcontract_execution_param_t const & param) {
    contract_runtime::xtransaction_execution_result_t result;

    switch (action->type()) {
    case data::xtop_action_type_t::system: {
        auto const * cons_action = static_cast<data::xsystem_consensus_action_t const *>(action.get());
        contract_common::xcontract_state_t contract_state{cons_action->contract_address(), make_observer(std::addressof(sa)), param};
        result = sys_action_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_action(std::move(action));
        break;
    }

    case data::xtop_action_type_t::user: {
        // auto const & action = dynamic_cast<data::xuser_consensus_action_t const &>(actions[i]);
        // contract_common::xcontract_state_t contract_state{ action.contract_address(), make_observer(std::addressof(ac)) };
        // result.transaction_results[i] = user_action_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_action(action, param);
        break;
    }

    case data::xtop_action_type_t::event: {
        // TODO: we don't have event action either. this is just a placeholder.
        break;
    }

    default: {
        xerror("[xtop_account_vm::execute_action] error action type: %d", action->type());
        assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        result.status.ec = error::xerrc_t::invalid_contract_type;
        break;
    }
    }

    for (size_t i = 0; i < result.output.followup_transaction_data.size(); i++) {
        auto & followup_tx = result.output.followup_transaction_data[i];
        if (followup_tx.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::immediately) {
            if (followup_tx.execute_type == contract_common::xenum_followup_transaction_execute_type::unexecuted) {
                auto followup_action = contract_runtime::xaction_generator_t::generate(followup_tx.followed_transaction);
                auto followup_result = execute_action(std::move(followup_action), sa, param);
                if (followup_result.status.ec) {
                    result.status.ec = followup_result.status.ec;
                    // clear up all follow up txs
                    result.output.followup_transaction_data.clear();
                    break;
                }
                // move follow up state upto tx result
                result.output.binlog = followup_result.output.binlog;
                result.output.contract_state_snapshot = followup_result.output.contract_state_snapshot;
                followup_tx.execute_type = contract_common::xfollowup_transaction_execute_type_t::success;
                for (auto & double_follow_up : followup_result.output.followup_transaction_data) {
                    result.output.followup_transaction_data.emplace_back(std::move(double_follow_up));
                    // not support double follow up now
                    assert(false);
                }
            } else if (followup_tx.execute_type == contract_common::xfollowup_transaction_execute_type_t::success) {
            } else {
                xerror("[xtop_account_vm::execute_action] error follow up tx execute type: %d", followup_tx.execute_type);
                assert(false);
            }
        } else if (followup_tx.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::delay) {
            assert(followup_tx.execute_type == contract_common::xfollowup_transaction_execute_type_t::unexecuted);
        } else {
            xerror("[xtop_account_vm::execute_action] error follow up tx schedule type: %d", followup_tx.schedule_type);
            assert(false);
        }
    }

    return result;
}

void xtop_account_vm::abort(const size_t start_index, const size_t size, xaccount_vm_execution_result_t & result) {
    contract_runtime::xtransaction_execution_result_t abort_result;
    abort_result.status.ec = error::xerrc_t::transaction_execution_abort;
    for (auto i = start_index + 1; i < size; i++) {
        result.transaction_results.emplace_back(abort_result);
        assert(result.transaction_results.size() == (i + 1));
    }
}

xaccount_vm_output_t xtop_account_vm::pack(std::vector<data::xcons_transaction_ptr_t> const & txs,
                                           xaccount_vm_execution_result_t const & result,
                                           state_accessor::xstate_accessor_t & sa) {
    xaccount_vm_output_t output;
    std::vector<data::xcons_transaction_ptr_t> output_txs(txs);

    using namespace state_accessor::properties;
    std::error_code ec;
    auto last_nonce_bytes = sa.get_property_cell_value<xproperty_type_t::map>(
        xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, xproperty_category_t::system}, data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, ec);
    top::error::throw_error(ec);
    auto last_nonce = last_nonce_bytes.empty() ? 0 : top::from_bytes<uint64_t>(last_nonce_bytes);

    auto last_hash_bytes = sa.get_property_cell_value<xproperty_type_t::map>(
        xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, xproperty_category_t::system}, data::XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, ec);
    top::error::throw_error(ec);
    auto last_hash = last_hash_bytes.empty() ? uint256_t{} : top::from_bytes<uint256_t>(last_hash_bytes);

    for (size_t i = 0; i < result.transaction_results.size(); i++) {
        auto const & r = result.transaction_results[i];
        auto & tx = output_txs[i];
        if (r.status.ec) {
            xdbg("wens_test, fail, %s, %s, %d\n", tx->get_source_addr().c_str(), tx->get_target_addr().c_str(), tx->get_tx_subtype());
            xdbg("wens_test, fail, eccode: %d, msg: %s", r.status.ec.value(), r.status.ec.message().c_str());
            tx->set_current_exec_status(data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_fail);
            if (tx->is_send_tx() || tx->is_self_tx()) {
                output.failed_tx_assemble.emplace_back(tx);
            } else if (tx->is_recv_tx()) {
                output.success_tx_assemble.emplace_back(tx);
            } else {
                xwarn("[xlightunit_builder_t::build_block] invalid tx type: %d", tx->get_tx_type());
                assert(false);
            }
        } else {
            xdbg("wens_test, %s, %s, %d\n", tx->get_source_addr().c_str(), tx->get_target_addr().c_str(), tx->get_tx_subtype());
            xdbg("wens_test, eccode: %d, msg: %s", ec.value(), ec.message().c_str());
            if (!r.output.receipt_data.empty()) {
                auto const& receipt_data = r.output.receipt_data.at(contract_common::RECEITP_DATA_ASSET_OUT);
                xdbg("wens_test,set receipt data, %s, %s, %d\n", tx->get_source_addr().c_str(), tx->get_target_addr().c_str(), tx->get_tx_subtype());
                xdbg("wens_test,set receipt data, %s\n", std::string{receipt_data.begin(), receipt_data.end()}.c_str());
                tx->set_receipt_data(r.output.receipt_data);
            }
            tx->set_current_exec_status(data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
            output.success_tx_assemble.emplace_back(tx);
            for (auto & follow_up : r.output.followup_transaction_data) {
                auto const & follow_up_tx = follow_up.followed_transaction;
                if (follow_up.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::immediately) {
                    // follow up tx of success tx is success
                    follow_up_tx->set_current_exec_status(data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
                    output.success_tx_assemble.emplace_back(follow_up_tx);
                } else if (follow_up.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::delay) {
                    assert(follow_up.execute_type == contract_common::xfollowup_transaction_execute_type_t::unexecuted);
                    follow_up_tx->get_transaction()->set_last_trans_hash_and_nonce(last_hash, last_nonce);
                    follow_up_tx->get_transaction()->set_digest();
                    follow_up_tx->get_transaction()->set_digest();
                    last_hash = follow_up_tx->get_tx_hash_256();
                    last_nonce = follow_up_tx->get_tx_nonce();
                    output.delay_tx_assemble.emplace_back(follow_up_tx);
                } else {
                    xwarn("[xlightunit_builder_t::build_block] invalid follow up tx type: %d", follow_up.schedule_type);
                    assert(false);
                }
            }
        }
    }

    if (output.success_tx_assemble.empty()) {
        output.status.ec = contract_vm::error::xenum_errc::none_success_tx;
        return output;
    }

    output.binlog = result.binlog;
    output.contract_state_snapshot = result.bincode;
    output.binlog_pack = result.binlog_pack;
    output.bincode_pack = result.bincode_pack;
    // assert(!output.binlog_pack.empty());
    // for (auto const & pair : output.binlog_pack) {
    //     assert(!pair.second.empty());
    // }
    // assert(!output.bincode_pack.empty());
    // for (auto const & pair : output.bincode_pack) {
    //     assert(!pair.second.empty());
    // }

    return output;
}

NS_END2
