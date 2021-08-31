#include "xcontract_runtime/xsystem_contract_manager.h"

#include "xbasic/xutility.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"
#include "xsystem_contracts/xtransfer_contract.h"

NS_BEG2(top, contract_runtime)

xtop_system_contract_manager& xtop_system_contract_manager::instance() {
    static xtop_system_contract_manager* manager;
    if (nullptr == manager) {
        manager = new xtop_system_contract_manager();
    }

    return *manager;

}

void xtop_system_contract_manager::deploy() {
    // m_system_contract_deployment_data = {
    //     {common::xaccount_address_t{system_contracts::transfer_address}, xcontract_deployment_data_t{std::make_shared<system_contracts::xtransfer_contract_t>(), {}}}
    // };
}

observer_ptr<system_contracts::xbasic_system_contract_t> xtop_system_contract_manager::system_contract(common::xaccount_address_t const & address) const noexcept {
    auto const it = m_system_contract_deployment_data.find(address);
    if (it != std::end(m_system_contract_deployment_data)) {
        return top::make_observer(top::get<xcontract_deployment_data_t>(*it).system_contract.get());
    }

    return nullptr;
}

NS_END2
