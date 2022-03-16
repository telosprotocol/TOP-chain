// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xzec/xzec_standby_pool_contract.h"

#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelect_transaction.hpp"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xnative_contract_address.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xproperty.h"

#include <cinttypes>

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "ZecStandby_"
#define XZEC_STANDBY XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG4(top, xvm, system_contracts, zec)

using top::data::election::xstandby_node_info_t;
using top::data::election::xstandby_result_store_t;

xtop_zec_standby_pool_contract::xtop_zec_standby_pool_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

xcontract::xcontract_base * xtop_zec_standby_pool_contract::clone() {
    return new xtop_zec_standby_pool_contract{this->m_network_id};
}

void xtop_zec_standby_pool_contract::setup() {
    STRING_CREATE(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT);
    STRING_CREATE(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME);

    std::string last_read_rec_standby_pool_contract_height{"0"};
    STRING_SET(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT, last_read_rec_standby_pool_contract_height);

    std::string last_read_rec_standby_pool_contract_logic_time{"0"};
    STRING_SET(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME, last_read_rec_standby_pool_contract_logic_time);
}

void xtop_zec_standby_pool_contract::on_timer(common::xlogic_time_t const current_time) {
    XMETRICS_TIME_RECORD(XZEC_STANDBY "on_timer_all_time");
    // get standby nodes from rec_standby_pool
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xzec_standby_pool_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_zec_standby_pool_addr, "xzec_standby_pool_contract_t instance is not triggled by sys_contract_zec_standby_pool_addr");
    // XCONTRACT_ENSURE(current_time <= TIME(), "xzec_standby_pool_contract_t::on_timer current_time > consensus leader's time");

    xdbg("[xzec_standby_pool_contract_t] on_timer: %" PRIu64, current_time);

    bool update_rec_standby_pool_contract_read_status{false};

    auto const last_read_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT)));
    auto const last_read_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME)));

    auto const height_step_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_height_step_limitation);
    auto const timeout_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_logic_timeout_limitation);

    uint64_t latest_height = get_blockchain_height(sys_contract_rec_standby_pool_addr);
    xdbg("[xzec_standby_pool_contract_t] get_latest_height: %" PRIu64, latest_height);

    XCONTRACT_ENSURE(latest_height >= last_read_height, "xzec_standby_pool_contract_t::on_timer latest_height < last_read_height");
    if (latest_height == last_read_height) {
        XMETRICS_PACKET_INFO(XZEC_STANDBY "update_status", "next_read_height", last_read_height, "current_time", current_time)
        STRING_SET(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME, std::to_string(current_time));
        return;
    }

    // calc current_read_height:
    uint64_t next_read_height = last_read_height;
    if (latest_height - last_read_height >= height_step_limitation) {
        next_read_height = last_read_height + height_step_limitation;
        update_rec_standby_pool_contract_read_status = true;
    }
    if (!update_rec_standby_pool_contract_read_status && current_time - last_read_time > timeout_limitation) {
        next_read_height = latest_height;
        update_rec_standby_pool_contract_read_status = true;
    }
    xinfo("[xzec_standby_pool_contract_t] next_read_height: %" PRIu64, next_read_height);

    base::xauto_ptr<xblock_t> block_ptr = get_block_by_height(sys_contract_rec_standby_pool_addr, next_read_height);
    std::string result;
    if (block_ptr == nullptr) {
        xwarn("[xzec_standby_pool_contract_t] fail to get the rec_standby_pool data. next_read_block height:%" PRIu64, next_read_height);
        return;
    }

    if (update_rec_standby_pool_contract_read_status) {
        XMETRICS_PACKET_INFO(XZEC_STANDBY "update_status", "next_read_height", next_read_height, "current_time", current_time)
        STRING_SET(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT, std::to_string(next_read_height));
        STRING_SET(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME, std::to_string(current_time));
    }
    return;
}

NS_END4
