// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xvnode/xcomponents/xvnode_sniff/xvnode_sniff_config.h"

NS_BEG2(top, vnode)

class xtop_vnode_sniff {
public:
    struct xtop_role_config {
        contract_runtime::system::xcontract_deployment_data_t role_data;
        std::map<common::xaccount_address_t, uint64_t> address_round;   // record address and timer round
    };
    using xrole_config_t = xtop_role_config;

    xtop_vnode_sniff(xtop_vnode_sniff const &) = delete;
    xtop_vnode_sniff & operator=(xtop_vnode_sniff const &) = delete;
    xtop_vnode_sniff(xtop_vnode_sniff &&) = default;
    xtop_vnode_sniff & operator=(xtop_vnode_sniff &&) = default;
    ~xtop_vnode_sniff() = default;

    xtop_vnode_sniff(observer_ptr<store::xstore_face_t> const & store,
                     observer_ptr<contract_runtime::system::xsystem_contract_manager_t> const & manager,
                     observer_ptr<vnetwork::xvnetwork_driver_face_t> const & driver,
                     observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool);

    void sniff_set();
    bool sniff_broadcast(xobject_ptr_t<base::xvblock_t> const & vblock);
    bool sniff_timer(xobject_ptr_t<base::xvblock_t> const & vblock);
    bool sniff_block(xobject_ptr_t<base::xvblock_t> const & vblock);
    xvnode_sniff_config_t sniff_config();

    bool is_valid_timer_call(common::xaccount_address_t const & address, xrole_config_t & data, const uint64_t height) const;
    void call(common::xaccount_address_t const & address, std::string const & action_name, std::string const & action_params, const uint64_t timestamp);
    void call(common::xaccount_address_t const & source_address,
              common::xaccount_address_t const & target_address,
              std::string const & action_name,
              std::string const & action_params,
              uint64_t timestamp);

private:
    observer_ptr<store::xstore_face_t> m_store;
    observer_ptr<contract_runtime::system::xsystem_contract_manager_t> m_system_contract_manager;
    observer_ptr<vnetwork::xvnetwork_driver_face_t> m_the_binding_driver;
    observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> m_txpool_face;
    std::map<common::xaccount_address_t, xrole_config_t> m_config_map; 
};

using xvnode_sniff_t = xtop_vnode_sniff;

NS_END2
