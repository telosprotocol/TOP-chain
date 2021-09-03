// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xsystem_contract_manager.h"

#include "xbasic/xutility.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xtransaction.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract_new.h"

#include "xsystem_contracts/xtransfer_contract.h"


NS_BEG2(top, contract_runtime)

void xtop_system_contract_manager::initialize(base::xvblockstore_t* blockstore) {
    m_blockstore = blockstore;
}

void xtop_system_contract_manager::deploy() {
    // deploy one system contract
    deploy_system_contract<system_contracts::xtop_transfer_contract>(common::xaccount_address_t{"address"}, xblock_sniff_config_t{}, contract_deploy_type_t::zec, common::xnode_type_t::zec, contract_broadcast_policy_t::normal);
}

bool xtop_system_contract_manager::contains(common::xaccount_address_t const & address) const noexcept {
    return m_system_contract_deployment_data.find(address) != std::end(m_system_contract_deployment_data);

}

observer_ptr<system_contracts::xbasic_system_contract_t> xtop_system_contract_manager::system_contract(common::xaccount_address_t const & address) const noexcept {
    auto const it = m_system_contract_deployment_data.find(address);
    if (it != std::end(m_system_contract_deployment_data)) {
        return top::make_observer(top::get<xcontract_deployment_data_t>(*it).m_system_contract.get());
    }

    return nullptr;
}

void xtop_system_contract_manager::init_system_contract(common::xaccount_address_t const & contract_address) {

    if (m_blockstore->exist_genesis_block(contract_address.value())) {
        xdbg("xtop_system_contract_manager::init_contract_chain contract account %s genesis block exist", contract_address.c_str());
        return;
    }
    xdbg("xtop_system_contract_manager::init_contract_chain contract account %s genesis block not exist", contract_address.c_str());


    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_t>();
    data::xtransaction_result_t result;
    result.m_property_binlog = system_contract(contract_address)->state()->binlog();
    result.m_full_state = system_contract(contract_address)->state()->fullstate_bin();
    base::xauto_ptr<base::xvblock_t> block(data::xblocktool_t::create_genesis_lightunit(contract_address.value(), tx, result));
    xassert(block);

    base::xvaccount_t _vaddr(block->get_account());
    auto ret = m_blockstore->store_block(_vaddr, block.get());
    if (!ret) {
        xerror("xtop_system_contract_manager::init_contract_chain %s genesis block fail", contract_address.c_str());
        return;
    }
    xdbg("xtop_system_contract_manager::init_contract_chain contract_adress: %s, %s", contract_address.c_str(), ret ? "SUCC" : "FAIL");
}

NS_END2
