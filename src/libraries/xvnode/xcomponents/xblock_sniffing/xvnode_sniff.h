// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvcnode.h"
#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xvnode/xcomponents/xblock_sniffing/xvnode_sniff_config.h"

NS_BEG4(top, vnode, components, sniffing)

struct xtable_schedule_info_t {
    uint16_t    cur_interval{0};
    uint16_t    target_interval{0};
    bool        clock_or_table{false}; ///< defualt false, means interval is clock interval, otherwise means table num
    uint16_t    cur_table{0};

    xtable_schedule_info_t() = default;
    xtable_schedule_info_t(uint16_t clock_interval, uint16_t start_table): target_interval{clock_interval}, cur_table{start_table}{}
};

class xtop_vnode_sniff {
public:
    struct xtop_role_config {
        contract_runtime::system::xcontract_deployment_data_t role_data;
        std::map<common::xaccount_address_t, uint64_t> address_round;  // record address and timer round
    };
    using xrole_config_t = xtop_role_config;

private:
    observer_ptr<store::xstore_face_t> m_store;
    observer_ptr<base::xvnodesrv_t> m_nodesvr;
    observer_ptr<contract_runtime::system::xsystem_contract_manager_t> m_system_contract_manager;
    observer_ptr<vnetwork::xvnetwork_driver_face_t> m_the_binding_driver;
    observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> m_txpool_face;
    mutable std::map<common::xaccount_address_t, xrole_config_t> m_config_map;
    mutable std::unordered_map<common::xaccount_address_t, xtable_schedule_info_t>      m_table_contract_schedule; // table schedule

public:
    xtop_vnode_sniff(xtop_vnode_sniff const &) = delete;
    xtop_vnode_sniff & operator=(xtop_vnode_sniff const &) = delete;
    xtop_vnode_sniff(xtop_vnode_sniff &&) = default;
    xtop_vnode_sniff & operator=(xtop_vnode_sniff &&) = default;
    ~xtop_vnode_sniff() = default;

    xtop_vnode_sniff(observer_ptr<store::xstore_face_t> const & store,
                     observer_ptr<base::xvnodesrv_t> const& nodesrv,
                     observer_ptr<contract_runtime::system::xsystem_contract_manager_t> const & manager,
                     observer_ptr<vnetwork::xvnetwork_driver_face_t> const & driver,
                     observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool);

    void sniff_set();
    bool sniff_broadcast(xobject_ptr_t<base::xvblock_t> const & vblock) const;
    bool sniff_timer(xobject_ptr_t<base::xvblock_t> const & vblock) const;
    bool sniff_block(xobject_ptr_t<base::xvblock_t> const & vblock) const;
    xvnode_sniff_config_t sniff_config() const;

    
    void call(common::xaccount_address_t const & address, std::string const & action_name, std::string const & action_params, const uint64_t timestamp) const;
    void call(common::xaccount_address_t const& address, std::string const& action_name, std::string const& action_params, uint64_t timestamp, uint64_t table_id) const;
    void call(common::xaccount_address_t const & source_address,
              common::xaccount_address_t const & target_address,
              std::string const & action_name,
              std::string const & action_params,
              uint64_t timestamp) const;

private:
    bool is_valid_timer_call(common::xaccount_address_t const & address, xrole_config_t & data, const uint64_t height) const;
    bool trigger_first_timer_call(common::xaccount_address_t const & address) const;
};

using xvnode_sniff_t = xtop_vnode_sniff;

NS_END4
