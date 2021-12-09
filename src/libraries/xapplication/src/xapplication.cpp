// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xapplication/xapplication.h"

#include "xapplication/xbeacon_chain_application.h"
// #include "xapplication/xcons_mgr_builder.h"
#include "xapplication/xtop_chain_application.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xutility.h"
#include "xblockstore/xblockstore_face.h"
#include "xcertauth/xcertauth_face.h"
#include "xchain_timer/xchain_timer.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xutility.h"
#include "xdata/xblocktool.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xelect_transaction.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xrootblock.h"
#include "xdb/xdb_factory.h"
#include "xelection/xvnode_house.h"
#include "xloader/xconfig_genesis_loader.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xrouter/xrouter.h"
#include "xstake/xstake_algorithm.h"
#include "xstore/xstore_error.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"

#include <stdexcept>

NS_BEG2(top, application)

xtop_application::xtop_application(common::xnode_id_t const & node_id, xpublic_key_t const & public_key, std::string const & sign_key)
  : m_node_id{node_id}
  , m_public_key{public_key}
  , m_sign_key(sign_key)
  , m_io_context_pools{{xio_context_type_t::general, {std::make_shared<xbase_io_context_wrapper_t>()}}}
  , m_timer_driver{std::make_shared<xbase_timer_driver_t>(m_io_context_pools[xio_context_type_t::general].front())}
  , m_elect_main{top::make_unique<elect::ElectMain>(node_id, std::set<uint32_t>({static_cast<uint32_t>(top::config::to_chainid(XGET_CONFIG(chain_name)))}))}
  , m_router{top::make_unique<router::xrouter_t>()}
  , m_bus{top::make_object_ptr<mbus::xmessage_bus_t>(true, 1000)}
  , m_logic_timer{make_object_ptr<time::xchain_timer_t>(m_timer_driver)}
  , m_grpc_thread{make_object_ptr<base::xiothread_t>()}
  , m_sync_thread{make_object_ptr<base::xiothread_t>()}
  , m_elect_client{top::make_unique<elect::xelect_client_imp>()} {
    std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::instance(XGET_CONFIG(db_path));
    m_store = store::xstore_factory::create_store_with_static_kvdb(db);
    base::xvchain_t::instance().set_xdbstore(m_store.get());
    base::xvchain_t::instance().set_xevmbus(m_bus.get());
    m_blockstore.attach(store::get_vblockstore());
    m_txstore = xobject_ptr_t<base::xvtxstore_t>(
        txstore::create_txstore(top::make_observer<mbus::xmessage_bus_face_t>(m_bus.get()), 
                                top::make_observer<xbase_timer_driver_t>(m_timer_driver)));
    base::xvchain_t::instance().set_xtxstore(m_txstore.get());

    m_nodesvr_ptr = make_object_ptr<election::xvnode_house_t>(node_id, sign_key, m_blockstore, make_observer(m_bus.get()));
#ifdef MOCK_CA
    m_cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
#else
    m_cert_ptr.attach(&auth::xauthcontext_t::instance(*m_nodesvr_ptr.get()));
#endif
    // genesis blocks should init imediately after db created
    if ((m_store == nullptr) || !m_store->open()) {
        xwarn("xtop_application::start db open failed!");
        exit(0);
    }

    if (!check_rootblock()) {
        throw std::logic_error{"creating rootblock failed"};
    }

    if (!create_genesis_accounts()) {
        throw std::logic_error{"creating genesis accounts failed"};
    }

    contract::xcontract_deploy_t::instance().deploy_sys_contracts();
    contract::xcontract_manager_t::instance().instantiate_sys_contracts();
    contract::xcontract_manager_t::instance().setup_blockchains(m_blockstore.get());
}

