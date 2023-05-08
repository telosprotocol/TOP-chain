// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xvnode.h"

#include "xmbus/xevent_role.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvnetwork/xvnetwork_driver.h"
#include "xvnode/xerror/xerror.h"
#include "xblockstore/xblockstore_face.h"

NS_BEG2(top, vnode)

xtop_vnode::xtop_vnode(observer_ptr<elect::ElectMain> const & elect_main,
                       common::xsharding_address_t const & sharding_address,
                       common::xslot_id_t const & slot_id,
                       common::xelection_round_t joined_election_round,
                       common::xminer_type_t miner_type,
                       bool genesis,
                       uint64_t raw_credit_score,
                       common::xelection_round_t election_round,
                       std::uint16_t const group_size,
                       std::uint64_t const associated_blk_height,
                       observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                       observer_ptr<router::xrouter_face_t> const & router,
                       observer_ptr<base::xvblockstore_t> const & block_store,
                       observer_ptr<base::xvtxstore_t> const & txstore,
                       observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                       observer_ptr<time::xchain_time_face_t> const & logic_timer,
                       observer_ptr<sync::xsync_object_t> const & sync_obj,
                       observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> const & txpool_service_mgr,
                       observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
                       observer_ptr<base::xvnodesrv_t> const & nodesvr)
  : xbasic_vnode_t{common::xnode_address_t{sharding_address,
                                           common::xaccount_election_address_t{vhost->account_address(), slot_id},
                                           election_round,
                                           group_size,
                                           associated_blk_height},
                   miner_type,
                   genesis,
                   raw_credit_score,
                   joined_election_round,
                   vhost,
                   election_cache_data_accessor}
  , m_elect_main{elect_main}
  , m_router{router}
  , m_block_store{block_store}
  , m_txstore{txstore}
  , m_bus{bus}
  , m_logic_timer{logic_timer}
  , m_sync_obj{sync_obj}
  , m_user_params{make_observer(std::addressof(data::xuser_params::get_instance()))}
  , m_the_binding_driver{std::make_shared<vnetwork::xvnetwork_driver_t>(
        m_vhost, m_election_cache_data_accessor,
        common::xnode_address_t{sharding_address, common::xaccount_election_address_t{m_vhost->account_address(), slot_id}, election_round, group_size, associated_blk_height},
        joined_election_round)}
   {
    bool const is_edge_archive = common::has<common::xnode_type_t::storage>(m_the_binding_driver->type()) || common::has<common::xnode_type_t::edge>(m_the_binding_driver->type());
    bool const is_frozen = common::has<common::xnode_type_t::frozen>(m_the_binding_driver->type());
    if (!is_edge_archive && !is_frozen && !common::has<common::xnode_type_t::fullnode>(m_the_binding_driver->type())) {
        m_txpool_face = txpool_service_mgr->create(m_the_binding_driver, m_router);

        xwarn("[virtual node] vnode %p create at address %s txproxy:%p",
              this,
              m_the_binding_driver->address().to_string().c_str(),
              static_cast<void *>(m_txpool_face.get()));
    } else {
        xwarn("[virtual node] vnode %p create at address %s", this, m_the_binding_driver->address().to_string().c_str());
    }
    m_prune_data = top::make_unique<components::prune_data::xprune_data>();
    m_sniff = top::make_unique<components::sniffing::xsniffer_t>(
        nodesvr, make_observer(contract_runtime::system::xsystem_contract_manager_t::instance()), make_observer(this));
}

