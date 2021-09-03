#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_runtime/xblock_sniff_config.h"
#include "xdata/xgenesis_data.h"
#include "xsystem_contracts/xbasic_system_contract.h"
#include "xvledger/xvblockstore.h"

#include <unordered_map>
#include <vector>

NS_BEG2(top, contract_runtime)

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
        common::xnode_type_t m_broadcast_target{common::xnode_type_t::invalid};
        contract_broadcast_policy_t m_broadcast_policy{contract_broadcast_policy_t::invalid};

        xtop_contract_deployment_data(std::shared_ptr<system_contracts::xbasic_system_contract_t> contract, xblock_sniff_config_t config)
          : m_system_contract(contract), m_sniff_config(std::move(config)) {
        }
    };
    using xcontract_deployment_data_t = xtop_contract_deployment_data;

    std::unordered_map<common::xaccount_address_t, xcontract_deployment_data_t> m_system_contract_deployment_data;
    // base::xvblockstore_t* m_blockstore;

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
    static xtop_system_contract_manager & instance();
    void deploy();
    void setup(base::xvblockstore_t * blockstore);
private:
    observer_ptr<system_contracts::xbasic_system_contract_t> system_contract(common::xaccount_address_t const & address) const noexcept;
    bool contains(common::xaccount_address_t const & address) const noexcept;

private:
    // void deploy_system_contract(common::xaccount_address_t const& address,
    //                             xblock_sniff_config_t sniff_config,
    //                             contract_deploy_type_t deploy_type,
    //                             std::string const& broadcast_target,
    //                             contract_broadcast_policy_t broadcast_policy = contract_broadcast_policy_t::invalid);

    // void init_system_contract(common::xaccount_address_t const & contract_address);

    void setup_address(const common::xaccount_address_t & contract_address, const common::xaccount_address_t & cluster_address);
    // need vm support
    void setup_chain(const common::xaccount_address_t & cluster_address, base::xvblockstore_t * blockstore);
    bool process_sniffed(const base::xvblock_ptr_t & block);
    // bool do_on_timer(const common::xaccount_address_t & contract, const xblock_sniff_config_data_t & data, const base::xvblock_ptr_t & block);

    std::unordered_map<common::xaccount_address_t, observer_ptr<system_contracts::xbasic_system_contract_t>> m_contract_inst_map;
    base::xrwlock_t m_rwlock;
};
using xsystem_contract_manager_t = xtop_system_contract_manager;

NS_END2