void xtop_application::start() {
    chain_data::xchain_data_processor_t::release();
    // load configuration first
    auto loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(m_store), make_observer(m_bus.get()), make_observer(m_logic_timer));
    config::xconfig_register_t::get_instance().add_loader(loader);
    config::xconfig_register_t::get_instance().load();

    chain_fork::xtop_chain_fork_config_center::init();
    base::xvblock_fork_t::instance().init(chain_fork::xtop_chain_fork_config_center::is_block_forked);

    m_txpool = xtxpool_v2::xtxpool_instance::create_xtxpool_inst(make_observer(m_store), make_observer(m_blockstore.get()), make_observer(m_cert_ptr.get()), make_observer(m_bus.get()));

    m_syncstore.attach(new store::xsyncvstore_t(*m_cert_ptr.get(), *m_blockstore.get()));
    contract::xcontract_manager_t::instance().init(make_observer(m_store), m_syncstore);

    xthread_pool_t txpool_service_thp;
    txpool_service_thp.push_back(make_object_ptr<base::xiothread_t>());
    txpool_service_thp.push_back(make_object_ptr<base::xiothread_t>());
    m_thread_pools[xtop_thread_pool_type::txpool_service] = txpool_service_thp;

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

    m_chain_applications.push_back(
        top::make_unique<xbeacon_chain_application_t>(make_observer(this), m_blockstore, m_nodesvr_ptr, m_cert_ptr, make_observer(m_grpc_thread), make_observer(m_sync_thread), sync_account_thread_pool, sync_handler_thread_pool));

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
    if (m_elect_main->RegisterNodeCallback(std::bind(&top::application::xtop_application::handle_register_node, this, std::placeholders::_1, std::placeholders::_2))) {
        throw std::logic_error{"register node callback failed!"};
    }

    contract::xcontract_manager_t::set_nodesrv_ptr(node_service());

    auto const last_logic_time = this->last_logic_time()->get_height();
    do {
        if (is_genesis_node()) {
            break;
        }

        auto const current_local_logic_time = config::gmttime_to_logic_time(base::xtime_utl::gmttime());
        bool const offline_too_long = current_local_logic_time < last_logic_time || (last_logic_time != 0 && (current_local_logic_time - last_logic_time >= 30)); // 5min = 300s = 30 logic time
        if (offline_too_long || !is_beacon_account()) {
            m_elect_client->bootstrap_node_join();
        }
    } while (false);

    for (auto i = 0u; i < m_chain_applications.size(); ++i) {
        auto const & chain_app = m_chain_applications[i];
        chain_app->start();
    }

    m_logic_timer->update_time(last_logic_time, time::xlogic_timer_update_strategy_t::force);
}

void xtop_application::stop() {
    for (auto i = m_chain_applications.size(); i > 0; --i) {
        auto const & chain_app = m_chain_applications[i - 1];
        chain_app->stop();
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

common::xnode_id_t const & xtop_application::node_id() const noexcept {
    return m_node_id;
}

xpublic_key_t const & xtop_application::public_key() const noexcept {
    return m_public_key;
}

std::string const & xtop_application::sign_key() const noexcept {
    return m_sign_key;
}

observer_ptr<xbase_timer_driver_t> xtop_application::timer_driver() const noexcept {
    return make_observer(m_timer_driver.get());
}

observer_ptr<elect::xnetwork_driver_face_t> xtop_application::network_driver(common::xnetwork_id_t const & network_id) const noexcept {
    return make_observer(m_elect_main->GetEcVhost(network_id.value()).get());
}

std::shared_ptr<top::elect::ElectManager> xtop_application::elect_manager() const noexcept {
    return m_elect_main->GetElectManager();
}

observer_ptr<elect::ElectMain> xtop_application::elect_main() const noexcept {
    return make_observer(m_elect_main.get());
}

observer_ptr<time::xchain_time_face_t> xtop_application::logic_timer() const noexcept {
    assert(m_logic_timer != nullptr);
    return make_observer(m_logic_timer.get());
}

observer_ptr<mbus::xmessage_bus_face_t> xtop_application::message_bus() const noexcept {
    return make_observer(m_bus.get());
}

observer_ptr<store::xstore_face_t> xtop_application::store() const noexcept {
    return make_observer(m_store.get());
}

observer_ptr<base::xvblockstore_t> xtop_application::blockstore() const noexcept {
    return make_observer(m_blockstore.get());
}

observer_ptr<base::xvtxstore_t> xtop_application::txstore() const noexcept {
    return make_observer(m_txstore.get());
}

observer_ptr<router::xrouter_face_t> xtop_application::router() const noexcept {
    return make_observer(m_router.get());
}

xtop_application::xthread_pool_t const & xtop_application::thread_pool(xthread_pool_type_t const thread_pool_type) const noexcept {
    assert(thread_pool_type == xthread_pool_type_t::synchronization || thread_pool_type == xthread_pool_type_t::unit_service || thread_pool_type == xthread_pool_type_t::txpool_service);

    return m_thread_pools.at(thread_pool_type);
}

observer_ptr<xtxpool_v2::xtxpool_face_t> xtop_application::txpool() const noexcept {
    return make_observer(m_txpool.get());
}

xobject_ptr_t<base::xvnodesrv_t> xtop_application::node_service() const noexcept {
    return m_nodesvr_ptr;
}

xobject_ptr_t<base::xvcertauth_t> xtop_application::cert_serivce() const noexcept {
    return m_cert_ptr;
}

xobject_ptr_t<store::xsyncvstore_t> xtop_application::syncstore() const noexcept {
    return m_syncstore;
}

base::xauto_ptr<top::base::xvblock_t> xtop_application::last_logic_time() const {
    XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
    return blockstore()->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr));
}

