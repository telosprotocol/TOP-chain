// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xapplication/xapplication.h"

#include "xapplication/xerror/xerror.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xscope_executer.h"
#include "xbasic/xtimer_driver.h"
#include "xblockstore/xblockstore_face.h"
#include "xcertauth/xcertauth_face.h"
#include "xchain_fork/xutility.h"
#include "xchain_timer/xchain_timer.h"
#include "xchaininit/xchain_command.h"
#include "xchaininit/xchain_info_query.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xconfig/xutility.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xelect/client/xelect_client.h"
#include "xelect_net/include/multilayer_network.h"
#include "xelect_net/include/multilayer_network_chain_query.h"
#include "xelection/xcache/xdata_accessor.h"
#include "xelection/xdata_accessor_error.h"
#include "xelection/xvnode_house.h"
#include "xgenesis/xgenesis_manager.h"
#include "xgrpc_mgr/xgrpc_mgr.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xmbus/xmessage_bus.h"
#include "xrouter/xrouter.h"
#include "xsafebox/safebox_proxy.h"
#include "xstore/xstore_error.h"
#include "xsync/xsync_object.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"
#include "xvnetwork/xvhost.h"
#include "xvnode/xvnode_manager.h"
#include "xstatistic/xstatistic.h"

NS_BEG2(top, application)

xtop_application::xtop_application(common::xnode_id_t const & node_id, xpublic_key_t const & public_key, std::string && sign_key) // todo make it right value
  : m_node_id{node_id}
  , m_public_key{public_key}
  , m_network_id{top::config::to_chainid(XGET_CONFIG(chain_name))}
  , m_io_context_pools{{xio_context_type_t::general, {std::make_shared<xbase_io_context_wrapper_t>()}}}
  , m_timer_driver{std::make_shared<xbase_timer_driver_t>(m_io_context_pools[xio_context_type_t::general].front())}
  , m_elect_main{top::make_unique<elect::ElectMain>(node_id, std::set<uint32_t>({static_cast<uint32_t>(m_network_id)}))}
  , m_router{top::make_unique<router::xrouter_t>()}
  , m_bus{top::make_object_ptr<mbus::xmessage_bus_t>(true, 1000)}
  , m_logic_timer{make_object_ptr<time::xchain_timer_t>(m_timer_driver)}
  , m_grpc_thread{make_object_ptr<base::xiothread_t>()}
  , m_sync_thread{make_object_ptr<base::xiothread_t>()}
  , m_elect_client{top::make_unique<elect::xelect_client_imp>()} {

    safebox::xsafebox_proxy::get_instance().add_key_pair(public_key, std::move(sign_key));

#ifdef CACHE_SIZE_STATISTIC
    xstatistic::xstatistic_hub_t::instance(); // create singleton at very first time.
#endif

    int db_kind = top::db::xdb_kind_kvdb;
    std::vector<db::xdb_path_t> db_data_paths{};
    base::xvchain_t::instance().get_db_config_custom(db_data_paths, db_kind);
    std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create(db_kind, XGET_CONFIG(db_path), db_data_paths);
    m_store = store::xstore_factory::create_store_with_static_kvdb(db);
    base::xvchain_t::instance().set_xdbstore(m_store.get());
    base::xvchain_t::instance().set_xevmbus(m_bus.get());
    m_blockstore.attach(store::get_vblockstore());

    m_txstore = xobject_ptr_t<base::xvtxstore_t>(
        txstore::create_txstore(top::make_observer<mbus::xmessage_bus_face_t>(m_bus.get()), top::make_observer<xbase_timer_driver_t>(m_timer_driver)));
    base::xvchain_t::instance().set_xtxstore(m_txstore.get());

    m_nodesvr_ptr = make_object_ptr<election::xvnode_house_t>(node_id, m_blockstore, make_observer(m_bus.get()));

    m_cert_ptr.attach(&auth::xauthcontext_t::instance(*m_nodesvr_ptr.get()));

    // genesis blocks should init imediately after db created
    m_genesis_manager = make_unique<genesis::xgenesis_manager_t>(top::make_observer(m_blockstore.get()));

    if ((m_store == nullptr) || !m_store->open()) {
        xwarn("xtop_application::start db open failed!");
        exit(0);
    }

    // prepare system contract data only
    contract::xcontract_deploy_t::instance().deploy_sys_contracts();
    contract::xcontract_manager_t::instance().instantiate_sys_contracts();
    contract::xcontract_manager_t::instance().register_address();

    // create all genesis block in one interface
    std::error_code ec;
    m_genesis_manager->init_genesis_block(ec);
    top::error::throw_error(ec);
}

