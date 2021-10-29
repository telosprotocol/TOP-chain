// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsystem_contract_runtime/xsystem_contract_manager.h"

#include "xbasic/xutility.h"
#include "xconfig/xpredefined_configurations.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xtop_action_generator.h"
#include "xcontract_runtime/xtransaction_execution_result.h"
#include "xdata/xblocktool.h"
#include "xdata/xdatautil.h"
#include "xdata/xlightunit.h"
#include "xdata/xtransaction_v2.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"
#include "xsystem_contracts/xtransfer_contract.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_archive_contract_new.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_edge_contract_new.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_rec_contract_new.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_zec_contract_new.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract_new.h"
#include "xvm/xsystem_contracts/xelection/xzec/xzec_elect_consensus_group_contract_new.h"
#include "xvm/xsystem_contracts/xelection/xzec/xzec_group_association_contract_new.h"
#include "xvm/xsystem_contracts/xelection/xzec/xzec_standby_pool_contract_new.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract_new.h"
#include "xvm/xsystem_contracts/xreward/xzec_reward_contract_new.h"
#include "xvm/xsystem_contracts/xslash/xtable_statistic_info_collection_contract_new.h"

NS_BEG3(top, contract_runtime, system)

xtop_contract_deployment_data::xtop_contract_deployment_data(common::xnode_type_t node_type,
                                                             xsniff_type_t sniff_type,
                                                             xsniff_broadcast_config_t broadcast_config,
                                                             xsniff_timer_config_t timer_config,
                                                             xsniff_block_config_t block_config)
  : node_type(node_type), sniff_type(sniff_type), broadcast_config(broadcast_config), timer_config(timer_config), block_config(block_config) {
}

void xtop_system_contract_manager::deploy(observer_ptr<base::xvblockstore_t> const & blockstore) {
    deploy_system_contract<system_contracts::xrec_elect_rec_contract_new_t>(common::xaccount_address_t{sys_contract_rec_elect_rec_addr},
                                                                            common::xnode_type_t::rec,
                                                                            xsniff_type_t::timer,
                                                                            xsniff_broadcast_config_t{xsniff_broadcast_type_t::all, xsniff_broadcast_policy_t::all_block},
                                                                            xsniff_timer_config_t{config::xrec_election_interval_onchain_goverance_parameter_t::name, "on_timer"},
                                                                            xsniff_block_config_t{},
                                                                            blockstore);

    deploy_system_contract<system_contracts::xrec_elect_zec_contract_new_t>(common::xaccount_address_t{sys_contract_rec_elect_zec_addr},
                                                                            common::xnode_type_t::rec,
                                                                            xsniff_type_t::timer,
                                                                            xsniff_broadcast_config_t{xsniff_broadcast_type_t::all, xsniff_broadcast_policy_t::all_block},
                                                                            xsniff_timer_config_t{config::xzec_election_interval_onchain_goverance_parameter_t::name, "on_timer"},
                                                                            xsniff_block_config_t{},
                                                                            blockstore);

    deploy_system_contract<system_contracts::xrec_standby_pool_contract_new_t>(
        common::xaccount_address_t{sys_contract_rec_standby_pool_addr},
        common::xnode_type_t::rec,
        xsniff_type_t::timer,
        xsniff_broadcast_config_t{xsniff_broadcast_type_t::all, xsniff_broadcast_policy_t::all_block},
        xsniff_timer_config_t{config::xrec_standby_pool_update_interval_onchain_goverance_parameter_t::name, "on_timer"},
        xsniff_block_config_t{},
        blockstore);

    deploy_system_contract<system_contracts::xrec_registration_contract_new_t>(
        common::xaccount_address_t{sys_contract_rec_registration_addr}, common::xnode_type_t::rec, {}, {}, {}, {}, blockstore);

    deploy_system_contract<system_contracts::xrec_elect_archive_contract_new_t>(
        common::xaccount_address_t{sys_contract_rec_elect_archive_addr},
        common::xnode_type_t::rec,
        xsniff_type_t::timer,
        xsniff_broadcast_config_t{xsniff_broadcast_type_t::all, xsniff_broadcast_policy_t::all_block},
        xsniff_timer_config_t{config::xarchive_election_interval_onchain_goverance_parameter_t::name, "on_timer"},
        xsniff_block_config_t{},
        blockstore);

    deploy_system_contract<system_contracts::xrec_elect_edge_contract_new_t>(common::xaccount_address_t{sys_contract_rec_elect_edge_addr},
                                                                             common::xnode_type_t::rec,
                                                                             xsniff_type_t::timer,
                                                                             xsniff_broadcast_config_t{xsniff_broadcast_type_t::all, xsniff_broadcast_policy_t::all_block},
                                                                             xsniff_timer_config_t{config::xedge_election_interval_onchain_goverance_parameter_t::name, "on_timer"},
                                                                             xsniff_block_config_t{},
                                                                             blockstore);

    deploy_system_contract<system_contracts::xzec_elect_consensus_group_contract_new_t>(
        common::xaccount_address_t{sys_contract_zec_elect_consensus_addr},
        common::xnode_type_t::zec,
        xsniff_type_t::timer,
        xsniff_broadcast_config_t{xsniff_broadcast_type_t::all, xsniff_broadcast_policy_t::all_block},
        xsniff_timer_config_t{config::xzone_election_trigger_interval_onchain_goverance_parameter_t::name, "on_timer"},
        xsniff_block_config_t{},
        blockstore);

    deploy_system_contract<system_contracts::xzec_standby_pool_contract_new_t>(
        common::xaccount_address_t{sys_contract_zec_standby_pool_addr},
        common::xnode_type_t::zec,
        xsniff_type_t::timer,
        xsniff_broadcast_config_t{xsniff_broadcast_type_t::all, xsniff_broadcast_policy_t::all_block},
        xsniff_timer_config_t{config::xzec_standby_pool_update_interval_onchain_goverance_parameter_t::name, "on_timer"},
        xsniff_block_config_t{},
        blockstore);

    deploy_system_contract<system_contracts::xtable_statistic_info_collection_contract_new>(
        common::xaccount_address_t{sys_contract_sharding_statistic_info_addr},
        common::xnode_type_t::consensus_validator,
        static_cast<xsniff_type_t>((uint32_t)xsniff_type_t::timer | (uint32_t)xsniff_type_t::block),
        {},
        {config::xtable_statistic_report_schedule_interval_onchain_goverance_parameter_t::value, "report_summarized_statistic_info"},
        {common::xaccount_address_t{sys_contract_sharding_table_block_addr}, common::xaccount_address_t{sys_contract_sharding_statistic_info_addr}, "on_collect_statistic_info"},
        blockstore);

    deploy_system_contract<system_contracts::xgroup_association_contract_new_t>(common::xaccount_address_t{sys_contract_zec_group_assoc_addr},
                                                                                common::xnode_type_t::zec,
                                                                                xsniff_type_t::invalid,
                                                                                xsniff_broadcast_config_t{},
                                                                                xsniff_timer_config_t{},
                                                                                xsniff_block_config_t{},
                                                                                blockstore);
}

