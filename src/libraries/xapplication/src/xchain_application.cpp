// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xapplication/xchain_application.h"

#include "xapplication/xapplication.h"
// #include "xapplication/xcons_mgr_builder.h"
#include "xtxpool_service_v2/xtxpool_service_mgr.h"
#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xdata/xchain_param.h"
#include "xelection/xcache/xdata_accessor.h"
#include "xelection/xdata_accessor_error.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvnetwork/xvhost.h"
#include "xvnode/xvnode_manager.h"
#include "xchaininit/xchain_command.h"
#include "xchaininit/xchain_info_query.h"
#include "xelect_net/include/multilayer_network_chain_query.h"

#include <cinttypes>
#include <functional>

NS_BEG2(top, application)

xtop_chain_application::xtop_chain_application(observer_ptr<xapplication_t> const &                 application,
                                               common::xnetwork_id_t const &                        network_id,
                                               xobject_ptr_t<base::xvblockstore_t> &blockstore,
                                               xobject_ptr_t<base::xvnodesrv_t> &nodesvr_ptr,
                                               xobject_ptr_t<base::xvcertauth_t> &cert_ptr,
                                               observer_ptr<base::xiothread_t> const &              grpc_thread,
                                               observer_ptr<base::xiothread_t> const &              sync_thread,
                                               std::vector<observer_ptr<base::xiothread_t>> const & sync_account_thread_pool,
                                               std::vector<observer_ptr<base::xiothread_t>> const & sync_handler_thread_pool)
  : m_application{application}
  , m_network_id{network_id}
  , m_elect_client_process{std::make_shared<elect::xelect_client_process>(
        m_network_id,
        m_application->message_bus(),
        std::bind(&xtop_chain_application::on_election_data_updated, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        m_application->logic_timer())}
  , m_election_cache_data_accessor{top::make_unique<election::cache::xdata_accessor_t>(network_id, m_application->logic_timer())}
  , m_vhost{std::make_shared<vnetwork::xvhost_t>(application->network_driver(m_network_id),
                                                 application->logic_timer(),
                                                 network_id,
                                                 make_observer(m_election_cache_data_accessor))}
  , m_message_callback_hub{std::make_shared<vnetwork::xmessage_callback_hub_t>(make_observer(m_vhost))}
  , m_sync_obj{top::make_unique<sync::xsync_object_t>(application->message_bus(), application->store(), make_observer(m_vhost), blockstore, nodesvr_ptr, cert_ptr,
                                                                                        sync_thread, sync_account_thread_pool, sync_handler_thread_pool)}
  , m_grpc_mgr{top::make_unique<grpcmgr::xgrpc_mgr_t>(m_application->message_bus(), grpc_thread)}
//   , m_cons_mgr{xcons_mgr_builder::build(data::xuser_params::get_instance().account.value(),
//                                         m_application->store(),
//                                         m_application->blockstore(),
//                                         m_application->txpool(),
//                                         m_application->logic_timer(),
//                                         m_application->cert_serivce(),
//                                         make_observer(m_election_cache_data_accessor),
//                                         m_application->message_bus(),
//                                         m_application->router())}
  , m_txpool_service_mgr{xtxpool_service_v2::xtxpool_service_mgr_instance::create_xtxpool_service_mgr_inst(m_application->store(),
                                                                                                        make_observer(m_application->blockstore().get()),
                                                                                                        m_application->txpool(),
                                                                                                        m_application->thread_pool(xthread_pool_type_t::txpool_service),
                                                                                                        m_application->message_bus(),
                                                                                                        m_application->logic_timer())}
  , m_vnode_manager{std::make_shared<vnode::xvnode_manager_t>(m_application->elect_main(),
                                                              m_application->message_bus(),
                                                              m_application->store(),
                                                              make_observer(m_application->blockstore().get()),
                                                              m_application->txstore(),
                                                              m_application->logic_timer(),
                                                              m_application->router(),
                                                              m_application->cert_serivce(),
                                                              make_observer(m_vhost),
                                                              make_observer(m_sync_obj),
                                                              make_observer(m_grpc_mgr),
                                                            //   make_observer(m_cons_mgr),
                                                              make_observer(m_txpool_service_mgr.get()),
                                                              m_application->txpool(),
                                                              make_observer(m_election_cache_data_accessor),
                                                              make_observer(m_application->node_service().get()))} {
                                                              }

void xtop_chain_application::start() {
    contract::xcontract_manager_t::instance().install_monitors(
        m_application->message_bus(), make_observer(m_message_callback_hub.get()), m_application->store(), m_application->syncstore());
    load_last_election_data();

    m_txpool_service_mgr->start();
    m_vhost->start();
    m_message_callback_hub->start();
    m_vnode_manager->start();

    top_grpc_init(XGET_CONFIG(grpc_port));

    m_sync_obj->start();

    top_console_init();

    auto const & frozen_sharding_address = common::build_frozen_sharding_address(m_network_id);
    auto const zone_type = common::node_type_from(frozen_sharding_address.zone_id());

    data::election::xelection_result_store_t election_result_store{};
    auto & static_sync_group = election_result_store.result_of(m_network_id)
                                                    .result_of(zone_type)
                                                    .result_of(frozen_sharding_address.cluster_id())
                                                    .result_of(frozen_sharding_address.group_id());
    static_sync_group.start_time(0);
    static_sync_group.group_version(common::xelection_round_t::max());

    auto & static_sync_node = static_sync_group.result_of(node_id());
    static_sync_node.joined_version = common::xelection_round_t::max();
    static_sync_node.stake = 0;

    assert(static_sync_group.size() == 1);
    on_election_data_updated(election_result_store, frozen_sharding_address.zone_id(), 0);
}

void xtop_chain_application::stop() {
    m_sync_obj->stop();

    m_vnode_manager->stop();
    m_message_callback_hub->stop();
    m_vhost->stop();
}

void xtop_chain_application::on_election_data_updated(data::election::xelection_result_store_t const & election_result_store,
                                                      common::xzone_id_t const & zid,
                                                      std::uint64_t const associated_blk_height) {
    std::error_code ec{election::xdata_accessor_errc_t::success};
    auto const & updated_election_data2 = m_election_cache_data_accessor->update_zone(zid, election_result_store, associated_blk_height, ec);

    if (ec) {
        xwarn("%s network %" PRIu32 " update zone(zid:%d) is not fully successful %s",
              ec.category().name(),
              static_cast<std::uint32_t>(m_network_id.value()),
              zid.value(),
              ec.message().c_str());
    }

    if (!updated_election_data2.empty()) {
        auto outdated_xips_pair = m_vnode_manager->handle_election_data(updated_election_data2);
        for (const auto & xip : outdated_xips_pair.first) {
            m_application->elect_manager()->OnElectQuit(xip);
        }
        for (const auto & xip : outdated_xips_pair.second) {
            // m_cons_mgr->destroy({xip.raw_low_part(), xip.raw_high_part()});
            m_txpool_service_mgr->destroy({xip.raw_low_part(), xip.raw_high_part()});
        }
        if (zid != common::xfrozen_zone_id) {
            m_application->elect_manager()->OnElectUpdated(election_result_store, zid, associated_blk_height);
        }
    } else {
        xinfo("[xchain_application] network %" PRIu32 " empty election data.  it may be from synchronization", static_cast<std::uint32_t>(m_network_id.value()));
    }
}

common::xnode_id_t const & xtop_chain_application::node_id() const noexcept {
    return m_application->node_id();
}

xpublic_key_t const & xtop_chain_application::public_key() const noexcept {
    return m_application->public_key();
}

std::string const & xtop_chain_application::sign_key() const noexcept {
    return m_application->sign_key();
}

void xtop_chain_application::top_grpc_init(uint16_t const grpc_port) {
    if (enable_grpc_service()) {
        grpcmgr::grpc_init(m_application->store().get(), m_application->blockstore().get(), m_sync_obj.get(), grpc_port);
    }
}

void xtop_chain_application::top_console_init() {
    top::elect::MultilayerNetworkInterfacePtr net_module = std::make_shared<elect::MultilayerNetworkChainQuery>();
    ChainCommandsPtr chain_cmd {nullptr};
    chain_cmd = std::make_shared<ChainCommands>(net_module, m_sync_obj.get());
    top::ChainInfo::Instance()->SetChainCmd(chain_cmd);
    std::cout << "==== start chaininfo center ===\n";
    xinfo("==== start chaininfo center ===");
}

NS_END2