void xtop_application::start() {
    // load configuration first
    auto loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(m_bus.get()), make_observer(m_logic_timer));
    config::xconfig_register_t::get_instance().add_loader(loader);
    config::xconfig_register_t::get_instance().load();

    base::xvblock_fork_t::instance().init(chain_fork::xutility_t::is_block_forked);

    m_txpool = xtxpool_v2::xtxpool_instance::create_xtxpool_inst(make_observer(m_blockstore), make_observer(m_cert_ptr), make_observer(m_bus));

    m_syncstore.attach(new store::xsyncvstore_t(*m_cert_ptr.get(), *m_blockstore.get()));
    contract::xcontract_manager_t::instance().init(m_syncstore);

    xthread_pool_t txpool_service_thp;
    txpool_service_thp.push_back(make_object_ptr<base::xiothread_t>());
    txpool_service_thp.push_back(make_object_ptr<base::xiothread_t>());
    m_thread_pools[xtop_thread_pool_type::txpool_service] = txpool_service_thp;
    xthread_pool_t statestore_thp;
    statestore_thp.push_back(make_object_ptr<base::xiothread_t>());
    statestore_thp.push_back(make_object_ptr<base::xiothread_t>());
    m_thread_pools[xtop_thread_pool_type::statestore] = statestore_thp;

    std::vector<observer_ptr<base::xiothread_t>> sync_account_thread_pool;
    for (uint32_t i = 0; i < 2; i++) {
        xobject_ptr_t<base::xiothread_t> thread = make_object_ptr<base::xiothread_t>();
        m_sync_account_thread_pool.push_back(thread);
        sync_account_thread_pool.push_back(make_observer(thread));
    }

    std::vector<observer_ptr<base::xiothread_t>> sync_handler_thread_pool;
    for (uint32_t i = 0; i < 2; i++) {
        xobject_ptr_t<base::xiothread_t> thread = make_object_ptr<base::xiothread_t>();
        m_sync_handler_thread_pool.push_back(thread);
        sync_handler_thread_pool.push_back(make_observer(thread));
    }

    // construct
    {
        m_elect_client_process = std::make_shared<elect::xelect_client_process>(
            m_network_id,
            make_observer(m_bus),
            std::bind(&xtop_application::on_election_data_updated, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            make_observer(m_logic_timer));

        m_election_cache_data_accessor = top::make_unique<election::cache::xdata_accessor_t>(m_network_id, make_observer(m_logic_timer));
        m_vhost = std::make_shared<vnetwork::xvhost_t>(
            make_observer(m_elect_main->GetEcVhost(m_network_id.value()).get()), make_observer(m_logic_timer), m_network_id, make_observer(m_election_cache_data_accessor));
        m_message_callback_hub = std::make_shared<vnetwork::xmessage_callback_hub_t>(make_observer(m_vhost));
        m_sync_obj = top::make_unique<sync::xsync_object_t>(
            make_observer(m_bus), make_observer(m_vhost), m_blockstore, m_cert_ptr, make_observer(m_sync_thread), sync_account_thread_pool, sync_handler_thread_pool);
        m_grpc_mgr = top::make_unique<grpcmgr::xgrpc_mgr_t>(make_observer(m_bus), make_observer(m_grpc_thread));

        m_txpool_service_mgr = xtxpool_service_v2::xtxpool_service_mgr_instance::create_xtxpool_service_mgr_inst(
            make_observer(m_blockstore), make_observer(m_txpool), m_thread_pools.at(xthread_pool_type_t::txpool_service), make_observer(m_bus), make_observer(m_logic_timer));
        xobject_ptr_t<base::xiothread_t> executor_thread = make_object_ptr<base::xiothread_t>();
        xobject_ptr_t<base::xiothread_t> syncer_thread = make_object_ptr<base::xiothread_t>();
        m_downloader = std::make_shared<state_sync::xstate_downloader_t>(
            base::xvchain_t::instance().get_xdbstore(), statestore::xstatestore_hub_t::instance(), make_observer(m_bus), executor_thread, syncer_thread);
        m_vnode_manager = std::make_shared<vnode::xvnode_manager_t>(make_observer(m_elect_main),
                                                                    make_observer(m_bus),
                                                                    make_observer(m_blockstore),
                                                                    make_observer(m_txstore),
                                                                    make_observer(m_logic_timer),
                                                                    make_observer(m_router),
                                                                    m_cert_ptr,
                                                                    make_observer(m_vhost),
                                                                    make_observer(m_sync_obj),
                                                                    make_observer(m_grpc_mgr),
                                                                    make_observer(m_txpool_service_mgr.get()),
                                                                    make_observer(m_txpool),
                                                                    make_observer(m_election_cache_data_accessor),
                                                                    make_observer(m_nodesvr_ptr),
                                                                    make_observer(m_downloader.get()));
    }

    for (auto & io_context_pool_info : m_io_context_pools) {
        auto & io_context_pool = top::get<xio_context_pool_t>(io_context_pool_info);
        for (auto & io_context : io_context_pool) {
            io_context->start();
        }
    }
    m_timer_driver->start();
    m_logic_timer->start();
    m_elect_main->start();

    // register node callback
    if (!m_elect_main->RegisterNodeCallback(std::bind(&top::application::xtop_application::handle_register_node, this, std::placeholders::_1, std::placeholders::_2))) {
        throw std::logic_error{"register node callback failed!"};
    }

    if (!m_elect_main->UpdateNodeSizeCallback(std::bind(&top::application::xtop_application::update_node_size, this, std::placeholders::_1, std::placeholders::_2))) {
        throw std::logic_error{"update node size callback failed!"};
    }

    contract::xcontract_manager_t::set_nodesrv_ptr(m_nodesvr_ptr);

    auto const last_logic_time = this->last_logic_time()->get_height();
    do {
        if (is_genesis_node()) {
            break;
        }

        auto const current_local_logic_time = config::gmttime_to_logic_time(base::xtime_utl::gmttime());
        bool const offline_too_long =
            current_local_logic_time < last_logic_time || (last_logic_time != 0 && (current_local_logic_time - last_logic_time >= 30));  // 5min = 300s = 30 logic time
        if (offline_too_long || !is_beacon_account()) {
            m_elect_client->bootstrap_node_join();
        }
    } while (false);

    // start
    {
        contract::xcontract_manager_t::instance().install_monitors(make_observer(m_bus), make_observer(m_message_callback_hub.get()), m_syncstore);
        load_last_election_data();

        m_txpool_service_mgr->start();
        m_vhost->start();
        m_message_callback_hub->start();
        m_vnode_manager->start();

        grpcmgr::grpc_init(m_blockstore.get(), m_sync_obj.get(), XGET_CONFIG(grpc_port));

        m_sync_obj->start();

        top_console_init();

        auto statestore_thp = m_thread_pools.at(xthread_pool_type_t::statestore);
        statestore::xstatestore_hub_t::instance()->start(statestore_thp[0], statestore_thp[1]);

        auto const & frozen_sharding_address = common::build_frozen_sharding_address(m_network_id);
        auto const zone_type = common::node_type_from(frozen_sharding_address.zone_id());

        data::election::xelection_result_store_t election_result_store{};
        auto & static_sync_group =
            election_result_store.result_of(m_network_id).result_of(zone_type).result_of(frozen_sharding_address.cluster_id()).result_of(frozen_sharding_address.group_id());
        static_sync_group.start_time(0);
        static_sync_group.group_version(common::xelection_round_t::max());

        auto & static_sync_node = static_sync_group.result_of(m_node_id);
        static_sync_node.joined_epoch(common::xelection_round_t::max());
        static_sync_node.stake(0);

        assert(static_sync_group.size() == 1);
        on_election_data_updated(election_result_store, frozen_sharding_address.zone_id(), 0);
    }

    m_logic_timer->update_time(last_logic_time, time::xlogic_timer_update_strategy_t::force);

    if (store::enable_block_recycler(false))
        xinfo("disable_block_recycler ok.");
    else
        xerror("disable_block_recycler fail");
}

