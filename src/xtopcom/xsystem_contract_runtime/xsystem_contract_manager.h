// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xutility.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_runtime/xblock_sniff_config.h"
#include "xdata/xgenesis_data.h"
#include "xsystem_contracts/xbasic_system_contract.h"
#include "xvledger/xvblockstore.h"

#include <unordered_map>

NS_BEG3(top, contract_runtime, system)

struct xtop_contract_deployment_data {
    // std::shared_ptr<system_contracts::xbasic_system_contract_t> system_contract{nullptr};
    common::xnode_type_t node_type{common::xnode_type_t::invalid};
    xsniff_type_t sniff_type{xsniff_type_t::none};
    xsniff_broadcast_config_t broadcast_config{};
    xsniff_timer_config_t timer_config{};
    xsniff_block_config_t block_config{};

    xtop_contract_deployment_data() = default;
    xtop_contract_deployment_data(xtop_contract_deployment_data const &) = default;
    xtop_contract_deployment_data & operator=(xtop_contract_deployment_data const &) = default;
    xtop_contract_deployment_data(xtop_contract_deployment_data &&) = default;
    xtop_contract_deployment_data & operator=(xtop_contract_deployment_data &&) = default;
    ~xtop_contract_deployment_data() = default;

    xtop_contract_deployment_data(common::xnode_type_t node_type,
                                  xsniff_type_t sniff_type,
                                  xsniff_broadcast_config_t broadcast_config,
                                  xsniff_timer_config_t timer_config,
                                  xsniff_block_config_t block_config);
};
using xcontract_deployment_data_t = xtop_contract_deployment_data;

struct xtop_system_contract_data {
    std::unique_ptr<system_contracts::xbasic_system_contract_t> system_contract;
    xcontract_deployment_data_t deployment_data;
};
using xsystem_contract_data = xtop_system_contract_data;

class xtop_system_contract_manager {
private:
    std::unordered_map<common::xaccount_address_t, xcontract_deployment_data_t> m_system_contract_deployment_data;
    std::unordered_map<common::xaccount_address_t, std::unique_ptr<system_contracts::xbasic_system_contract_t>> m_system_contracts;

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
    static xtop_system_contract_manager * instance() {
        static auto * inst = new xtop_system_contract_manager();
        return inst;
    }

    void deploy(observer_ptr<base::xvblockstore_t> const & blockstore);
    std::unordered_map<common::xaccount_address_t, xcontract_deployment_data_t> const & deployment_data() const noexcept;
    observer_ptr<contract_common::xbasic_contract_t> system_contract(common::xaccount_address_t const & address) const noexcept;

private:
    template <typename system_contract_type>
    void deploy_system_contract(common::xaccount_address_t const & address,
                                common::xnode_type_t node_type,
                                xsniff_type_t sniff_type,
                                xsniff_broadcast_config_t broadcast_config,
                                xsniff_timer_config_t timer_config,
                                xsniff_block_config_t block_config,
                                observer_ptr<base::xvblockstore_t> const & blockstore);

    void init_system_contract(common::xaccount_address_t const & contract_address, observer_ptr<base::xvblockstore_t> const & blockstore);

    bool contains(common::xaccount_address_t const & address) const noexcept;
};
using xsystem_contract_manager_t = xtop_system_contract_manager;

template <typename system_contract_type>
void xtop_system_contract_manager::deploy_system_contract(common::xaccount_address_t const & address,
                                                          common::xnode_type_t node_type,
                                                          xsniff_type_t sniff_type,
                                                          xsniff_broadcast_config_t broadcast_config,
                                                          xsniff_timer_config_t timer_config,
                                                          xsniff_block_config_t block_config,
                                                          observer_ptr<base::xvblockstore_t> const & blockstore) {
    // must system contract & not deploy yet
    assert(data::is_sys_contract_address(address));
    assert(!contains(address));

    auto sys_contract = top::make_unique<system_contract_type>();
    xcontract_deployment_data_t data{node_type, sniff_type, broadcast_config, std::move(timer_config), std::move(block_config)};
#if !defined(NDEBUG)
    auto r1 = 
#endif
    m_system_contract_deployment_data.insert(std::make_pair(address, std::move(data)));
    assert(top::get<bool>(r1));

#if !defined(NDEBUG)
    auto r2 =
#endif
    m_system_contracts.emplace(address, std::move(sys_contract));
    assert(top::get<bool>(r2));


    if (data::is_sys_sharding_contract_address(address)) {
        for ( auto i = 0; i < enum_vbucket_has_tables_count; i++) {
            init_system_contract(common::xaccount_address_t{address.value() + "@" + std::to_string(i)}, blockstore);
        }
    } else {
        init_system_contract(address, blockstore);
    }
}

NS_END3
