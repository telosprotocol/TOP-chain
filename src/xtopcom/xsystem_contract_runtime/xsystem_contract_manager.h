// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_runtime/xblock_sniff_config.h"
#include "xdata/xgenesis_data.h"
#include "xsystem_contracts/xbasic_system_contract.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvcnode.h"

#include <unordered_map>
#include <vector>

NS_BEG3(top, contract_runtime, system)

struct xtop_contract_deployment_data {
    std::shared_ptr<system_contracts::xbasic_system_contract_t> system_contract{nullptr};
    common::xnode_type_t node_type{common::xnode_type_t::invalid};
    xsniff_type_t sniff_type{xsniff_type_t::invalid};
    xsniff_broadcast_config_t broadcast_config{};
    xsniff_timer_config_t timer_config{};
    xsniff_block_config_t block_config{};

    xtop_contract_deployment_data() = default;
    xtop_contract_deployment_data(std::shared_ptr<system_contracts::xbasic_system_contract_t> contract_,
                                  common::xnode_type_t node_type_,
                                  xsniff_type_t sniff_type_,
                                  xsniff_broadcast_config_t broadcast_config_,
                                  xsniff_timer_config_t timer_config_,
                                  xsniff_block_config_t block_config_)
      : system_contract(contract_), node_type(node_type_), sniff_type(sniff_type_), broadcast_config(broadcast_config_), timer_config(timer_config_), block_config(block_config_) {
    }
};
using xcontract_deployment_data_t = xtop_contract_deployment_data;

class xtop_system_contract_manager {
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
        static xtop_system_contract_manager * inst = new xtop_system_contract_manager();
        return inst;
    };
    void deploy(observer_ptr<base::xvblockstore_t> const & blockstore);
    std::unordered_map<common::xaccount_address_t, xcontract_deployment_data_t> const & deployment_data() const noexcept;
    observer_ptr<system_contracts::xbasic_system_contract_t> system_contract(common::xaccount_address_t const & address) const noexcept;

private:
    template <typename system_contract_type>
    void deploy_system_contract(common::xaccount_address_t const & address,
                                common::xnode_type_t node_type,
                                xsniff_type_t sniff_type,
                                xsniff_broadcast_config_t broadcast_config,
                                xsniff_timer_config_t timer_config,
                                xsniff_block_config_t block_config);

    void init_system_contract(common::xaccount_address_t const & contract_address);

    bool contains(common::xaccount_address_t const & address) const noexcept;

    std::unordered_map<common::xaccount_address_t, xcontract_deployment_data_t> m_system_contract_deployment_data;
    observer_ptr<base::xvblockstore_t> m_blockstore{nullptr};
};
using xsystem_contract_manager_t = xtop_system_contract_manager;

template <typename system_contract_type>
void xtop_system_contract_manager::deploy_system_contract(common::xaccount_address_t const & address,
                                                          common::xnode_type_t node_type,
                                                          xsniff_type_t sniff_type,
                                                          xsniff_broadcast_config_t broadcast_config,
                                                          xsniff_timer_config_t timer_config,
                                                          xsniff_block_config_t block_config) {
    // must system contract & not deploy yet
    assert(data::is_sys_contract_address(address));
    assert(!contains(address));

    xcontract_deployment_data_t data{std::make_shared<system_contract_type>(), node_type, sniff_type, broadcast_config, timer_config, block_config};
    auto ret = m_system_contract_deployment_data.insert(std::make_pair(address, data));
    assert(ret.second == true);

    if (data::is_sys_sharding_contract_address(address)) {
        for ( auto i = 0; i < enum_vbucket_has_tables_count; i++) {
            init_system_contract(common::xaccount_address_t{address.value() + "@" + std::to_string(i)});
        }
    } else {
        init_system_contract(address);
    }

}

NS_END3

