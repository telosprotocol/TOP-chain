// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"
#include "xcontract_runtime/xblock_sniff_config.h"
#include "xdata/xgenesis_data.h"
#include "xsystem_contracts/xbasic_system_contract.h"
#include "xvledger/xvblockstore.h"

#include <unordered_map>
#include <vector>

NS_BEG3(top, contract_runtime, system)

enum class enum_contract_deploy_type: uint8_t {
    rec,
    zec,
    consensus,
    table
};
using contract_deploy_type_t = enum_contract_deploy_type;

enum class enum_contract_broadcast_policy: uint8_t {
    invalid,
    normal,
    fullunit
};
using contract_broadcast_policy_t = enum_contract_broadcast_policy;


class xtop_system_contract_manager {
    struct xtop_contract_deployment_data {
        std::shared_ptr<system_contracts::xbasic_system_contract_t> m_system_contract;
        xblock_sniff_config_t m_sniff_config;
        contract_deploy_type_t m_deploy_type;
        common::xnode_type_t m_broadcast_target{ common::xnode_type_t::invalid };
        contract_broadcast_policy_t m_broadcast_policy{ contract_broadcast_policy_t::invalid };
    };
    using xcontract_deployment_data_t = xtop_contract_deployment_data;

    std::unordered_map<common::xaccount_address_t, xcontract_deployment_data_t> m_system_contract_deployment_data;
    base::xvblockstore_t* m_blockstore;

public:
    xtop_system_contract_manager() = default;
    xtop_system_contract_manager(xtop_system_contract_manager const &) = delete;
    xtop_system_contract_manager & operator=(xtop_system_contract_manager const &) = delete;
    xtop_system_contract_manager(xtop_system_contract_manager &&) = default;
    xtop_system_contract_manager & operator=(xtop_system_contract_manager &&) = default;
    ~xtop_system_contract_manager() = default;


    /**
     * @brief get an instance
     *
     * @return xtop_system_contract_manager&
     */
    static xtop_system_contract_manager& instance();
    void initialize(base::xvblockstore_t* blockstore);
    void deploy();

    observer_ptr<system_contracts::xbasic_system_contract_t> system_contract(common::xaccount_address_t const & address) const noexcept;
    template<typename system_contract_type>
    void deploy_system_contract(common::xaccount_address_t const& address,
                                xblock_sniff_config_t sniff_config,
                                contract_deploy_type_t deploy_type,
                                common::xnode_type_t broadcast_target,
                                contract_broadcast_policy_t broadcast_policy);

private:
    void init_system_contract(common::xaccount_address_t const & contract_address);

    bool contains(common::xaccount_address_t const & address) const noexcept;

};
using xsystem_contract_manager_t = xtop_system_contract_manager;


template<typename system_contract_type>
void xtop_system_contract_manager::deploy_system_contract(common::xaccount_address_t const& address,
                                                            xblock_sniff_config_t sniff_config,
                                                            contract_deploy_type_t deploy_type,
                                                            common::xnode_type_t broadcast_target,
                                                            contract_broadcast_policy_t broadcast_policy = contract_broadcast_policy_t::invalid) {
    // must system contract & not deploy yet
    assert(data::is_sys_contract_address(address));
    assert(!contains(address));

    xcontract_deployment_data_t data;
    data.m_deploy_type = deploy_type;
    data.m_broadcast_policy = broadcast_policy;
    data.m_sniff_config = sniff_config;
    data.m_broadcast_target = broadcast_target;

    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_t>();
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(address.value(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()), top::state_accessor::xstate_access_control_data_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(common::xaccount_address_t{address}, top::make_observer(property_access_control.get()));
    auto contract_ctx= std::make_shared<contract_common::xcontract_execution_context_t>(tx, contract_state);
    data.m_system_contract = std::make_shared<system_contract_type>(top::make_observer(contract_ctx.get()));

    m_system_contract_deployment_data[address] = data;

    if (contract_deploy_type_t::table == deploy_type) {
        for ( auto i = 0; i < enum_vbucket_has_tables_count; i++) {
            init_system_contract(common::xaccount_address_t{address.value() + "@" + std::to_string(i)});
        }
    } else {
        init_system_contract(address);
    }

}

NS_END3