bool xtop_application::check_rootblock() {
    base::xvblock_t* rootblock = xrootblock_t::get_rootblock();

    if (true == blockstore()->exist_genesis_block(base::xvaccount_t(rootblock->get_account()))) {
        return true;
    }

    if (false == blockstore()->store_block(base::xvaccount_t(rootblock->get_account()), rootblock)) {
        xerror("xtop_application::check_rootblock rootblock set db fail");
        return false;
    }

    xinfo("xtop_application::check_rootblock success");
    return true;
}

bool xtop_application::preprocess_accounts_data() {
    if (chain_data::xtop_chain_data_processor::check_state()) {
        return true;
    }
    std::vector<chain_data::data_processor_t> user_data;
    chain_data::xchain_data_processor_t::get_all_user_data(user_data);
    for (auto const & data : user_data) {
        if (!create_genesis_account(data.address, data)) {
            xassert(0);
            return false;
        }
    }
    if (chain_data::xtop_chain_data_processor::set_state()) {
        return true;
    }
    return false;
}

bool xtop_application::create_genesis_accounts() {
    if (!preprocess_accounts_data()) {
        xwarn("xtop_application::create_genesis_accounts preprocess_accounts_data failed");
        return false;
    }
    std::map<std::string, uint64_t> genesis_accounts = xrootblock_t::get_all_genesis_accounts();
    for (auto const & pair : genesis_accounts) {
        common::xaccount_address_t account_address{pair.first};
        if (m_blockstore->exist_genesis_block(account_address.value())) {
            xdbg("xtop_contract_manager::setup_chain blockchain account %s genesis block exist", account_address.c_str());
            continue;
        }
        if (!create_genesis_account(pair.first, pair.second)) {
            xassert(0);
            return false;
        }
    }

    xinfo("xtop_application::create_genesis_accounts success");
    return true;
}

bool xtop_application::create_genesis_account(std::string const & address, uint64_t const init_balance) {
    xdbg("xtop_application::create_genesis_account address=%s balance=%ld", address.c_str(), init_balance);
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
    xassert(genesis_block != nullptr);
    base::xvaccount_t _vaddr(address);
    // m_blockstore->delete_block(_vaddr, genesis_block.get());  // delete default genesis block
    auto ret = m_blockstore->store_block(_vaddr, genesis_block.get());
    if (!ret) {
        xerror("xtop_application::create_genesis_account store genesis block fail");
        return false;
    }
    return true;
}

