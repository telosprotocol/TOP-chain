// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelect/client/xelect_client_process.h"

#include "xbasic/xutility.h"
#include "xchain_fork/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xevent_timer.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvstate.h"
#include "xstatestore/xstatestore_face.h"

#include <cinttypes>

NS_BEG2(top, elect)
using base::xstring_utl;
using data::xblock_ptr_t;
using mbus::xevent_ptr_t;
using mbus::xevent_store_t;

xelect_client_process::xelect_client_process(common::xnetwork_id_t const & network_id,
                                             observer_ptr<mbus::xmessage_bus_face_t> const & mb,
                                             elect_update_handler2 cb2,
                                             observer_ptr<time::xchain_time_face_t> const & xchain_timer)
  : xbase_sync_event_monitor_t(mb), m_network_id{network_id}, m_update_handler2{std::move(cb2)}, m_xchain_timer(xchain_timer) {
    assert(!broadcast(m_network_id));
    xdbg("xelect_client_process created %p", mb.get());
    // mb->add_listener((int)mbus::xevent_major_type_store, std::bind(&xelect_client_process::push_event, this, std::placeholders::_1));
    mb->add_listener((int)mbus::xevent_major_type_chain_timer, std::bind(&xelect_client_process::push_event, this, std::placeholders::_1));
    m_xchain_timer->watch("elect_client_process_on_timer", 1, std::bind(&xelect_client_process::update_election_status, this, std::placeholders::_1));
}

bool xelect_client_process::filter_event(const xevent_ptr_t & e) {
    xdbg("xelect_client_process::filter_event major type %d minor type %d", static_cast<int>(e->major_type), static_cast<int>(e->minor_type));
    switch (e->major_type) {
    case mbus::xevent_major_type_chain_timer:
        assert(dynamic_xobject_ptr_cast<mbus::xevent_chain_timer_t>(e));
        return dynamic_xobject_ptr_cast<mbus::xevent_chain_timer_t>(e)->time_block->get_account() == sys_contract_beacon_timer_addr;
    default:
        return false;
    }
}

void xelect_client_process::process_event(const xevent_ptr_t & e) {
    switch (e->major_type) {
    case mbus::xevent_major_type_chain_timer:
        process_timer(e);
        break;
    default:
        assert(false);
        break;
    }
}

void xelect_client_process::process_timer(const mbus::xevent_ptr_t & e) {
    assert(dynamic_xobject_ptr_cast<mbus::xevent_chain_timer_t>(e));
    auto const & event = dynamic_xobject_ptr_cast<mbus::xevent_chain_timer_t>(e);
    auto const * block = event->time_block;

    xdbg("[xelect_client_process::process_timer] update xchain timer to %" PRIu64, block->get_height());
    m_xchain_timer->update_time(block->get_height(), time::xlogic_timer_update_strategy_t::discard_old_value);
}

uint64_t xelect_client_process::get_new_election_height(data::xunitstate_ptr_t const& unitstate) const {
    common::xaccount_address_t const & contract_address = unitstate->account_address();
    if (contract_address == relay_make_block_contract_address) {
        std::string height_str;
        unitstate->string_get(data::system_contract::XPROPERTY_RELAY_ELECT_PACK_HEIGHT, height_str);
        if (height_str.empty()) {
            xerror("[zec election] zone elect finish for relay with empty pack height.block=%s", unitstate->get_bstate()->dump().c_str());
            return 0;
        }
        return static_cast<std::uint64_t>(std::stoull(height_str));
    } else {
        return unitstate->height();
    }
}

