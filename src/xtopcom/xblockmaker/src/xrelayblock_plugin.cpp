// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xhex.h"
#include "xcommon/xerror/xerror.h"
#include "xblockmaker/xrelayblock_plugin.h"
#include "xblockmaker/xerror/xerror.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xblocktool.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xtx_factory.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockextract.h"
#include "xdata/xblockbuild.h"
#include "xvledger/xvledger.h"
#include "xdata/xsystem_contract/xdata_structures.h"

NS_BEG2(top, blockmaker)

const uint16_t EXPIRE_DURATION = 300;

void xrelayblock_plugin_t::init(statectx::xstatectx_ptr_t const& statectx_ptr, std::error_code & ec) {
    base::xvaccount_t _vaddr(sys_contract_relay_make_block_addr);
    m_relay_make_block_contract_state = statectx_ptr->load_unit_state(_vaddr);    
    if (nullptr == m_relay_make_block_contract_state) {
        ec = blockmaker::error::xerrc_t::blockmaker_load_unitstate;
        xerror("xrelayblock_plugin_t::init fail-load unitstate.%s", _vaddr.get_account().c_str());
        return;
    }
    m_last_phase = m_relay_make_block_contract_state->string_get(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    if (m_last_phase.empty()) {
        ec = blockmaker::error::xerrc_t::blockmaker_property_invalid;
        xerror("xrelayblock_plugin_t::init fail-invalid property.%s", 
            _vaddr.get_account().c_str());
        return;
    }
    xdbg("xrelayblock_plugin_t::init prop_phase=%s,accountnonce=%ld", 
        m_last_phase.c_str(),m_relay_make_block_contract_state->account_send_trans_number());
}

std::vector<data::xcons_transaction_ptr_t>  xrelayblock_plugin_t::make_contract_txs(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp, std::error_code & ec) {
    std::vector<data::xcons_transaction_ptr_t> contxs;
    // data::xcons_transaction_ptr_t tx1 = make_relay_election_repackage_contract_tx(statectx_ptr, timestamp, ec);
    // if (ec) {
    //     return {};
    // }
    // if (tx1 != nullptr) {
    //     contxs.push_back(tx1);
    // }

    data::xcons_transaction_ptr_t tx2 = make_relay_make_block_contract_tx(statectx_ptr, timestamp, ec);
    if (ec) {
        return {};
    }
    if (nullptr != tx2) {
        contxs.push_back(tx2);
        return contxs;
    }
    return {};
}

data::xcons_transaction_ptr_t xrelayblock_plugin_t::make_relay_make_block_contract_tx(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp, std::error_code & ec) {
    if (true) {// TODO(jimmy) always call on_make_block method  use macro  m_last_phase == "2"
        std::string func_name = "on_make_block";
        std::string call_params = get_new_relay_election_data(statectx_ptr, timestamp);
        base::xstream_t stream(base::xcontext_t::instance());
        stream << call_params;
        data::xtransaction_ptr_t tx = data::xtx_factory::create_sys_contract_call_self_tx(m_relay_make_block_contract_state->get_account(),
                                                        m_relay_make_block_contract_state->account_send_trans_number(), m_relay_make_block_contract_state->account_send_trans_hash(),
                                                        func_name, std::string((char *)stream.data(), stream.size()), timestamp, EXPIRE_DURATION); 
        data::xcons_transaction_ptr_t contx = make_object_ptr<data::xcons_transaction_t>(tx.get());
        xdbg_info("xrelayblock_plugin_t::make_relay_make_block_contract_tx tx=%s", contx->dump().c_str());
        return contx;
    } else {
        return nullptr;
    }
}

std::string xrelayblock_plugin_t::get_new_relay_election_data(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp) const {
    auto const relay_election_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(relay_election_interval);
    auto clock = timestamp/10;
    if (!m_time_to_check_election && (clock == 0 || (clock % (relay_election_interval / 4) != 0))){
        return {};
    }

    m_time_to_check_election = true;

    if (m_last_phase != "2") {
        return {};
    }

    m_time_to_check_election = false; 

    // re-package the relay shard election data into the relay chain:
    //  1. get the relay shard election data
    //  2. read the re-package contract data
    //  3. compare the read relay shard election data epoch and the contract data epoch
    //  4. epoch of relay election data is newer, generate transaction calling the re-package contract

    xobject_ptr_t<base::xvblock_t> zec_elect_relay_blk =
        data::xblocktool_t::get_latest_connectted_state_changed_block(base::xvchain_t::instance().get_xblockstore(), zec_elect_relay_contract_address.vaccount());
    // zec_elect_relay_blk->get_parent_block_height()
    xobject_ptr_t<base::xvblock_t> relay_make_block_blk =
        data::xblocktool_t::get_latest_connectted_state_changed_block(base::xvchain_t::instance().get_xblockstore(), relay_make_block_contract_address.vaccount());

    xobject_ptr_t<base::xvbstate_t> const zec_elect_relay_state =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(zec_elect_relay_blk.get(), metrics::statestore_access_from_contract_framework);
    xobject_ptr_t<base::xvbstate_t> const relay_make_block_state =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(relay_make_block_blk.get(), metrics::statestore_access_from_contract_framework);

    if (nullptr == zec_elect_relay_state || nullptr == relay_make_block_state) {
        xwarn("role_context fail to load relay election contract state");
        return {};
    }

    data::xunit_bstate_t const zec_elect_relay_unit_state{zec_elect_relay_state.get()};
    data::xunit_bstate_t const relay_elect_relay_unit_state{relay_make_block_state.get()};

    auto const & property_names = data::election::get_property_name_by_addr(zec_elect_relay_contract_address);
    if (property_names.size() != 1) {
        xerror("role_context: zec_elect_relay_contract property count unexpected");
        return {};
    }

    auto const & property_name = property_names.front();
    std::string result;
    zec_elect_relay_unit_state.string_get(property_name, result);
    if (result.empty()) {
        xerror("role_context: relay election data empty. property=%s,block=%s", property_name.c_str(), zec_elect_relay_blk->dump().c_str());
        return {};
    }

    xdbg("role_context: process relay election data %s, %" PRIu64 " from clock: %" PRIu64 " %" PRIu64,
         zec_elect_relay_contract_address.c_str(),
         zec_elect_relay_blk->get_height(),
         clock,
         timestamp);

    using top::data::election::xelection_result_store_t;

    std::string current_result;
    relay_elect_relay_unit_state.string_get(property_name, current_result);
    if (current_result.empty()) {
        // even genesis is not empty.
        xerror("role_context: local packaged relay election data empty. property=%s,block=%s", property_name.c_str(), relay_make_block_blk->dump().c_str());
        return {};
    }
    auto const & current_election_result_store = codec::msgpack_decode<xelection_result_store_t>({std::begin(current_result), std::end(current_result)});

    auto const & coming_election_result_store = codec::msgpack_decode<xelection_result_store_t>({std::begin(result), std::end(result)});

    auto const relay_group_address = common::build_relay_group_address(common::network_id());
    auto const & coming_group_result = coming_election_result_store.result_of(relay_group_address.network_id())
                                            .result_of(common::xnode_type_t::relay)
                                            .result_of(relay_group_address.cluster_id())
                                            .result_of(relay_group_address.group_id());

    auto const & current_group_result = current_election_result_store.result_of(relay_group_address.network_id())
                                            .result_of(common::xnode_type_t::relay)
                                            .result_of(relay_group_address.cluster_id())
                                            .result_of(relay_group_address.group_id());

    if (coming_group_result.group_epoch() <= current_group_result.group_epoch()) {
        xwarn("coming epoch is older, no need to update, %" PRIu64 " <= % " PRIu64, coming_group_result.group_epoch().value(), current_group_result.group_epoch().value());
        return {};
    }

    return result;
}

xblock_resource_description_t xrelayblock_plugin_t::make_resource(const data::xblock_consensus_para_t & cs_para, std::error_code & ec) const {
    std::string after_prop_phase = m_relay_make_block_contract_state->string_get(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    if (after_prop_phase != m_last_phase && after_prop_phase == "2") {
        std::string prop_relayblock = m_relay_make_block_contract_state->string_get(data::system_contract::XPROPERTY_RELAY_BLOCK_STR);
        if (prop_relayblock.empty()) {
            ec = blockmaker::error::xerrc_t::blockmaker_property_invalid;
            xerror("xrelayblock_plugin_t::make_resource fail-relayblock property empty.");
            return {};
        }

        data::xrelay_block relay_block;
        relay_block.decodeBytes(top::to_bytes(prop_relayblock), ec);
        if (ec) {
            xerror("xrelayblock_plugin_t::make_resource fail-invalid relayblock property.");
            return {};
        }
        relay_block.set_epochid(cs_para.get_election_round());
        relay_block.set_viewid(cs_para.get_viewid());
        uint256_t hash256 = from_bytes<uint256_t>(relay_block.build_signature_hash().to_bytes());

        xblock_resource_description_t resource_desc;
        resource_desc.resource_key_name = data::RESOURCE_RELAY_BLOCK;
        resource_desc.resource_value = prop_relayblock;
        resource_desc.is_input_resource = false;  // TODO(jimmy) put relayblock to output resource
        resource_desc.need_signature = true;
        resource_desc.signature_hash = hash256;
        xdbg_info("xrelayblock_plugin_t::make_resource succ.block=%s,size=%zu,sighash=%s", 
            relay_block.dump().c_str(),prop_relayblock.size(),top::to_hex(top::to_bytes(hash256)).c_str());
        return resource_desc;
    }
    xdbg("xrelayblock_plugin_t::make_resource no need. m_last_phase=%s,after_prop_phase=%s", 
        m_last_phase.c_str(),after_prop_phase.c_str());
    return {};
}


NS_END2
