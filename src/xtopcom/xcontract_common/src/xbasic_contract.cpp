// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xbasic_contract.h"

#include "xbasic/xutility.h"
#include "xconfig/xchain_names.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xcontract_common/xerror/xerror.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_v2.h"
#include "xstake/xstake_algorithm.h"

#include <cassert>
#include <iostream>

NS_BEG2(top, contract_common)

xtop_contract_metadata::xtop_contract_metadata(common::xaccount_address_t const& account, xcontract_type_t type)
    : m_type(type), m_account(account) {
}

common::xaccount_address_t xtop_basic_contract::address() const {
    if (m_associated_execution_context != nullptr) {
        return m_associated_execution_context->contract_state()->state_account_address();
    } else {
        return m_contract_meta.m_account;
    }
}

xcontract_type_t xtop_basic_contract::type() const {
    return m_contract_meta.m_type;
}

void xtop_basic_contract::register_property(properties::xbasic_property_t * property) {
    m_property_initializer.register_property(make_observer(property));
}

common::xnetwork_id_t xtop_basic_contract::network_id() const {
    return common::xnetwork_id_t{config::to_chainid(XGET_CONFIG(chain_name))};
}

uint64_t xtop_basic_contract::balance() const {
    return m_balance.amount();
}

state_accessor::xtoken_t xtop_basic_contract::withdraw(std::uint64_t amount) {
    return m_balance.withdraw(amount);
}

void xtop_basic_contract::deposit(state_accessor::xtoken_t token) {
    assert(m_balance.symbol() == token.symbol());
    m_balance.deposit(std::move(token));
}

observer_ptr<xcontract_state_t> xtop_basic_contract::contract_state() const noexcept {
    assert(m_associated_execution_context != nullptr);
    return m_associated_execution_context->contract_state();
}

void xtop_basic_contract::asset_to_next_action(state_accessor::xtoken_t token) {
    base::xstream_t stream{base::xcontext_t::instance()};
    token.move_to(stream);

    std::error_code ec;
    write_receipt_data(contract_common::RECEITP_DATA_ASSET_OUT, xbyte_buffer_t{stream.data(), stream.data() + stream.size()}, ec);
    assert(!ec);
    top::error::throw_error(ec);
}

data::enum_xaction_type xtop_basic_contract::action_type() const {
    return m_associated_execution_context->action_type();
}

data::enum_xaction_type xtop_basic_contract::source_action_type() const {
    return m_associated_execution_context->source_action_type();
}

data::enum_xaction_type xtop_basic_contract::target_action_type() const {
    return m_associated_execution_context->target_action_type();
}

xbyte_buffer_t xtop_basic_contract::action_data() const {
    return m_associated_execution_context->action_data();
}

state_accessor::xtoken_t xtop_basic_contract::last_action_asset(std::error_code & ec) const {
    assert(!ec);

    auto const& receipt_data = m_associated_execution_context->input_receipt_data(RECEITP_DATA_ASSET_OUT);
    state_accessor::xtoken_t token;
    if (!receipt_data.empty()) {
        m_associated_execution_context->remove_input_receipt_data(RECEITP_DATA_ASSET_OUT);
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)receipt_data.data(), receipt_data.size());
        token.move_from(stream);
    }
    return std::move(token);
}

data::enum_xtransaction_type xtop_basic_contract::transaction_type() const {
    return m_associated_execution_context->transaction_type();
}

common::xaccount_address_t xtop_basic_contract::sender() const {
    return m_associated_execution_context->sender();
}

common::xaccount_address_t xtop_basic_contract::recver() const {
    return m_associated_execution_context->recver();
}

void xtop_basic_contract::call(common::xaccount_address_t const & target_addr,
                               std::string const & method_name,
                               std::string const & method_params,
                               xfollowup_transaction_schedule_type_t type) {
    assert(type != xfollowup_transaction_schedule_type_t::invalid);
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();

    tx->make_tx_run_contract(data::xproperty_asset{0}, method_name, method_params);
    tx->set_different_source_target_address(address().value(), target_addr.value());
    data::xcons_transaction_ptr_t cons_tx;
    if (type == xfollowup_transaction_schedule_type_t::immediately) {
        // delay type need process nonce final
        auto latest_hash = contract_state()->latest_followup_tx_hash();
        auto latest_nonce = contract_state()->latest_followup_tx_nonce();
        tx->set_last_trans_hash_and_nonce(latest_hash, latest_nonce);
        tx->set_digest();
        tx->set_len();
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
        contract_state()->latest_followup_tx_hash(cons_tx->get_tx_hash_256());
        contract_state()->latest_followup_tx_nonce(cons_tx->get_tx_nonce());
        xdbg_info("xtop_basic_contract::call tx:%s,from:%s,to:%s,amount:%ld,nonce:%ld",
            tx->get_digest_hex_str().c_str(), address().value().c_str(), target_addr.value().c_str(), tx->get_tx_nonce());
    } else {
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    }

    xassert(cons_tx->is_self_tx() || cons_tx->is_send_tx());
    m_associated_execution_context->add_followup_transaction(std::move(cons_tx), type);
}