void xelect_client_process::process_election_block(data::xunitstate_ptr_t const& unitstate, common::xlogic_time_t const current_time) {
    common::xaccount_address_t const & contract_address = unitstate->account_address();
    auto const it = m_election_status.find(contract_address);
    if (it == std::end(m_election_status)) {
        xwarn("xelect_client_process::process_election_block not found in m_election_status,block:%s", unitstate->get_bstate()->dump().c_str());
        return;
    }

    assert(current_time > m_election_status[contract_address].last_update_time);

    auto const local_height = top::get<xinternal_election_status_t>(*it).height;
    if (local_height >= unitstate->height()) {
        xwarn("xelect_client_process::process_election_block block height is lower,local_height:%llu,block:%s", local_height, unitstate->get_bstate()->dump().c_str());
        return;
    }

    xinfo("xelect_client_process::process_elect %s, %" PRIu64, contract_address.to_string().c_str(), unitstate->height());

    uint64_t const new_election_height = get_new_election_height(unitstate);
    if (local_height >= new_election_height) {
        xwarn("xelect_client_process::process_election_block block height is lower,local_height:%llu,new height:%llu,block:%s", local_height, new_election_height, unitstate->get_bstate()->dump().c_str());
        return;
    }

    auto const & property_names = data::election::get_property_name_by_addr(contract_address);
    for (auto const & property : property_names) {
        std::string result;
        unitstate->string_get(property, result);
        if (result.empty()) {
            xerror("[zec election] zone elect finish with empty result. property=%s,block=%s", property.c_str(), unitstate->get_bstate()->dump().c_str());
            continue;
        }
        xdbg("xelect_client_process::process_elect %s, %" PRIu64 " done", contract_address.to_string().c_str(), unitstate->height());
        using top::data::election::xelection_result_store_t;
        auto const& election_result_store = codec::msgpack_decode<xelection_result_store_t>({ std::begin(result), std::end(result) });
        if (election_result_store.empty()) {
            if (!(contract_address == common::xaccount_address_t{sys_contract_rec_elect_archive_addr} &&
                  property == data::election::get_property_by_group_id(common::xlegacy_exchange_group_id))) {
                if (unitstate->height() != 0) {
                    xerror("xelect_client_process::process_elect decode property empty, block=%s", unitstate->get_bstate()->dump().c_str());
                } else {
                    xwarn("xelect_client_process::process_elect decode property empty, block=%s", unitstate->get_bstate()->dump().c_str());
                }
                return;
            }
        }

        for (auto const& election_result_info : election_result_store) {
            if (top::get<common::xnetwork_id_t const>(election_result_info) != m_network_id) {
                continue;
            }

            auto const & election_network_result = top::get<data::election::xelection_network_result_t>(election_result_info);
            for (auto const & election_type_result : election_network_result) {
                auto const node_type = top::get<common::xnode_type_t const>(election_type_result);

                xdbg("start processing %s data", common::to_string(node_type).c_str());

                if (common::has<common::xnode_type_t::consensus>(node_type) || common::has<common::xnode_type_t::consensus_validator>(node_type) ||
                    common::has<common::xnode_type_t::consensus_auditor>(node_type)) {
                    m_update_handler2(election_result_store, common::xconsensus_zone_id, new_election_height, false);
                } else if (common::has<common::xnode_type_t::zec>(node_type)) {
                    m_update_handler2(election_result_store, common::xzec_zone_id, new_election_height, false);
                } else if (common::has<common::xnode_type_t::committee>(node_type)) {
                    m_update_handler2(election_result_store, common::xcommittee_zone_id, new_election_height, false);
                } else if (common::has<common::xnode_type_t::edge>(node_type)) {
                    m_update_handler2(election_result_store, common::xedge_zone_id, new_election_height, false);
                } else if (common::has<common::xnode_type_t::storage>(node_type)) {
                    m_update_handler2(election_result_store, common::xstorage_zone_id, new_election_height, false);
                } else if (common::has<common::xnode_type_t::fullnode>(node_type)) {
                    m_update_handler2(election_result_store, common::xfullnode_zone_id, new_election_height, false);
                } else if (common::has<common::xnode_type_t::evm>(node_type)) {
                    m_update_handler2(election_result_store, common::xevm_zone_id, new_election_height, false);
                } else if (common::has<common::xnode_type_t::relay>(node_type)) {
                    m_update_handler2(election_result_store, common::xrelay_zone_id, new_election_height, false);
                } else {
                    assert(false);
                }
            }
        }
    }

    top::get<xinternal_election_status_t>(*it).height = new_election_height;
    top::get<xinternal_election_status_t>(*it).last_update_time = current_time;
}

