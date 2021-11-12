// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_vm/xaccount_vm.h"

#include "xbase/xlog.h"
#include "xbasic/xerror/xerror.h"
#include "xcontract_common/xcontract_execution_param.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xerror/xerror.h"
#include "xcontract_runtime/xaction_session.h"
#include "xcontract_runtime/xtop_action_generator.h"
#include "xcontract_vm/xerror/xerror.h"
#include "xdata/xconsensus_action.h"

#include <cinttypes>
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
    xdbg("[xtop_account_vm::execute] tx size: %" PRIu64, result_size);
    xdbg("[xtop_account_vm::execute] param, clock: %" PRIu64 ", timestamp: %" PRIu64 ", table_account: %s, table_height: %" PRIu64 ", total_lock_tgas: %" PRIu64,
         param.clock,
         param.timestamp,
         param.table_account.c_str(),
         param.table_commit_height,
         param.total_lock_tgas_token);

    xaccount_vm_execution_result_t result;
    result.transaction_results.reserve(result_size);

    state_accessor::xstate_access_control_data_t ac_data;  // final get from config or program initialization start
    state_accessor::xstate_accessor_t sa{state_pack, ac_data};

    auto actions = contract_runtime::xaction_generator_t::generate(txs_for_actions);

    size_t i = 0;
    try {
        for (i = 0; i < result_size; i++) {
            // calc delay time collection
            {
                auto delay_time = txs[i]->get_transaction()->get_delay_from_fire_timestamp(cs_para.get_gettimeofday_s());
                if (txs[i]->is_self_tx() || txs[i]->is_send_tx()) {
                    XMETRICS_GAUGE(metrics::txdelay_from_client_to_sendtx_exec, delay_time);
                } else if (txs[i]->is_recv_tx()) {
                    XMETRICS_GAUGE(metrics::txdelay_from_client_to_recvtx_exec, delay_time);
                } else if (txs[i]->is_confirm_tx()) {
                    XMETRICS_GAUGE(metrics::txdelay_from_client_to_confirmtx_exec, delay_time);
                }
            }

            auto action_result = execute_action(std::move(actions[i]), param, sa);
            if (action_result.status.ec) {
                xwarn("[xtop_account_vm::execute] tx[%" PRIu64 "] failed, category: %s, msg: %s, abort all txs after!",
                      i,
                      action_result.status.ec.category().name(),
                      action_result.status.ec.message().c_str());
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
        xwarn("[xtop_account_vm::execute] tx[%" PRIu64 "] failed, abort all txs after!", i);
        abort(i, result_size, result);
        result.status.ec = error::xerrc_t::transaction_execution_abort;
    }

    return pack(txs, result, param, sa);
}

contract_runtime::xtransaction_execution_result_t xtop_account_vm::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action,
                                                                                  contract_common::xcontract_execution_param_t const & param,
                                                                                  state_accessor::xstate_accessor_t & sa) {
    contract_runtime::xtransaction_execution_result_t result;

    switch (action->type()) {
    case data::xtop_action_type_t::system: {
        auto const * cons_action = static_cast<data::xsystem_consensus_action_t const *>(action.get());
        contract_common::xcontract_state_t contract_state{cons_action->contract_address(), make_observer(std::addressof(sa)), param};
        result = sys_action_runtime_->new_session(make_observer(std::addressof(contract_state)))->execute_action(std::move(action));

        XMETRICS_GAUGE(metrics::txexecutor_total_system_contract_count, 1);
        if (result.status.ec) {
            XMETRICS_GAUGE(metrics::txexecutor_system_contract_failed_count, 1);
        }
        break;
    }

    case data::xtop_action_type_t::user: {
        // TODO: we don't support user action yet.
        break;
    }

    case data::xtop_action_type_t::event: {
        // TODO: we don't have event action either. this is just a placeholder.
        break;
    }

    default: {
        xerror("[xtop_account_vm::execute_action] error action type: %" PRIi32, action->type());
        assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        result.status.ec = error::xerrc_t::invalid_contract_type;
        break;
    }
    }

    xdbg("[xtop_account_vm::execute] followup_tx size: %" PRIu64, result.output.followup_transaction_data.size());
    for (size_t i = 0; i < result.output.followup_transaction_data.size(); i++) {
        auto & followup_tx = result.output.followup_transaction_data[i];
        xdbg("[xtop_account_vm::execute] followup_tx[%" PRIu64 "] failed, schedule_type: %" PRIu64 ", execute_type: %" PRIu64 "",
             i,
             followup_tx.schedule_type,
             followup_tx.execute_type);
        if (followup_tx.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::immediately) {
            if (followup_tx.execute_type == contract_common::xenum_followup_transaction_execute_type::unexecuted) {
                auto followup_action = contract_runtime::xaction_generator_t::generate(followup_tx.followed_transaction);
                auto followup_result = execute_action(std::move(followup_action), param, sa);
                if (followup_result.status.ec) {
                    xwarn("[xtop_account_vm::execute] followup_tx[%" PRIu64 "] failed, category: %s, msg: %s, break!",
                          i,
                          followup_result.status.ec.category().name(),
                          followup_result.status.ec.message().c_str());
                    result.status.ec = followup_result.status.ec;
                    result.output.followup_transaction_data.clear();
                    break;
                }
                followup_tx.execute_type = contract_common::xfollowup_transaction_execute_type_t::success;
                // TODO: not support double follow up now
                if (followup_result.output.followup_transaction_data.size() > 0) {
                    assert(false);
                }
            } else if (followup_tx.execute_type == contract_common::xfollowup_transaction_execute_type_t::success) {
            } else {
                xerror("[xtop_account_vm::execute_action] error followup_tx execute type: %" PRIi32, followup_tx.execute_type);
                assert(false);
            }
        } else if (followup_tx.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::delay) {
            assert(followup_tx.execute_type == contract_common::xfollowup_transaction_execute_type_t::unexecuted);
        } else {
            xerror("[xtop_account_vm::execute_action] error followup_tx schedule type: %" PRIi32, followup_tx.schedule_type);
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
                                           contract_common::xcontract_execution_param_t const & param,
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

    auto recv_tx_num_bytes = sa.get_property_cell_value<xproperty_type_t::map>(
        xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, xproperty_category_t::system}, data::XPROPERTY_TX_INFO_RECVTX_NUM, ec);
    top::error::throw_error(ec);
    auto recv_tx_num = recv_tx_num_bytes.empty() ? 0 : top::from_bytes<uint64_t>(recv_tx_num_bytes);
    xinfo("[xtop_account_vm::pack] pack last_nonce: %" PRIu64 ", recv_tx_num: %" PRIu64, last_nonce, recv_tx_num);

    uint64_t recv_tx_num_new = recv_tx_num;

    for (size_t i = 0; i < result.transaction_results.size(); i++) {
        auto const & r = result.transaction_results[i];
        auto & tx = output_txs[i];
        if (tx->is_recv_tx()) {
            recv_tx_num_new++;
        }
        for (auto const & pair : r.output.fee_change) {
            auto const & option = pair.first;
            auto const & value = pair.second;
            if (option == contract_common::xcontract_execution_fee_option_t::send_tx_lock_tgas) {
                tx->set_current_send_tx_lock_tgas(value);
            } else if (option == contract_common::xcontract_execution_fee_option_t::used_tgas) {
                tx->set_current_used_tgas(value);
            } else if (option == contract_common::xcontract_execution_fee_option_t::used_deposit) {
                tx->set_current_used_deposit(value);
            } else if (option == contract_common::xcontract_execution_fee_option_t::used_disk) {
                tx->set_current_used_disk(value);
            } else if (option == contract_common::xcontract_execution_fee_option_t::recv_tx_use_send_tx_tgas) {
                tx->set_current_recv_tx_use_send_tx_tgas(value);
            } else {
                assert(false);
            }
        }
        if (r.status.ec) {
            xwarn("[xtop_account_vm::pack] tx[%" PRIu64 "] failed, category: %s, msg: %s", i, r.status.ec.category().name(), r.status.ec.message().c_str());
            tx->set_current_exec_status(data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_fail);
            if (tx->is_send_tx() || tx->is_self_tx()) {
                xwarn("[xtop_account_vm::pack] fail send/self tx[%" PRIu64 "] add to failed assemble", i);
                output.failed_tx_assemble.emplace_back(tx);
            } else if (tx->is_recv_tx()) {
                xwarn("[xtop_account_vm::pack] fail recv tx[%" PRIu64 "] add to success assemble", i);
                output.success_tx_assemble.emplace_back(tx);
            } else {
                xerror("[xtop_account_vm::pack] invalid tx type: %" PRIi32, tx->get_tx_type());
                assert(false);
            }
        } else {
            if (!r.output.receipt_data.empty()) {
                auto const& receipt_data = r.output.receipt_data.at(contract_common::RECEITP_DATA_ASSET_OUT);
                tx->set_receipt_data(r.output.receipt_data);
            }
            tx->set_current_exec_status(data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
            xdbg("[xtop_account_vm::pack] tx[%" PRIu64 "] add to success assemble", i);
            output.success_tx_assemble.emplace_back(tx);
            for (auto & follow_up : r.output.followup_transaction_data) {
                auto const & follow_up_tx = follow_up.followed_transaction;
                follow_up_tx->set_push_pool_timestamp(param.timeofday);
                if (follow_up.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::immediately) {
                    // followup_tx of success tx is success
                    xdbg("[xtop_account_vm::pack] immediately followup_tx of tx[%" PRIu64 "] add to success assemble", i);
                    follow_up_tx->set_current_exec_status(data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
                    output.success_tx_assemble.emplace_back(follow_up_tx);
                } else if (follow_up.schedule_type == contract_common::xfollowup_transaction_schedule_type_t::delay) {
                    assert(follow_up.execute_type == contract_common::xfollowup_transaction_execute_type_t::unexecuted);
                    xdbg("[xtop_account_vm::pack] delay followup_tx of tx[%" PRIu64 "] add to delay assemble, set last nonce: %" PRIu64, i, last_nonce);
                    follow_up_tx->get_transaction()->set_last_trans_hash_and_nonce(last_hash, last_nonce);
                    follow_up_tx->get_transaction()->set_digest();
                    follow_up_tx->get_transaction()->set_digest();
                    last_hash = follow_up_tx->get_tx_hash_256();
                    last_nonce = follow_up_tx->get_tx_nonce();
                    output.delay_tx_assemble.emplace_back(follow_up_tx);
                } else {
                    xerror("[xtop_account_vm::pack] invalid follow_up tx schedule type: %" PRIi32, follow_up.schedule_type);
                    assert(false);
                }
            }
        }
    }
    // set recv num
    if (recv_tx_num_new != recv_tx_num) {
        sa.set_property_cell_value<xproperty_type_t::map>(xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, xproperty_category_t::system},
                                                          data::XPROPERTY_TX_INFO_RECVTX_NUM,
                                                          top::to_bytes<uint64_t>(recv_tx_num_new),
                                                          ec);
    }
    // set create time
    xtypeless_property_identifier_t time_property{data::XPROPERTY_ACCOUNT_CREATE_TIME, xproperty_category_t::system};
    auto time_property_exist = sa.property_exist(xproperty_identifier_t{time_property, xproperty_type_t::uint64}, ec);
    top::error::throw_error(ec);
    if (!time_property_exist) {
        auto create_time = param.clock == 0 ? base::TOP_BEGIN_GMTIME : param.clock;
        sa.create_property(xproperty_identifier_t{time_property, xproperty_type_t::uint64}, ec);
        top::error::throw_error(ec);
        sa.set_property<xproperty_type_t::uint64>(time_property, create_time, ec);
        top::error::throw_error(ec);
    }

    if (output.success_tx_assemble.empty()) {
        output.status.ec = error::xenum_errc::transaction_all_failed;
        return output;
    }

    output.binlog_pack = result.binlog_pack;
    output.bincode_pack = result.bincode_pack;
    assert(!output.binlog_pack.empty());
    for (auto const & pair : output.binlog_pack) {
        assert(!pair.second.empty());
    }
    assert(!output.bincode_pack.empty());
    for (auto const & pair : output.bincode_pack) {
        assert(!pair.second.empty());
    }

    return output;
}

NS_END2
