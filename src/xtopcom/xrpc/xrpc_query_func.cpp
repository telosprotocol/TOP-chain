#include "xrpc/xrpc_query_func.h"

#include "xbase/xbase.h"
#include "xbase/xcontext.h"
#include "xbase/xint.h"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xblocktool.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_network_result_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xtableblock.h"
#include "xdata/xtransaction_cache.h"
#include "xevm_common/common.h"
#include "xmbus/xevent_behind.h"
#include "xrouter/xrouter.h"
#include "xrpc/xrpc_loader.h"
#include "xrpc/xuint_format.h"
#include "xstore/xaccount_context.h"
#include "xstore/xtgas_singleton.h"
#include "xtxexecutor/xtransaction_fee.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvm/manager/xcontract_manager.h"

#include <cstdint>
#include <iostream>

NS_BEG2(top, xrpc)

bool xrpc_query_func::is_prop_name_already_set_property(const std::string & prop_name) {
    static std::set<std::string> property_names = {
        data::XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY,
        data::XPROPERTY_CONTRACT_STANDBYS_KEY,
        data::XPROPERTY_CONTRACT_GROUP_ASSOC_KEY,
    };

    auto iter = property_names.find(prop_name);
    if (iter != property_names.end()) {
        return true;
    }
    if (prop_name.size() > 3 && data::XPROPERTY_CONTRACT_ELECTION_RESULT_KEY == prop_name.substr(0, 3)) {
        return true;
    }
    return false;
}

bool xrpc_query_func::is_prop_name_not_set_property(const std::string & prop_name) {
    static std::set<std::string> property_names = {data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY,
                                                   data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY,
                                                   data::system_contract::XPORPERTY_CONTRACT_REG_KEY,
                                                   data::system_contract::XPORPERTY_CONTRACT_TICKETS_KEY,
                                                   data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY,
                                                   data::system_contract::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY,
                                                   data::system_contract::XPORPERTY_CONTRACT_TASK_KEY,
                                                   data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY1,
                                                   data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY2,
                                                   data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY3,
                                                   data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY4,
                                                   data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1,
                                                   data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2,
                                                   data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3,
                                                   data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4,
                                                   data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY,
                                                   data::system_contract::XPORPERTY_CONTRACT_REFUND_KEY,
                                                   data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE,
                                                   data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY,
                                                   data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY,
                                                   data::system_contract::XPROPERTY_CONTRACT_SLASH_INFO_KEY,
                                                   data::system_contract::XPROPERTY_REWARD_DETAIL,
                                                   data::system_contract::XPROPERTY_LAST_HASH,
                                                   data::system_contract::XPROPERTY_EFFECTIVE_HASHES,
                                                   data::system_contract::XPROPERTY_ALL_HASHES,
                                                   data::system_contract::XPROPERTY_HEADERS,
                                                   data::system_contract::XPROPERTY_HEADERS_SUMMARY,
                                                   PROPOSAL_MAP_ID,
                                                   VOTE_MAP_ID};

    auto iter = property_names.find(prop_name);
    if (iter != property_names.end()) {
        return true;
    }
    return false;
}

bool xrpc_query_func::query_special_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name, data::xaccount_ptr_t unitstate, bool compatible_mode) {
    if (is_prop_name_already_set_property(prop_name)) {
        top::contract::xcontract_manager_t::instance().get_contract_data(
            top::common::xaccount_address_t{owner}, unitstate, prop_name, top::contract::xjson_format_t::detail, compatible_mode, jph);
        return true;
    } else if (is_prop_name_not_set_property(prop_name)) {
        xJson::Value jm;
        top::contract::xcontract_manager_t::instance().get_contract_data(
            top::common::xaccount_address_t{owner}, unitstate, prop_name, top::contract::xjson_format_t::detail, compatible_mode, jm);
        jph[prop_name] = jm;
        return true;
    }

    if (data::XPROPERTY_PLEDGE_VOTE_KEY == prop_name) {
        base::xvproperty_t * propobj = unitstate->get_bstate()->get_property_object(prop_name);
        base::xmapvar_t<std::string> * var_obj = dynamic_cast<base::xmapvar_t<std::string> *>(propobj);
        std::map<std::string, std::string> pledge_votes = var_obj->query();
        for (auto & v : pledge_votes) {
            uint64_t vote_num{0};
            uint16_t duration{0};
            uint64_t lock_time{0};

            store::xaccount_context_t::deserilize_vote_map_field(v.first, duration, lock_time);
            store::xaccount_context_t::deserilize_vote_map_value(v.second, vote_num);
            xdbg("pledge_redeem_vote %d, %d, %d", vote_num, duration, lock_time);
            if (vote_num == 0)
                continue;
            xJson::Value j;
            j["vote_num"] = static_cast<unsigned long long>(vote_num);
            if (duration != 0)
                j["lock_token"] = static_cast<unsigned long long>(store::xaccount_context_t::get_top_by_vote(vote_num, duration));
            else {
                auto propobj_str = unitstate->get_bstate()->load_string_var(data::XPROPERTY_EXPIRE_VOTE_TOKEN_KEY);
                j["lock_token"] = propobj_str->query();
            }
            j["duration"] = duration;
            j["lock_time"] = static_cast<unsigned long long>(lock_time);
            jph[data::XPROPERTY_PLEDGE_VOTE_KEY].append(j);
        }
        return true;
    }

    if (prop_name == data::XPROPERTY_TEP1_BALANCE_KEY)
    {
        xJson::Value j;
        auto kvs = unitstate->map_get(prop_name);
        for (auto & v : kvs) {
            auto const token_id = top::from_string<common::xtoken_id_t>(v.first);
            auto token_balance = unitstate->tep_token_balance(token_id);
            j[common::symbol(token_id).to_string()] = token_balance.str();
        }
        jph[prop_name] = j;
        return true;
    }

    if (prop_name == data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY) {
        xJson::Value j;
        std::error_code ec;
        auto const allowance_data = unitstate->allowance(ec);
        for (auto const & allowance_datum : allowance_data) {
            auto const token_id = top::get<common::xtoken_id_t const>(allowance_datum);
            auto const & allowance = top::get<data::system_contract::xallowance_t>(allowance_datum);

            auto const & symbol = common::symbol(token_id, ec);
            if (ec) {
                xwarn("invalid token id %d", static_cast<int>(token_id));
                continue;
            }

            xJson::Value spenderJson;
            for (auto const & a : allowance) {
                auto const & spender = top::get<common::xaccount_address_t const>(a);
                auto const & value = top::get<evm_common::u256>(a);

                xdbg("found allowance: symbol %s spender %s value %s", symbol.c_str(), spender.c_str(), value.str().c_str());

                spenderJson[spender.value()] = value.str();
            }

            j[symbol.to_string()] = spenderJson;
        }
        jph[prop_name] = j;
        return true;
    }
    return false;
}