xtop_vnode::xtop_vnode(observer_ptr<elect::ElectMain> const & elect_main,
                       observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                       std::shared_ptr<election::cache::xgroup_element_t> group_info,
                       observer_ptr<router::xrouter_face_t> const & router,
                       observer_ptr<base::xvblockstore_t> const & block_store,
                       observer_ptr<base::xvtxstore_t> const & txstore,
                       observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                       observer_ptr<time::xchain_time_face_t> const & logic_timer,
                       observer_ptr<sync::xsync_object_t> const & sync_obj,
                       //    observer_ptr<xunit_service::xcons_service_mgr_face> const & cons_mgr,
                       observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> const & txpool_service_mgr,
                       observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
                       observer_ptr<base::xvnodesrv_t> const & nodesvr)
  : xtop_vnode{elect_main,
               group_info->node_element(vhost->account_address())->address().sharding_address(),
               group_info->node_element(vhost->account_address())->slot_id(),
               group_info->node_element(vhost->account_address())->joined_election_round(),
               group_info->node_element(vhost->account_address())->election_info().miner_type(),
               group_info->node_element(vhost->account_address())->election_info().genesis(),
               group_info->node_element(vhost->account_address())->raw_credit_score(),
               group_info->election_round(),
               group_info->group_size(),
               group_info->associated_blk_height(),
               vhost,
               router,
               block_store,
               txstore,
               bus,
               logic_timer,
               sync_obj,
               txpool_service_mgr,
               election_cache_data_accessor,
               nodesvr} {}

std::shared_ptr<vnetwork::xvnetwork_driver_face_t> const & xtop_vnode::vnetwork_driver() const {
    return m_the_binding_driver;
}

//xtxpool_service_v2::xtxpool_proxy_face_ptr const & xtop_vnode::txpool_proxy() const {
//    return m_txpool_face;
//}

void xtop_vnode::synchronize() {
    m_the_binding_driver->start();

    xinfo("[virtual node] vnode (%p) start synchronizing at address %s", this, m_the_binding_driver->address().to_string().c_str());
}

void xtop_vnode::start() {
    assert(!running());

    assert(m_the_binding_driver != nullptr);
    assert(m_router != nullptr);
    assert(m_logic_timer != nullptr);
    assert(m_vhost != nullptr);

    top::store::install_block_recycler(nullptr);
    sync_add_vnet();
    new_driver_added();
    // m_grpc_mgr->try_add_listener(common::has<common::xnode_type_t::storage_archive>(vnetwork_driver()->type()) ||
    //                              common::has<common::xnode_type_t::storage_exchange>(vnetwork_driver()->type()));
    // if (m_cons_face != nullptr) {
    //     m_cons_face->start(this->start_time());
    // }
    if (m_txpool_face != nullptr) {
        m_txpool_face->start();
    }

    running(true);
    xkinfo("[virtual node] vnode (%p) start running at address %s", this, m_the_binding_driver->address().to_string().c_str());
}

void xtop_vnode::fade() {
    assert(running());
    assert(!faded());
    assert(m_the_binding_driver != nullptr);
    assert(m_the_binding_driver->running());

    update_contract_manager(true);

    sync_remove_vnet();
    // if (m_cons_face != nullptr) {
    //     m_cons_face->fade();
    // }
    if (m_txpool_face != nullptr) {
        m_txpool_face->fade();
    }
    m_faded.store(true, std::memory_order_release);
}

void xtop_vnode::stop() {
    assert(running());
    assert(faded());
    // any component can stop should
    // control multi-times stop
    if (m_txpool_face != nullptr) {
        m_txpool_face->unreg();
    }
    // m_grpc_mgr->try_remove_listener(common::has<common::xnode_type_t::storage_archive>(vnetwork_driver()->type()));
    running(false);
    driver_removed();
    update_contract_manager(true);
    xkinfo("[virtual node] vnode (%p) stop running at address %s", this, m_the_binding_driver->address().to_string().c_str());
}

void xtop_vnode::new_driver_added() {
    update_rpc_service();
    update_contract_manager(false);

    update_auto_prune_control(m_the_binding_driver->type(), nullptr);
    xkinfo("new_driver_added node type:%s", common::to_string(m_the_binding_driver->type()).c_str());
}