void xtop_application::stop() {
    // stop
    {
        m_sync_obj->stop();
        m_vnode_manager->stop();
        m_message_callback_hub->stop();
        m_vhost->stop();
    }

    m_elect_main->stop();
    m_logic_timer->stop();
    m_timer_driver->stop();
    for (auto & io_context_pool_info : m_io_context_pools) {
        auto & io_context_pool = top::get<xio_context_pool_t>(io_context_pool_info);
        for (auto & io_context : io_context_pool) {
            io_context->stop();
        }
    }
}

void xtop_application::on_election_data_updated(data::election::xelection_result_store_t const & election_result_store,
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
            m_elect_main->GetElectManager()->OnElectQuit(xip);
        }
        for (const auto & xip : outdated_xips_pair.second) {
            // m_cons_mgr->destroy({xip.raw_low_part(), xip.raw_high_part()});
            m_txpool_service_mgr->destroy({xip.raw_low_part(), xip.raw_high_part()});
        }
        if (zid != common::xfrozen_zone_id) {
            m_elect_main->GetElectManager()->OnElectUpdated(election_result_store, zid, associated_blk_height);
        }
    } else {
        xinfo("[xchain_application] network %" PRIu32 " empty election data.  it may be from synchronization", static_cast<std::uint32_t>(m_network_id.value()));
    }
}