void xrpc_query_func::query_account_property_base(xJson::Value & jph, const std::string & owner, const std::string & prop_name, data::xaccount_ptr_t unitstate, bool compatible_mode) {
    if (unitstate == nullptr) {
        xwarn("xrpc_query_manager::query_account_property fail-query unit state.account=%s", owner.c_str());
        return;
    }
    if (false == unitstate->get_bstate()->find_property(prop_name)) {
        xwarn("xrpc_query_manager::query_account_property fail-find property.account=%s,prop_name=%s", owner.c_str(), prop_name.c_str());
        return;
    }
    if (true == query_special_property(jph, owner, prop_name, unitstate, compatible_mode)) {
        return;
    }

    base::xvproperty_t * propobj = unitstate->get_bstate()->get_property_object(prop_name);
    if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string_map) {
        auto propobj_map = unitstate->get_bstate()->load_string_map_var(prop_name);
        auto values = propobj_map->query();
        xJson::Value j;
        for (auto & v : values) {
            j[v.first] = v.second;
        }
        jph[prop_name] = j;
    } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string) {
        auto propobj_str = unitstate->get_bstate()->load_string_var(prop_name);
        jph[prop_name] = propobj_str->query();
    } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string_deque) {
        auto propobj_deque = unitstate->get_bstate()->load_string_deque_var(prop_name);
        auto values = propobj_deque->query();
        for (auto & v : values) {
            jph[prop_name].append(v);
        }
    } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_token) {
        auto propobj = unitstate->get_bstate()->load_token_var(prop_name);
        base::vtoken_t balance = propobj->get_balance();
        jph[prop_name] = std::to_string(balance);
    } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_uint64) {
        auto propobj = unitstate->get_bstate()->load_uint64_var(prop_name);
        uint64_t value = propobj->get();
        jph[prop_name] = std::to_string(value);
    }
}

void xrpc_query_func::query_account_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name, xfull_node_compatible_mode_t compatible_mode) {
    xdbg("xrpc_query_manager::query_account_property account=%s,prop_name=%s", owner.c_str(), prop_name.c_str());
    // load newest account state
    if (m_store == nullptr)
        return;
    data::xaccount_ptr_t unitstate = m_store->query_account(owner);
    query_account_property_base(jph, owner, prop_name, unitstate, compatible_mode == xfull_node_compatible_mode_t::compatible);
}

void xrpc_query_func::query_account_property(xJson::Value & jph,
                                              const std::string & owner,
                                              const std::string & prop_name,
                                              const uint64_t height,
                                              xfull_node_compatible_mode_t compatible_mode) {
    xdbg("xrpc_query_manager::query_account_property account=%s,prop_name=%s,height=%llu", owner.c_str(), prop_name.c_str(), height);
    // load newest account state
    base::xvaccount_t _vaddr(owner);
    auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddr, height, 0, false, metrics::blockstore_access_from_rpc_get_block_query_propery);
    if (_block == nullptr) {
        xdbg("xrpc_query_manager::query_account_property block %s, height %llu, not exist", owner.c_str(), height);
        return;
    }

    if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
        xdbg("xrpc_query_manager::query_account_property %s, height %llu, genesis or nil block", owner.c_str(), height);
        return;
    }

    base::xauto_ptr<base::xvbstate_t> bstate =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get(), metrics::statestore_access_from_rpc_query_propery);
    data::xaccount_ptr_t unitstate = nullptr;
    if (bstate != nullptr) {
        unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
    }

    query_account_property_base(jph, owner, prop_name, unitstate, compatible_mode == xfull_node_compatible_mode_t::compatible);
}

NS_END2