void xtop_vnode::driver_removed() {
    if (m_rpc_services != nullptr) {
        m_rpc_services->stop();
    }
    // if (common::has<common::xnode_type_t::storage_exchange>(m_the_binding_driver->type()) && m_tx_prepare_mgr != nullptr) {
    //     m_tx_prepare_mgr->stop();
    // }
    sync_remove_vnet();
}

bool  xtop_vnode::update_auto_prune_control(top::common::xnode_type_t node_type, base::xvdbstore_t* xvdb_ptr)
{
    xinfo("try update block prune. node type %s", common::to_string(m_the_binding_driver->type()).c_str());

    if(base::xvchain_t::instance().is_auto_prune_enable() == false)
        return false;//not allow change anymore

    if (common::has<common::xnode_type_t::frozen>(node_type)) {
        return false;
    }

    if (!common::has<common::xnode_type_t::storage>(node_type)) {
        return true;
    }

    //force to turn off auto_prune for archive node
    base::xvchain_t::instance().enable_auto_prune(false);

    //froce to turn off it at config file
    std::string prune_off("off");
    m_prune_data->update_prune_config_file(prune_off);

    return true;
}

void xtop_vnode::update_rpc_service() {
    xdbg("try update rpc service. node type %s", common::to_string(m_the_binding_driver->type()).c_str());
    if (!common::has<common::xnode_type_t::frozen>(m_the_binding_driver->type())) {
        auto const http_port = XGET_CONFIG(http_port);
        auto const ws_port = XGET_CONFIG(ws_port);
        // TODO(justin): remove unit_services temp
        xdbg("[virtual node] update rpc service with node type %s address %s",
             common::to_string(m_the_binding_driver->type()).c_str(),
             m_the_binding_driver->address().to_string().c_str());
        m_rpc_services = std::make_shared<xrpc::xrpc_init>(m_the_binding_driver,
                                                           m_the_binding_driver->type(),
                                                           m_router,
                                                           http_port,
                                                           ws_port,
                                                           m_txpool_face,
                                                           m_block_store,
                                                           m_txstore,
                                                           m_elect_main,
                                                           m_election_cache_data_accessor);
     }
}

void xtop_vnode::update_contract_manager(bool destory) {
    // TODO(justin): remove unit_services temp
    contract::xcontract_manager_t::instance().push_event(make_object_ptr<mbus::xevent_vnode_t>(destory, m_txpool_face, m_the_binding_driver));
}

void xtop_vnode::sync_add_vnet() {
    if (miner_type() != common::xenum_miner_type::invalid) {
        bool is_storage_node = false;
        bool has_other_node = false;
        if (genesis()
            || common::has<common::xminer_type_t::archive>(miner_type())
            || common::has<common::xminer_type_t::exchange>(miner_type())) {
            is_storage_node = true;
        }
        if (genesis()
            || common::has<common::xminer_type_t::advance>(miner_type())
            || common::has<common::xminer_type_t::validator>(miner_type())
            || common::has<common::xminer_type_t::edge>(miner_type())) {
            has_other_node = true;
        }

        base::xvchain_t::instance().set_node_type(is_storage_node, has_other_node);
    }

    m_sync_obj->add_vnet(vnetwork_driver(), miner_type(), genesis());

    xinfo("xtop_vnode::sync_add_vnet vnode (%p) at address %s starts synchronizing. miner_type=%d,genesis=%d", this, address().to_string().c_str(), miner_type(), genesis());
}

void xtop_vnode::sync_remove_vnet() {
    m_sync_obj->remove_vnet(vnetwork_driver(), miner_type(), genesis());
}

//std::vector<common::xip2_t> get_group_nodes_xip2_from(std::shared_ptr<xvnode_face_t> const & vnode, common::xip_t const & group_xip, std::error_code & ec) const {
//    assert(!ec);
//
//    if (address().xip2().xip().group_xip() == group_xip) {
//        return neighbors_xip2(ec);
//    }
//}

components::sniffing::xsniffer_config_t xtop_vnode::sniff_config() const {
    return m_sniff->sniff_config();
}

NS_END2