void xtop_application::load_last_election_data() {
    std::vector<common::xaccount_address_t> sys_addr{rec_elect_rec_contract_address,
                                                     rec_elect_archive_contract_address,
                                                     rec_elect_exchange_contract_address,
                                                     rec_elect_fullnode_contract_address,
                                                     rec_elect_edge_contract_address,
                                                     rec_elect_zec_contract_address,
                                                     zec_elect_consensus_contract_address,
                                                     zec_elect_eth_contract_address,
                                                     relay_make_block_contract_address};

    std::map<common::xaccount_address_t, common::xzone_id_t> addr_to_zone_id{{rec_elect_rec_contract_address, common::xcommittee_zone_id},
                                                                             {rec_elect_zec_contract_address, common::xzec_zone_id},
                                                                             {rec_elect_archive_contract_address, common::xstorage_zone_id},
                                                                             {rec_elect_exchange_contract_address, common::xstorage_zone_id},
                                                                             {rec_elect_fullnode_contract_address, common::xfullnode_zone_id},
                                                                             {rec_elect_edge_contract_address, common::xedge_zone_id},
                                                                             {zec_elect_consensus_contract_address, common::xdefault_zone_id},
                                                                             {zec_elect_eth_contract_address, common::xevm_zone_id},
                                                                             {relay_make_block_contract_address, common::xrelay_zone_id}};
    for (const auto & addr : sys_addr) {
        for (auto const & property : data::election::get_property_name_by_addr(addr)) {
            common::xzone_id_t zone_id = addr_to_zone_id[addr];
            using top::data::election::xelection_result_store_t;
            std::error_code ec;

            xwarn("xbeacon_chain_application::load_last_election_data begin. contract %s; property %s", addr.to_string().c_str(), property.c_str());
            xscope_executer_t loading_result_logger{
                [&ec, &addr, property] { xwarn("xbeacon_chain_application::load_last_election_data end. contract %s; property %s", addr.to_string().c_str(), property.c_str()); }};

            data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_change_state(addr);
            if (unitstate == nullptr) {
                xerror("xtop_application::load_last_election_data fail-get state.");
                top::error::throw_error(ec);
                continue;
            }
            std::string result;
            if (xsuccess != unitstate->string_get(property, result)) {
                xerror("xtop_application::load_last_election_data fail-get property.");
                top::error::throw_error(ec);
                continue;
            }

            uint64_t block_height = unitstate->height();
            auto const & last_election_result_store = codec::msgpack_decode<data::election::xelection_result_store_t>({std::begin(result), std::end(result)});
            xinfo("xbeacon_chain_application::load_last_election_data load block.addr=%s,height=%ld", addr.to_string().c_str(), block_height);

            if ((addr == rec_elect_rec_contract_address || addr == rec_elect_zec_contract_address || addr == zec_elect_consensus_contract_address ||
                 addr == zec_elect_eth_contract_address || addr == relay_make_block_contract_address) &&
                block_height != 0) {
                uint64_t prev_block_height = block_height - 1;
                data::xunitstate_ptr_t unitstate2 = statestore::xstatestore_hub_t::instance()->get_unit_committed_changed_state(addr, prev_block_height);
                if (unitstate == nullptr) {
                    xwarn("xtop_application::load_last_election_data fail-get state.");
                } else {
                    if (xsuccess != unitstate2->string_get(property, result)) {
                        xerror("xtop_application::load_last_election_data fail-get property.");
                        top::error::throw_error(ec);
                        continue;
                    }
                    auto const & before_last_election_result_store = codec::msgpack_decode<data::election::xelection_result_store_t>({std::begin(result), std::end(result)});
                    on_election_data_updated(before_last_election_result_store, zone_id, prev_block_height);  // TODO(jimmy) use state->get_block_height() ?
                }
            }
            on_election_data_updated(last_election_result_store, zone_id, block_height);
        }
    }
}