void xelect_client_process::process_election_contract(common::xaccount_address_t const& contract_address,
                                                      common::xlogic_time_t const current_time,
                                                      common::xlogic_time_t const update_interval) {
    if (current_time > m_election_status[contract_address].last_update_time &&
        current_time - m_election_status[contract_address].last_update_time >= update_interval) {
        
        data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_change_state(contract_address);
        if (nullptr == unitstate) {
            xerror("xelect_client_process::process_election_contract fail-load state.%s", contract_address.to_string().c_str());
            return;
        }

        process_election_block(unitstate, current_time);
    }
}

void xelect_client_process::update_election_status(common::xlogic_time_t current_time) {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
    constexpr config::xinterval_t update_divider = 4;
#else
    constexpr config::xinterval_t committee_group_update_interval = 180;
    constexpr config::xinterval_t consensus_group_update_interval = 60;
    constexpr config::xinterval_t nonconsensus_group_update_interval = 60;
#endif

    {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_rec_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(rec_election_interval) / update_divider;
#else
        auto const update_rec_interval = committee_group_update_interval;      // for mainnet & bounty
#endif
        process_election_contract(common::xaccount_address_t{sys_contract_rec_elect_rec_addr}, current_time, update_rec_interval);
    }

    {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_archive_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(archive_election_interval) / update_divider;
#else
        auto const update_archive_interval = nonconsensus_group_update_interval;  // for mainnet & bounty
#endif
        process_election_contract(common::xaccount_address_t{sys_contract_rec_elect_archive_addr}, current_time, update_archive_interval);
    }

    {
        // auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();
        // if (chain_fork::xchain_fork_config_center_t::is_forked(fork_config.standalone_exchange_point, current_time)) {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_exchange_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(exchange_election_interval) / update_divider;
#else
        auto const update_exchange_interval = nonconsensus_group_update_interval;  // for mainnet & bounty
#endif
        process_election_contract(common::xaccount_address_t{sys_contract_rec_elect_exchange_addr}, current_time, update_exchange_interval);
        // }
    }

    {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_fullnode_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullnode_election_interval) / update_divider;
#else
        auto const update_fullnode_interval = nonconsensus_group_update_interval;  // for mainnet & bounty
#endif
        process_election_contract(common::xaccount_address_t{sys_contract_rec_elect_fullnode_addr}, current_time, update_fullnode_interval);
    }

    {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_edge_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(edge_election_interval) / update_divider;
#else
        auto const update_edge_interval = nonconsensus_group_update_interval;      // for mainnet & bounty
#endif
        process_election_contract(common::xaccount_address_t{sys_contract_rec_elect_edge_addr}, current_time, update_edge_interval);
    }

    {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_zec_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(zec_election_interval) / update_divider;
#else
        auto const update_zec_interval = committee_group_update_interval;          // for mainnet & bounty
#endif
        process_election_contract(common::xaccount_address_t{sys_contract_rec_elect_zec_addr}, current_time, update_zec_interval);
    }

    {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_consensus_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval) / update_divider;
#else
        auto const update_consensus_interval = consensus_group_update_interval;    // for mainnet & bounty
#endif
        process_election_contract(common::xaccount_address_t{sys_contract_zec_elect_consensus_addr}, current_time, update_consensus_interval);
    }

    {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_eth_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_election_interval) / update_divider;
#else
        auto const update_eth_interval = consensus_group_update_interval;
#endif
        process_election_contract(zec_elect_eth_contract_address, current_time, update_eth_interval);
    }

    {
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        auto const update_relay_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(relay_election_interval) / update_divider;
#else
        auto const update_relay_interval = consensus_group_update_interval;
#endif
        process_election_contract(relay_make_block_contract_address, current_time, update_relay_interval);
    }
}

NS_END2