bool xtop_application::create_genesis_account(std::string const & address, chain_data::data_processor_t const & data) {
    xdbg("xtop_application::create_genesis_account address=%s balance=%ld burn_balance=%ld tgas_balance=%ld vote_balance=%ld lock_balance=%ld lock_tgas=%ld unvote_num=%ld expire_vote=%ld create_time=%ld lock_token=%ld pledge_vote_str_cnt=%ld",
         address.c_str(),
         data.top_balance,
         data.burn_balance,
         data.tgas_balance,
         data.vote_balance,
         data.lock_balance,
         data.lock_tgas,
         data.unvote_num,
         data.expire_vote,
         data.create_time,
         data.lock_token,
         data.pledge_vote.size());
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(address, data);
    xassert(genesis_block != nullptr);
    base::xvaccount_t _vaddr(address);
    // m_blockstore->delete_block(_vaddr, genesis_block.get());  // delete default genesis block
    auto ret = m_blockstore->store_block(_vaddr, genesis_block.get());
    if (!ret) {
        xerror("xtop_application::create_genesis_account store genesis block fail");
        return false;
    }
    return true;
}

int32_t xtop_application::handle_register_node(std::string const & node_addr, std::string const & node_sign) {
#ifndef XENABLE_MOCK_ZEC_STAKE
    // filter seed node
    if (xrootblock_t::is_seed_node(node_addr)) {
        xinfo("[register_node_callback] success, seed node, node_addr: %s", node_addr.c_str());
        return store::xstore_success;
    }

    // check whether include in register contract
    std::string value_str;
    int ret = m_store->map_get(sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_REG_KEY, node_addr, value_str);

    if (ret != store::xstore_success || value_str.empty()) {
        xwarn("[register_node_callback] get node register info fail, node_addr: %s", node_addr.c_str());
        return ret;
    }

    xstake::xreg_node_info node_info;
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
    uint256_t          hash_value = utl::xsha2_256_t::digest(node_addr);
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

void xtop_application::create_thread_pools() {}

bool xtop_application::is_genesis_node() const noexcept {
    const std::vector<node_info_t> & seeds = data::xrootblock_t::get_seed_nodes();
    auto const & user_params = data::xuser_params::get_instance();
    top::common::xnode_id_t node_id = user_params.account;

    for (auto const & item : seeds) {
        if (user_params.account == common::xnode_id_t{item.m_account}) {
            xinfo("xtop_application::is_genesis_node is genesis node %s", node_id.c_str());
            return true;
        }
    }

    xwarn("xtop_application::is_genesis_node not genesis node %s", node_id.c_str());
    return false;
}

bool xtop_application::is_beacon_account() const noexcept {
    try {
        auto const & user_params = data::xuser_params::get_instance();
        top::common::xnode_id_t node_id = top::common::xnode_id_t{ user_params.account };

        std::string result;
        auto latest_vblock = data::xblocktool_t::get_latest_connectted_state_changed_block(m_blockstore.get(), xvaccount_t{sys_contract_rec_elect_rec_addr});
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
        if (bstate == nullptr) {
            xerror("xtop_application::is_beacon_account fail-get state.");
            return false;
        }
        xunit_bstate_t unitstate(bstate.get());

        auto property_names = data::election::get_property_name_by_addr(common::xaccount_address_t{ sys_contract_rec_elect_rec_addr });
        common::xnetwork_id_t network_id{ top::config::to_chainid(XGET_CONFIG(chain_name)) };
        for (auto const & property : property_names) {
            if (false == unitstate.string_get(property, result) || result.empty()) {
                xwarn("xtop_application::is_beacon_account no property %s", property.c_str());
                continue;
            }
            using top::data::election::xelection_result_store_t;
            auto const & election_result_store = codec::msgpack_decode<xelection_result_store_t>({ std::begin(result), std::end(result) });
            auto & current_group_nodes = election_result_store.result_of(network_id)
                                                              .result_of(common::xnode_type_t::committee)
                                                              .result_of(common::xcommittee_cluster_id)
                                                              .result_of(common::xcommittee_group_id);

            if (top::get<bool>(current_group_nodes.find(node_id))) {
                xinfo("xtop_application::is_beacon_account is genesis node. %s", node_id.c_str());
                return true;
            }
        }

        xwarn("xtop_application::is_beacon_account not genesis %s", node_id.c_str());
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