base::xauto_ptr<top::base::xvblock_t> xtop_application::last_logic_time() const {
    XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
    return m_blockstore->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr));
}

int32_t xtop_application::handle_register_node(std::string const & node_addr, std::string const & node_sign) {
#if !defined(XENABLE_MOCK_ZEC_STAKE)
    // filter seed node
    if (data::xrootblock_t::is_seed_node(node_addr)) {
        xinfo("[register_node_callback] success, seed node, node_addr: %s", node_addr.c_str());
        return store::xstore_success;
    }

    // check whether include in register contract
    std::string value_str;
    int ret = statestore::xstatestore_hub_t::instance()->map_get(rec_registration_contract_address, top::data::system_contract::XPORPERTY_CONTRACT_REG_KEY, node_addr, value_str);

    if (ret != store::xstore_success || value_str.empty()) {
        xwarn("[register_node_callback] get node register info fail, node_addr: %s", node_addr.c_str());
        return ret;
    }

    data::system_contract::xreg_node_info node_info;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());

    node_info.serialize_from(stream);

    // check mortgage
    uint64_t default_mortgage = node_info.get_required_min_deposit();
    if (node_info.m_account_mortgage < default_mortgage) {
        xwarn("[register_node_callback] get node register info fail, mortgage not enough; node_addr: %s mortgage: %PRIu64 min_need_mortgage: %PRIu64",
              node_addr.c_str(),
              node_info.m_account_mortgage,
              default_mortgage);
        return store::xaccount_property_mortgage_invalid;
    }

    // check node_sign to verify node
    utl::xkeyaddress_t xaddr{node_addr};
    uint256_t hash_value = utl::xsha2_256_t::digest(node_addr);
    XMETRICS_GAUGE(metrics::cpu_hash_256_handle_register_node_calc, 1);

    utl::xecdsasig_t sig{(const uint8_t *)node_sign.data()};

    auto reg_pub = base::xstring_utl::base64_decode(node_info.consensus_public_key.to_string());
    uint8_t out_publickey_data[utl::UNCOMPRESSED_PUBLICKEY_SIZE];
    memcpy(out_publickey_data, reg_pub.data(), (size_t)std::min(utl::UNCOMPRESSED_PUBLICKEY_SIZE, (int)reg_pub.size()));
    if (!utl::xsecp256k1_t::verify_signature(sig, hash_value, out_publickey_data, false)) {
        xinfo("[register_node_callback] verify node signature fail, node_addr: %s", node_addr.c_str());
        return store::xaccount_not_exist;
    }

#endif

    xinfo("[register_node_callback] success, node_addr: %s", node_addr.c_str());
    return store::xstore_success;
}

void xtop_application::update_node_size(uint64_t & node_size, std::error_code & ec) {
    assert(!ec);
#if defined(XBUILD_CI)
    node_size = 50;
#elif defined(XBUILD_DEV)
    node_size = 14;
#elif defined(XBUILD_GALILEO)
    node_size = 128;
#else  // mainnet
    node_size = 700;
#endif

    data::election::xstandby_result_store_t standby_result_store;
    std::string serialized_value{};
    if (statestore::xstatestore_hub_t::instance()->string_get(rec_standby_pool_contract_address, data::XPROPERTY_CONTRACT_STANDBYS_KEY, serialized_value) == 0 &&
        !serialized_value.empty()) {
        auto const & standby_result_store = codec::msgpack_decode<data::election::xstandby_result_store_t>({std::begin(serialized_value), std::end(serialized_value)});
        common::xnetwork_id_t network_id{top::config::to_chainid(XGET_CONFIG(chain_name))};
        auto const & standby_network_storage_result = standby_result_store.result_of(network_id);
        if (!standby_network_storage_result.empty()) {
            node_size = standby_network_storage_result.size();
            xinfo("[update_node_size] success, node_size: %llu", node_size);
            return;
        } else {
            ec = error::xerrc_t::load_standby_data_missing_property;
            xinfo("[update_node_size] failed, standby empty?");
            assert(false);
            return;
        }
    }
    ec = error::xerrc_t::load_standby_data_missing_block;
    xinfo("[update_node_size] failed, string get failed.");
    return;
}