bool xtop_system_contract_manager::contains(common::xaccount_address_t const & address) const noexcept {
    return m_system_contract_deployment_data.find(address) != std::end(m_system_contract_deployment_data);
}

observer_ptr<contract_common::xbasic_contract_t> xtop_system_contract_manager::system_contract(common::xaccount_address_t const & address) const noexcept {
    common::xaccount_address_t contract_address{address};

    if (address.type() == base::enum_vaccount_addr_type_native_contract && address.ledger_id().zone_id() == common::xconsensus_zone_id) {
        contract_address = common::xaccount_address_t{address.base_address()};
    }

    auto const it = m_system_contracts.find(contract_address);
    if (it != std::end(m_system_contracts)) {
        return top::make_observer(top::get<std::unique_ptr<system_contracts::xbasic_system_contract_t>>(*it).get());
    }

    xdbg("system_contract_manager: contract %s not found", address.value().c_str());
    return nullptr;
}

std::unordered_map<common::xaccount_address_t, xcontract_deployment_data_t> const & xtop_system_contract_manager::deployment_data() const noexcept {
    return m_system_contract_deployment_data;
}

void xtop_system_contract_manager::init_system_contract(common::xaccount_address_t const & contract_address, observer_ptr<base::xvblockstore_t> const & blockstore) {
    if (blockstore->exist_genesis_block(contract_address.value())) {
        xdbg("xtop_system_contract_manager::init_contract_chain contract account %s genesis block exist", contract_address.c_str());
        return;
    }
    xdbg("xtop_system_contract_manager::init_contract_chain contract account %s genesis block not exist", contract_address.c_str());

    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();
    data::xproperty_asset asset_out{0};
    tx->make_tx_run_contract(asset_out, "setup", "");
    tx->set_same_source_target_address(contract_address.value());
    tx->set_digest();
    tx->set_len();

    xobject_ptr_t<base::xvbstate_t> bstate =
        make_object_ptr<base::xvbstate_t>(contract_address.value(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    state_accessor::xstate_accessor_t sa{top::make_observer(bstate.get()), state_accessor::xstate_access_control_data_t{}};
    auto contract_state =
        top::make_unique<contract_common::xcontract_state_t>(contract_address, top::make_observer(std::addressof(sa)), contract_common::xcontract_execution_param_t{});

    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    auto action = contract_runtime::xaction_generator_t::generate(cons_tx);

    auto contract_obj = system_contract(contract_address);
    auto contract_ctx= top::make_unique<contract_common::xcontract_execution_context_t>(std::move(action), make_observer(contract_state.get())); // action will be moved into xcontract_execution_context_t.
    contract_ctx->consensus_action_stage(xconsensus_action_stage_t::recv); // default stage set to target to match previous behavior
    assert(action == nullptr);
    xtransaction_execution_result_t result = contract_obj->execute(top::make_observer(contract_ctx));
    assert(!result.status.ec);
    xtransaction_result_t consensus_result;
    consensus_result.m_property_binlog = result.output.binlog;
    consensus_result.m_full_state = result.output.contract_state_snapshot;
    assert(!result.output.binlog.empty());
    assert(!result.output.contract_state_snapshot.empty());

    base::xauto_ptr<base::xvblock_t> block(data::xblocktool_t::create_genesis_lightunit(contract_address.value(), tx, consensus_result));
    xassert(block);

    base::xvaccount_t _vaddr(block->get_account());
    auto ret = blockstore->store_block(_vaddr, block.get());
    if (!ret) {
        xerror("xtop_system_contract_manager::init_contract_chain %s genesis block fail", contract_address.c_str());
        return;
    }
    xdbg("xtop_system_contract_manager::init_contract_chain contract_adress: %s, %s", contract_address.c_str(), ret ? "SUCC" : "FAIL");
}

NS_END3