void xtop_basic_contract::call(common::xaccount_address_t const & target_addr,
                               std::string const & source_method_name,
                               std::string const & source_method_params,
                               std::string const & target_method_name,
                               std::string const & target_method_params,
                               xfollowup_transaction_schedule_type_t type) {
    assert(type != xfollowup_transaction_schedule_type_t::invalid);
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();

    tx->get_source_action().set_action_name(source_method_name);
    tx->get_source_action().set_action_param(source_method_params);
    tx->make_tx_run_contract(data::xproperty_asset{0}, target_method_name, target_method_params);
    tx->set_different_source_target_address(address().value(), target_addr.value());
    data::xcons_transaction_ptr_t cons_tx;
    if (type == xfollowup_transaction_schedule_type_t::immediately) {
        // delay type need process nonce final
        auto latest_hash = contract_state()->latest_followup_tx_hash();
        auto latest_nonce = contract_state()->latest_followup_tx_nonce();
        tx->set_last_trans_hash_and_nonce(latest_hash, latest_nonce);
        tx->set_digest();
        tx->set_len();
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
        contract_state()->latest_followup_tx_hash(cons_tx->get_tx_hash_256());
        contract_state()->latest_followup_tx_nonce(cons_tx->get_tx_nonce());
    } else {
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    }

    xassert(cons_tx->is_self_tx() || cons_tx->is_send_tx());
    m_associated_execution_context->add_followup_transaction(std::move(cons_tx), type);
}

void xtop_basic_contract::sync_call(common::xaccount_address_t const & target_addr,
                                    std::string const & method_name,
                                    std::string const & method_params,
                                    xfollowup_transaction_schedule_type_t type) {
    assert(false);  // not supported yet.

    bool valid{false};
    // address type check
    auto const & source_addr = address();
    if ((data::is_beacon_contract_address(source_addr) && data::is_beacon_contract_address(target_addr)) ||
        (data::is_zec_contract_address(source_addr) && data::is_zec_contract_address(target_addr)) ||
        (data::is_sys_sharding_contract_address(source_addr) && data::is_sys_sharding_contract_address(target_addr))) {
        if (data::account_map_to_table_id(source_addr).get_subaddr() == data::account_map_to_table_id(source_addr).get_subaddr()) {
            valid = true;
        }
    }
    if (!valid) {
        call(target_addr, method_name, method_params, type);
        return;
    }
    // only target action
    data::xcons_transaction_ptr_t cons_tx;
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();
    tx->make_tx_run_contract(data::xproperty_asset{0}, method_name, method_params);
    tx->set_different_source_target_address(address().value(), target_addr.value());
    cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    std::unique_ptr<data::xbasic_top_action_t const> action = top::make_unique<data::xsystem_consensus_action_t>(cons_tx);
    // exec
    // auto obj = m_associated_execution_context->system_contract(target_addr);
    auto ctx = make_unique<contract_common::xcontract_execution_context_t>(std::move(action), contract_state());
    // ctx->contract_state(target_addr);
    if (source_addr == target_addr) {
        ctx->consensus_action_stage(data::xconsensus_action_stage_t::self);
    } else {
        ctx->consensus_action_stage(data::xconsensus_action_stage_t::recv);
    }

    // auto result = obj->execute(top::make_observer(ctx));
    // TODO: follow up of result
    // assert(!result.status.ec);
    // switch address back
    // m_associated_execution_context->contract_state(source_addr);
}

