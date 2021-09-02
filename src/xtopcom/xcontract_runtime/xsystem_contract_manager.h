#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_runtime/xblock_sniff_config.h"
#include "xsystem_contracts/xbasic_system_contract.h"

#include <unordered_map>
#include <vector>

NS_BEG2(top, contract_runtime)

// struct xtop_contract_deployment_data {
//     std::shared_ptr<system_contracts::xbasic_system_contract_t> system_contract;
//     xblock_sniff_config_t sniff_config;
//     xtop_contract_deployment_data(std::shared_ptr<system_contracts::xbasic_system_contract_t> contract, xblock_sniff_config_t config)
//         : system_contract(contract), sniff_config(std::move(config)) {
//     }
// };
// using xcontract_deployment_data_t = xtop_contract_deployment_data;

class xtop_system_contract_manager {
    struct xtop_contract_deployment_data {
        std::shared_ptr<system_contracts::xbasic_system_contract_t> system_contract;
        xblock_sniff_config_t sniff_config;
        xtop_contract_deployment_data(std::shared_ptr<system_contracts::xbasic_system_contract_t> contract, xblock_sniff_config_t config)
            : system_contract(contract), sniff_config(std::move(config)) {
        }
    };
    using xcontract_deployment_data_t = xtop_contract_deployment_data;
    std::unordered_map<common::xaccount_address_t, xcontract_deployment_data_t> m_system_contract_deployment_data;

public:
    xtop_system_contract_manager() = default;
    xtop_system_contract_manager(xtop_system_contract_manager const &) = delete;
    xtop_system_contract_manager & operator=(xtop_system_contract_manager const &) = delete;
    xtop_system_contract_manager(xtop_system_contract_manager &&) = default;
    xtop_system_contract_manager & operator=(xtop_system_contract_manager &&) = default;
    ~xtop_system_contract_manager() = default;

    void deploy();

    observer_ptr<system_contracts::xbasic_system_contract_t> system_contract(common::xaccount_address_t const & address) const noexcept;
    bool process_sniff(base::xvblock_ptr_t block);
};
using xsystem_contract_manager_t = xtop_system_contract_manager;

NS_END2
