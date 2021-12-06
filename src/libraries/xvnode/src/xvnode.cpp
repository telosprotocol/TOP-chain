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
                       common::xelection_round_t election_round,
                       std::uint16_t const group_size,
                       std::uint64_t const associated_blk_height,
                       observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                       observer_ptr<router::xrouter_face_t> const & router,
                       observer_ptr<store::xstore_face_t> const & store,
                       observer_ptr<base::xvblockstore_t> const & block_store,
                       observer_ptr<base::xvtxstore_t> const & txstore,
                       observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                       observer_ptr<time::xchain_time_face_t> const & logic_timer,
                       observer_ptr<sync::xsync_object_t> const & sync_obj,
                       observer_ptr<grpcmgr::xgrpc_mgr_t> const & grpc_mgr,
                    //    observer_ptr<xunit_service::xcons_service_mgr_face> const & cons_mgr,
                       observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> const & txpool_service_mgr,
                       observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                       observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
  : xbasic_vnode_t{common::xnode_address_t{sharding_address,
                                           common::xaccount_election_address_t{vhost->host_node_id(), slot_id},
                                           election_round,
                                           group_size,
                                           associated_blk_height},
                   joined_election_round,
                   vhost,
                   election_cache_data_accessor}
  , m_elect_main{elect_main}
  , m_router{router}
  , m_store{store}
  , m_block_store{block_store}
  , m_txstore{txstore}
  , m_bus{bus}
  , m_logic_timer{logic_timer}
  , m_sync_obj{sync_obj}
  , m_grpc_mgr{grpc_mgr}
  , m_txpool{txpool}
  , m_dev_params{make_observer(std::addressof(data::xdev_params::get_instance()))}
  , m_user_params{make_observer(std::addressof(data::xuser_params::get_instance()))}
  , m_the_binding_driver{std::make_shared<vnetwork::xvnetwork_driver_t>(
        m_vhost, m_election_cache_data_accessor,
        common::xnode_address_t{sharding_address, common::xaccount_election_address_t{m_vhost->host_node_id(), slot_id}, election_round, group_size, associated_blk_height},
        joined_election_round)} {
    bool is_edge_archive = common::has<common::xnode_type_t::storage>(m_the_binding_driver->type()) || common::has<common::xnode_type_t::edge>(m_the_binding_driver->type());
    bool is_frozen = common::has<common::xnode_type_t::frozen>(m_the_binding_driver->type());
    if (!is_edge_archive && !is_frozen) {
        // m_cons_face = cons_mgr->create(m_the_binding_driver);
        m_txpool_face = txpool_service_mgr->create(m_the_binding_driver, m_router);

        // xwarn("[virtual node] vnode %p create at address %s cons_proxy:%p txproxy:%p",
        //       this,
        //       m_the_binding_driver->address().to_string().c_str(),
        //       m_cons_face.get(),
        //       m_txpool_face.get());
    } else {
        xwarn("[virtual node] vnode %p create at address %s", this, m_the_binding_driver->address().to_string().c_str());
    }
    m_prune_data = make_unique<components::prune_data::xprune_data>();
}