void xtop_basic_contract::sync_call(common::xaccount_address_t const & target_addr,
                                    std::string const & source_method_name,
                                    std::string const & source_method_params,
                                    std::string const & method_name,
                                    std::string const & method_params,
                                    xfollowup_transaction_schedule_type_t type) {
    assert(false);  // not supported yet.

    bool valid{false};
    // address type check
    auto const & source_addr = address();
    if ((data::is_beacon_contract_address(source_addr) && data::is_beacon_contract_address(target_addr)) ||
        (data::is_zec_contract_address(source_addr) && data::is_zec_contract_address(target_addr)) ||
        (data::is_sys_sharding_contract_address(source_addr) && data::is_sys_sharding_contract_address(target_addr))) {
        if (data::account_map_to_table_id(source_addr).get_subaddr() == data::account_map_to_table_id(source_addr).get_subaddr()) {
            valid = true;
        }
    }
    if (!valid) {
        call(target_addr, method_name, method_params, type);
        return;
    }
    // only target action
    data::xcons_transaction_ptr_t cons_tx;
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();
    tx->get_source_action().set_action_name(source_method_name);
    tx->get_source_action().set_action_param(source_method_params);
    tx->make_tx_run_contract(data::xproperty_asset{0}, method_name, method_params);
    tx->set_different_source_target_address(address().value(), target_addr.value());
    cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    std::unique_ptr<data::xbasic_top_action_t const> action = top::make_unique<data::xsystem_consensus_action_t>(cons_tx);
    // exec
    // auto obj = m_associated_execution_context->system_contract(target_addr);
    auto ctx = make_unique<contract_common::xcontract_execution_context_t>(std::move(action), contract_state());
    // ctx->contract_state(target_addr);
    if (source_addr == target_addr) {
        ctx->consensus_action_stage(data::xconsensus_action_stage_t::self);
    } else {
        ctx->consensus_action_stage(data::xconsensus_action_stage_t::recv);
    }

    // auto result = obj->execute(top::make_observer(ctx));
    // TODO: follow up of result
    // assert(!result.status.ec);
    // switch address back
    // m_associated_execution_context->contract_state(source_addr);
}

void xtop_basic_contract::transfer(common::xaccount_address_t const & target_addr, uint64_t amount, xfollowup_transaction_schedule_type_t type, std::error_code & ec) {
    assert(data::is_contract_address(common::xaccount_address_t{address()}));
    assert(type != xfollowup_transaction_schedule_type_t::invalid);
    if (data::is_user_contract_address(common::xaccount_address_t{address()})) {
        xwarn("xtop_basic_contract::transfer fail to create user contract transaction from:%s,to:%s,amount:%lu", address().value().c_str(), target_addr.value().c_str(), amount);
        ec = contract_common::error::xerrc_t::user_contract_forbid_create_transfer;
    }
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(address().value(), target_addr.value());
    tx->set_deposit(0);
    tx->set_fire_timestamp(timestamp());
    tx->set_expire_duration(0);
    data::xcons_transaction_ptr_t cons_tx;
    if (type == xfollowup_transaction_schedule_type_t::immediately) {
        // delay type need process nonce final
        auto latest_hash = contract_state()->latest_followup_tx_hash();
        auto latest_nonce = contract_state()->latest_followup_tx_nonce();
        tx->set_last_trans_hash_and_nonce(latest_hash, latest_nonce);
        tx->set_digest();
        tx->set_len();
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
        contract_state()->latest_followup_tx_hash(cons_tx->get_tx_hash_256());
        contract_state()->latest_followup_tx_nonce(cons_tx->get_tx_nonce());
        xdbg_info("xtop_basic_contract::transfer tx:%s,from:%s,to:%s,amount:%ld,nonce:%ld",
            tx->get_digest_hex_str().c_str(), address().value().c_str(), target_addr.value().c_str(), amount, tx->get_tx_nonce());
    } else {
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    }

    xassert(cons_tx->is_self_tx() || cons_tx->is_send_tx());
    m_associated_execution_context->add_followup_transaction(std::move(cons_tx), type);
}

void xtop_basic_contract::transfer(common::xaccount_address_t const & target_addr, uint64_t amount, xfollowup_transaction_schedule_type_t type) {
    std::error_code ec;
    transfer(target_addr, amount, type, ec);
    top::error::throw_error(ec);
}

void xtop_basic_contract::delay_followup(xfollowup_transaction_delay_param_t const & param) {
    xstake::xreward_dispatch_task task;
    task.onchain_timer_round = param.time;
    task.contract = param.target_address.value();
    task.action = param.method_name;
    if (task.action == xstake::XTRANSFER_ACTION) {
        std::map<std::string, uint64_t> map;
        base::xstream_t stream(base::xcontext_t::instance());
        map.emplace(task.contract, top::from_string<uint64_t>(task.params));
        stream << map;
        task.params = std::string((char *)stream.data(), stream.size());
    } else {
        task.params = param.method_params;
    }
    m_associated_execution_context->contract_state()->delay_followup(task);
}

void xtop_basic_contract::delay_followup(std::vector<xfollowup_transaction_delay_param_t> const & params) {
    std::vector<xstake::xreward_dispatch_task> tasks;
    for (auto const & param : params) {
        xstake::xreward_dispatch_task task;
        task.onchain_timer_round = param.time;
        task.contract = param.target_address.value();
        task.action = param.method_name;
        if (task.action == xstake::XTRANSFER_ACTION) {
            std::map<std::string, uint64_t> map;
            base::xstream_t stream(base::xcontext_t::instance());
            map.emplace(task.contract, top::from_string<uint64_t>(param.method_params));
            stream << map;
            task.params = std::string((char *)stream.data(), stream.size());
        } else {
            task.params = param.method_params;
        }
        tasks.emplace_back(task);
    }
    m_associated_execution_context->contract_state()->delay_followup(tasks);
}

