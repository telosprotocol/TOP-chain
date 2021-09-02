#include "xcontract_runtime/xsystem_contract_manager.h"

#include "xbasic/xutility.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xtransaction.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"
// #include "xsystem_contracts/xtransfer_contract.h"

NS_BEG2(top, contract_runtime)

void xtop_system_contract_manager::initialize(base::xvblockstore_t* blockstore) {
    m_blockstore = blockstore;
}

void xtop_system_contract_manager::deploy() {
    // m_system_contract_deployment_data = {
    //     // {common::xaccount_address_t{system_contracts::transfer_address}, xcontract_deployment_data_t{std::make_shared<system_contracts::xtransfer_contract_t>(), {}}}
    // };
    // deploy one system contract
    deploy_system_contract(common::xaccount_address_t{"address"}, xblock_sniff_config_t{}, contract_deploy_type_t::zec, "zec", contract_broadcast_policy_t::normal);
}

void xtop_system_contract_manager::deploy_system_contract(common::xaccount_address_t const& address,
                                                        xblock_sniff_config_t sniff_config,
                                                        contract_deploy_type_t deploy_type,
                                                        std::string const& broadcast_target,
                                                        contract_broadcast_policy_t broadcast_policy) {
    // must system contract & not deploy yet
    assert(data::is_sys_contract_address(address));
    assert(!contains(address));

    xcontract_deployment_data_t data;
    data.m_deploy_type = deploy_type;
    data.m_broadcast_policy = broadcast_policy;
    data.m_sniff_config = sniff_config;

    std::vector<std::string> target_list;
    base::xstring_utl::split_string(broadcast_target, '|', target_list);

    for (auto const & target : target_list) {
        if (target == "rec") {
            data.m_broadcast_target |= common::xnode_type_t::rec;
        } else if (target == "zec") {
            data.m_broadcast_target |= common::xnode_type_t::zec;
        } else if (target == "arc") {
            data.m_broadcast_target |= common::xnode_type_t::storage;
        } else if (target == "all") {
             data.m_broadcast_target |= common::xnode_type_t::all;
        }
    }

    m_system_contract_deployment_data[address] = data;

    if (contract_deploy_type_t::table == deploy_type) {
        for ( auto i = 0; i < enum_vbucket_has_tables_count; i++) {
            init_system_contract(common::xaccount_address_t{address.value() + "@" + std::to_string(i)});
        }
    } else {
        init_system_contract(address);
    }

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

    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(contract_address.value(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);

    data::xtransaction_result_t result;
    base::xauto_ptr<base::xvblock_t> block(data::xblocktool_t::create_genesis_lightunit(contract_address.value(), tx, result));
    xassert(block);

    base::xvaccount_t _vaddr(block->get_account());
    // m_blockstore->delete_block(_vaddr, genesis_block.get());  // delete default genesis block
    auto ret = m_blockstore->store_block(_vaddr, block.get());
    if (!ret) {
        xerror("xtop_system_contract_manager::init_contract_chain %s genesis block fail", contract_address.c_str());
        return;
    }
    ret = m_blockstore->execute_block(_vaddr, block.get());
    if (!ret) {
        xerror("xtop_system_contract_manager::init_contract_chain execute genesis block fail");
        return;
    }
    xdbg("xtop_system_contract_manager::init_contract_chain contract_adress: %s, %s", contract_address.c_str(), ret ? "SUCC" : "FAIL");
}

NS_END2