bool xtop_application::is_genesis_node() const noexcept {
    const std::vector<data::node_info_t> & seeds = data::xrootblock_t::get_seed_nodes();
    auto const & user_params = data::xuser_params::get_instance();
    top::common::xnode_id_t node_id = user_params.account;

    for (auto const & item : seeds) {
        if (user_params.account == common::xnode_id_t{item.m_account}) {
            xinfo("xtop_application::is_genesis_node is genesis node %s", node_id.to_string().c_str());
            return true;
        }
    }

    xwarn("xtop_application::is_genesis_node not genesis node %s", node_id.to_string().c_str());
    return false;
}

bool xtop_application::is_beacon_account() const noexcept {
    try {
        auto const & user_params = data::xuser_params::get_instance();
        top::common::xnode_id_t node_id = top::common::xnode_id_t{user_params.account};

        std::string result;
        data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_change_state(rec_elect_rec_contract_address);
        if (unitstate == nullptr) {
            xerror("xtop_application::is_beacon_account fail-get state.");
            return false;
        }

        auto property_names = data::election::get_property_name_by_addr(rec_elect_rec_contract_address);
        common::xnetwork_id_t network_id{top::config::to_chainid(XGET_CONFIG(chain_name))};
        for (auto const & property : property_names) {
            result = unitstate->string_get(property);
            if (result.empty()) {
                xwarn("xtop_application::is_beacon_account no property %s", property.c_str());
                continue;
            }
            using top::data::election::xelection_result_store_t;
            auto const & election_result_store = codec::msgpack_decode<xelection_result_store_t>({std::begin(result), std::end(result)});
            auto & current_group_nodes = election_result_store.result_of(network_id)
                                             .result_of(common::xnode_type_t::committee)
                                             .result_of(common::xcommittee_cluster_id)
                                             .result_of(common::xcommittee_group_id);

            if (top::get<bool>(current_group_nodes.find(node_id))) {
                xinfo("xtop_application::is_beacon_account is genesis node. %s", node_id.to_string().c_str());
                return true;
            }
        }

        xwarn("xtop_application::is_beacon_account not genesis %s", node_id.to_string().c_str());
        return false;
    } catch (top::error::xtop_error_t const & eh) {
        std::cerr << "application start failed. exception: " << eh.what() << "; error: " << eh.code().message() << "; category: " << eh.code().category().name() << std::endl;
        xerror("xapplication_t::is_beacon_account failed with exception: %s; error: %s; category: %s", eh.what(), eh.code().message().c_str(), eh.code().category().name());
        return false;
    } catch (std::exception & eh) {
        std::cerr << "application start failed. exception: " << eh.what() << std::endl;
        xerror("xapplication_t::is_beacon_account failed with exception: %s", eh.what());
        return false;
    }
}

void xtop_application::top_console_init() {
    top::elect::MultilayerNetworkInterfacePtr net_module = std::make_shared<elect::MultilayerNetworkChainQuery>();
    ChainCommandsPtr chain_cmd{nullptr};
    chain_cmd = std::make_shared<ChainCommands>(net_module, m_sync_obj.get());
    top::ChainInfo::Instance()->SetChainCmd(chain_cmd);
    std::cout << "==== start chaininfo center ===\n";
    xinfo("==== start chaininfo center ===");
}

NS_END2

#if !defined XCXX14_OR_ABOVE

NS_BEG1(std)
std::size_t hash<top::application::xthread_pool_type_t>::operator()(top::application::xthread_pool_type_t const type) const noexcept {
    return static_cast<std::size_t>(type);
};

std::size_t hash<top::application::xio_context_type_t>::operator()(top::application::xio_context_type_t const type) const noexcept {
    return static_cast<std::size_t>(type);
};

NS_END1
#endif