void xtop_basic_contract::exec_delay_followup() {
    // only for reward contract now
    if (m_associated_execution_context->recver() != common::xaccount_address_t{sys_contract_zec_reward_addr}) {
        return;
    }
    state_accessor::properties::xtypeless_property_identifier_t property{xstake::XPORPERTY_CONTRACT_TASK_KEY};
    auto tasks = m_associated_execution_context->contract_state()->delay_followup();
    const uint32_t task_num_per_round = 16;
    xinfo("[xtop_basic_contract::exec_delay_followup] tasks size: %zu, task_num_per_round: %u", tasks.size(), task_num_per_round);

    for (size_t i = 0; i < task_num_per_round; i++) {
        auto it = tasks.begin();
        if (it == tasks.end()) {
            return;
        }
        auto const id = it->first;
        auto const task = it->second;
        if (task.action == xstake::XREWARD_CLAIMING_ADD_NODE_REWARD || task.action == xstake::XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD) {
            // debug output
            base::xstream_t stream_params(base::xcontext_t::instance(), (uint8_t *)task.params.c_str(), (uint32_t)task.params.size());
            uint64_t onchain_timer_round;
            std::map<std::string, top::xstake::uint128_t> rewards;
            stream_params >> onchain_timer_round;
            stream_params >> rewards;
            for (auto const & r : rewards) {
                xinfo("[xzec_reward_contract::execute_task] contract: %s, action: %s, account: %s, reward: [%llu, %u], onchain_timer_round: %llu\n",
                      task.contract.c_str(),
                      task.action.c_str(),
                      r.first.c_str(),
                      static_cast<uint64_t>(r.second / xstake::REWARD_PRECISION),
                      static_cast<uint32_t>(r.second % xstake::REWARD_PRECISION),
                      task.onchain_timer_round);
            }
            call(common::xaccount_address_t{task.contract}, task.action, task.params, xfollowup_transaction_schedule_type_t::immediately);
        } else if (task.action == xstake::XTRANSFER_ACTION) {
            // issuances stored in task.params
            std::map<std::string, uint64_t> issuances;
            base::xstream_t seo_stream(base::xcontext_t::instance(), (uint8_t *)task.params.c_str(), (uint32_t)task.params.size());
            seo_stream >> issuances;
            for (auto const & issue : issuances) {
                xinfo("[xtop_basic_contract::exec_delay_followup] action: %s, contract account: %s, issuance: %llu, onchain_timer_round: %llu\n",
                      task.action.c_str(),
                      issue.first.c_str(),
                      issue.second,
                      task.onchain_timer_round);
                transfer(common::xaccount_address_t{issue.first}, issue.second, xfollowup_transaction_schedule_type_t::immediately);
            }
        }
        m_associated_execution_context->contract_state()->remove_property_cell<state_accessor::properties::xproperty_type_t::map>(property, id);
        tasks.erase(it);
    }
}

std::vector<xfollowup_transaction_datum_t> xtop_basic_contract::followup_transaction() const {
    return m_associated_execution_context->followup_transaction();
}

void xtop_basic_contract::reset_execution_context(observer_ptr<xcontract_execution_context_t> exe_ctx) {
    m_associated_execution_context = exe_ctx;
}

observer_ptr<properties::xproperty_initializer_t const> xtop_basic_contract::property_initializer() const noexcept {
    return make_observer(std::addressof(m_property_initializer));
}

xbyte_buffer_t xtop_basic_contract::receipt_data(std::string const & key) const {
    return m_associated_execution_context->input_receipt_data(key);
}

void xtop_basic_contract::write_receipt_data(std::string const & key, xbyte_buffer_t value, std::error_code & ec) {
    auto& receipt_data = m_associated_execution_context->output_receipt_data();
    if (receipt_data.item_exist(key)) {
        ec = error::xerrc_t::receipt_data_already_exist;
        return;
    }

    receipt_data.add_item(key, value);
}

common::xlogic_time_t xtop_basic_contract::time() const {
    return m_associated_execution_context->contract_state()->time();
}

common::xlogic_time_t xtop_basic_contract::timestamp() const {
    return m_associated_execution_context->contract_state()->timestamp();
}

std::string const & xtop_basic_contract::random_seed() const noexcept {
    return m_associated_execution_context->contract_state()->random_seed();
}

uint64_t xtop_basic_contract::state_height(common::xaccount_address_t const & address) const {
    return m_associated_execution_context->contract_state()->state_height(address);
}

bool xtop_basic_contract::block_exist(common::xaccount_address_t const & address, uint64_t height) const {
    return m_associated_execution_context->contract_state()->block_exist(address, height);
}

NS_END2