xtop_vnode::xtop_vnode(observer_ptr<elect::ElectMain> const & elect_main,
                       observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                       std::shared_ptr<election::cache::xgroup_element_t> group_info,
                       observer_ptr<router::xrouter_face_t> const & router,
                       observer_ptr<store::xstore_face_t> const & store,
                       observer_ptr<base::xvblockstore_t> const & block_store,
                       observer_ptr<base::xvtxstore_t> const & txstore,
                       observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                       observer_ptr<time::xchain_time_face_t> const & logic_timer,
                       observer_ptr<sync::xsync_object_t> const & sync_obj,
                       observer_ptr<grpcmgr::xgrpc_mgr_t> const & grpc_mgr,
                       //    observer_ptr<xunit_service::xcons_service_mgr_face> const & cons_mgr,
                       observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> const & txpool_service_mgr,
                       observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                       observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
  : xtop_vnode{elect_main,
               group_info->node_element(vhost->host_node_id())->address().sharding_address(),
               group_info->node_element(vhost->host_node_id())->slot_id(),
               group_info->node_element(vhost->host_node_id())->joined_election_round(),
               group_info->election_round(),
               group_info->group_size(),
               group_info->associated_blk_height(),
               vhost,
               router,
               store,
               block_store,
               txstore,
               bus,
               logic_timer,
               sync_obj,
               grpc_mgr,
               //    cons_mgr,
               txpool_service_mgr,
               txpool,
               election_cache_data_accessor} {
}

std::shared_ptr<vnetwork::xvnetwork_driver_face_t> const & xtop_vnode::vnetwork_driver() const noexcept {
    return m_the_binding_driver;
}

void xtop_vnode::synchronize() {
    if (m_sync_started)
        return;

    sync_add_vnet();

    m_sync_started = true;

    m_the_binding_driver->start();

    xinfo("[virtual node] vnode (%p) start synchronizing at address %s", this, m_the_binding_driver->address().to_string().c_str());
}

void xtop_vnode::start() {
    assert(!running());

    assert(m_the_binding_driver != nullptr);
    assert(m_router != nullptr);
    assert(m_logic_timer != nullptr);
    assert(m_vhost != nullptr);

    new_driver_added();
    m_grpc_mgr->try_add_listener(common::has<common::xnode_type_t::storage_archive>(vnetwork_driver()->type()) ||
        common::has<common::xnode_type_t::storage_full_node>(vnetwork_driver()->type()));
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
    m_grpc_mgr->try_remove_listener(common::has<common::xnode_type_t::storage_archive>(vnetwork_driver()->type()));
    running(false);
    m_the_binding_driver->stop();
    driver_removed();
    update_contract_manager(true);

    xkinfo("[virtual node] vnode (%p) stop running at address %s", this, m_the_binding_driver->address().to_string().c_str());
}

void xtop_vnode::new_driver_added() {
    // need call by order, if depends on other components
    // for example : store depends on message bus, then
    // update_message_bus should be called before update_store
    // update_consensus_instance();
    // update_unit_service();
    // update_tx_cache_service();
    update_rpc_service();
    update_contract_manager(false);
    
    update_auto_prune_control(m_the_binding_driver->type(), m_store.get());
    xkinfo("new_driver_added node type:%s", common::to_string(m_the_binding_driver->type()).c_str());
}

void xtop_vnode::driver_removed() {
    if (m_rpc_services != nullptr) {
        m_rpc_services->stop();
    }
    // if (common::has<common::xnode_type_t::storage_full_node>(m_the_binding_driver->type()) && m_tx_prepare_mgr != nullptr) {
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
        // if (base::xvchain_t::instance().get_round_number() < 5) {
        //     xdbg("wait to start enable_block_recycler: %d", base::xvchain_t::instance().get_round_number());
        //     base::xvchain_t::instance().add_round_number();
        //     return true;
        // }
        // uint64_t now = base::xvchain_t::instance().get_time_now();
        // if (base::xvchain_t::instance().get_start_time() + 5 * 60 * 1000000 < now) {
        //     xdbg("wait for enough time to start prune data: %" PRIu64 ",%" PRIu64, base::xvchain_t::instance().get_start_time(), now);
        //     return true;
        // }
        if (top::store::install_block_recycler(xvdb_ptr))
            xinfo("install_block_recycler ok.");
        else
            xerror("install_block_recycler fail.");
        
        if (top::store::enable_block_recycler(true))
            xinfo("enable_block_recycler ok.");
        else
            xerror("enable_block_recycler fail.");
        return true;
    }
    
    //detect it is archive node
    if (top::store::enable_block_recycler(false))
        xinfo("disable_block_recycler ok.");
    else
        xerror("disable_block_recycler fail.");
    
    //force to turn off auto_prune for archive node
    base::xvchain_t::instance().enable_auto_prune(false);
    
    //froce to turn off it at config file
    std::string prune_off("off");
    m_prune_data->update_prune_config_file(prune_off);
    
    return true;
}

void xtop_vnode::update_rpc_service() {
    xdbg("try update rpc service. node type %s", common::to_string(m_the_binding_driver->type()).c_str());
    if (!common::has<common::xnode_type_t::storage_archive>(m_the_binding_driver->type()) &&
        !common::has<common::xnode_type_t::frozen>(m_the_binding_driver->type())) {
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
                                                           m_store,
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
    assert(!m_sync_started);

    m_sync_obj->add_vnet(vnetwork_driver());

    xinfo("vnode (%p) at address %s starts synchronizing", this, address().to_string().c_str());
}

void xtop_vnode::sync_remove_vnet() {
    // if ((type() & common::xnode_type_t::edge) == common::xnode_type_t::invalid) {

    m_sync_obj->remove_vnet(vnetwork_driver());

    //}
}

//std::vector<common::xip2_t> get_group_nodes_xip2_from(std::shared_ptr<xvnode_face_t> const & vnode, common::xip_t const & group_xip, std::error_code & ec) const {
//    assert(!ec);
//
//    if (address().xip2().xip().group_xip() == group_xip) {
//        return neighbors_xip2(ec);
//    }
//}

NS_END2
