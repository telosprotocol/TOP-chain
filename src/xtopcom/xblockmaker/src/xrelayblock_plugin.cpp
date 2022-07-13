// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xcommon/xerror/xerror.h"
#include "xblockmaker/xrelayblock_plugin.h"
#include "xblockmaker/xerror/xerror.h"
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
    // contxs.push_back(tx1);
    data::xcons_transaction_ptr_t tx2 = make_relay_make_block_contract_tx(statectx_ptr, timestamp, ec);
    if (ec) {
        return {};
    }
    if (nullptr != tx2) {
        contxs.push_back(tx2);
    }
    return contxs;
}

data::xcons_transaction_ptr_t xrelayblock_plugin_t::make_relay_election_repackage_contract_tx(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp, std::error_code & ec) {
    base::xvaccount_t _vaddr(sys_contract_relay_repackage_election_addr);
    auto unitstate = statectx_ptr->load_unit_state(_vaddr);    
    if (nullptr == unitstate) {
        ec = blockmaker::error::xerrc_t::blockmaker_load_unitstate;
        xerror("xrelayblock_plugin_t::make_relay_election_repackage_contract_tx fail-load unitstate.%s", 
            _vaddr.get_account().c_str());
        return nullptr;
    }

    std::string func_name = "on_timer";
    data::xtransaction_ptr_t tx = data::xtx_factory::create_sys_contract_call_self_tx(unitstate->get_account(),
                                                     unitstate->account_send_trans_number(), unitstate->account_send_trans_hash(),
                                                      func_name, std::string(), timestamp, EXPIRE_DURATION); 
    data::xcons_transaction_ptr_t contx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    xdbg("xrelayblock_plugin_t::make_relay_election_repackage_contract_tx nonce=%ld", unitstate->account_send_trans_number());
    return contx;
}

data::xcons_transaction_ptr_t xrelayblock_plugin_t::make_relay_make_block_contract_tx(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp, std::error_code & ec) {
    if (m_last_phase == "2") {// TODO(jimmy)  use macro
        std::string func_name = "on_make_block";
        std::string call_params;  // empty call params now
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

xblock_resource_description_t xrelayblock_plugin_t::make_resource(std::error_code & ec) const {
    std::string after_prop_phase = m_relay_make_block_contract_state->string_get(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    if (after_prop_phase != m_last_phase && after_prop_phase == "2") {
        std::string prop_relayblock = m_relay_make_block_contract_state->string_get(data::system_contract::XPROPERTY_RELAY_BLOCK_STR);
        if (prop_relayblock.empty()) {
            ec = blockmaker::error::xerrc_t::blockmaker_property_invalid;
            xerror("xrelayblock_plugin_t::make_resource fail-invalid relayblock property.");
            return {};
        }
        xblock_resource_description_t resource_desc;
        resource_desc.resource_key_name = data::RESOURCE_RELAY_BLOCK;
        resource_desc.resource_value = prop_relayblock;
        resource_desc.is_output_resource = true;
        resource_desc.need_signature = true;
        xdbg_info("xrelayblock_plugin_t::make_resource succ.blocksize=%zu", prop_relayblock.size());
        return resource_desc;
    }
    xdbg("xrelayblock_plugin_t::make_resource no need. m_last_phase=%s,after_prop_phase=%s", 
        m_last_phase.c_str(),after_prop_phase.c_str());
    return {};
}


NS_END2
