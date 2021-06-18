// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <string>

#include "xbase/xns_macro.h"
#include "xbase/xmem.h"

NS_BEG2(top, tcc)

// for TCC (TOP CHAIN Comittee)

// caution: property name can't exceed 16 bytes
#define ONCHAIN_PARAMS            "@150"  //"onchain_params" // map of <string, string>
#define SYSTEM_GENERATED_ID       "@151"  //"system_id" // string
#define PROPOSAL_MAP_ID           "@152"  //"proposal_map" // map of <string, string(proposal details)>
//#define COSIGN_MAP_ID           "cosigning_map" // map of <string, map<std::string, bool> >
#define VOTE_MAP_ID               "@154"  //"voting_map" // map of <string, map<std::string, bool> >

#define CURRENT_VOTED_PROPOSAL    "@155"  //"@current_voted"

#define UPDATE_ACTION_PARAMETER   "update_action_parameter" // parameter update
#define UPDATE_ACTION_PLUGIN      "update_action_plugin"  // plug-in update (to be implemented)
#define UPDATE_ACTION_GLOBAL      "update_action_global"    // global update (to be implemented)

#define XPROPOSAL_TYPE_TO_STR(val) #val
enum class proposal_type: uint8_t {
    proposal_update_parameter = 1,
    proposal_update_asset,
    proposal_add_parameter,
    proposal_delete_parameter,
    proposal_update_parameter_incremental_add,
    proposal_update_parameter_incremental_delete,
};

constexpr size_t MODIFICATION_DESCRIPTION_SIZE = 500;

constexpr uint8_t priority_normal = 1;
constexpr uint8_t priority_important = 2;
constexpr uint8_t priority_critical = 3;

constexpr uint16_t status_none = 0;

constexpr uint16_t cosigning_in_progress = 4;
constexpr uint16_t cosigning_failure = 5;
constexpr uint16_t cosigning_success = 6;

constexpr uint16_t voting_in_progress = 8;
constexpr uint16_t voting_failure = 9;
constexpr uint16_t voting_success = 10;

struct proposal_info {
    int32_t serialize(base::xstream_t & stream) const;
    int32_t deserialize(base::xstream_t & stream);

    std::string proposal_id;
	std::string parameter;
	std::string new_value;
	std::string modification_description;
    std::string proposal_client_address;
    proposal_type type;
    uint64_t deposit;
    uint64_t effective_timer_height;
    uint8_t  priority;
    uint16_t cosigning_status;
    uint16_t voting_status;
    uint64_t end_time;
};


NS_END2
