#include "xcontract_runtime/xsystem_contract_manager.h"

#include "xbasic/xutility.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"
#include "xsystem_contracts/xelection/rec/xrec_standby_pool_contract.h"
// #include "xsystem_contracts/xtransfer_contract.h"

using namespace top::system_contracts;

NS_BEG2(top, contract_runtime)

#define DEPLOY_DATA(origin_addr, contract_ptr, sniff_addr, sniff_action, sniff_interval)                                                                                                \
    {common::xaccount_address_t{origin_addr},                                                                                                                                      \
    {contract_ptr, xblock_sniff_config_t{std::make_pair(common::xaccount_address_t{sniff_addr}, xblock_sniff_config_data_t{sniff_action, sniff_interval})}}}

void xtop_system_contract_manager::deploy() {
    m_system_contract_deployment_data = {
        DEPLOY_DATA(sys_contract_rec_standby_pool_addr,
               std::make_shared<xrec_standby_pool_contract_new_t>(),
               sys_contract_rec_standby_pool_addr,
               std::bind(&xtop_system_contract_manager::process_sniff, this, std::placeholders::_1),
               0),
    };
}

observer_ptr<xbasic_system_contract_t> xtop_system_contract_manager::system_contract(common::xaccount_address_t const & address) const noexcept {
    auto const it = m_system_contract_deployment_data.find(address);
    if (it != std::end(m_system_contract_deployment_data)) {
        return top::make_observer(top::get<xcontract_deployment_data_t>(*it).system_contract.get());
    }

    return nullptr;
}

bool xtop_system_contract_manager::process_sniff(base::xvblock_ptr_t block) {
    return true;
}

NS_END2
