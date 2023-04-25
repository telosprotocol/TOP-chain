// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xcrypto_key.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"
#include "xbasic/xtimer_driver_fwd.h"
#include "xblockstore/xsyncvstore_face.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xnode_id.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdbstore/xstore_face.h"
#include "xelect/client/xelect_client_process.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xgenesis/xgenesis_manager_fwd.h"
#include "xmbus/xmessage_bus_face.h"
#include "xrouter/xrouter_face.h"
#include "xstate_sync/xstate_downloader.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvcertauth.h"
#include "xvledger/xvcnode.h"
#include "xvledger/xvtxstore.h"
#include "xvnetwork/xmessage_callback_hub.h"
#include "xvnetwork/xvhost_face_fwd.h"
#include "xvnode/xvnode_manager_face.h"

#include <memory>
#include <string>
#include <unordered_map>

/// ==== begin fwd

NS_BEG2(top, grpcmgr)
class xgrpc_mgr_t;
NS_END2

NS_BEG2(top, sync)
class xtop_sync_object;
using xsync_object_t = xtop_sync_object;
NS_END2

NS_BEG2(top, elect)
class xelect_client_imp;

// todo totally remove file elect_main.h
class MultilayerNetwork;
using ElectMain = MultilayerNetwork;
NS_END2

/// ===== end fwd

NS_BEG2(top, application)

enum class xtop_thread_pool_type : std::uint8_t { invalid, unit_service, synchronization, txpool_service, statestore };
using xthread_pool_type_t = xtop_thread_pool_type;

enum class xtop_io_context_type : uint8_t { invalid, general };
using xio_context_type_t = xtop_io_context_type;

NS_END2

NS_BEG1(std)

#if !defined(XCXX14)

template <>
struct hash<top::application::xthread_pool_type_t> final {
    std::size_t operator()(top::application::xthread_pool_type_t const type) const noexcept;
};

template <>
struct hash<top::application::xio_context_type_t> final {
    std::size_t operator()(top::application::xio_context_type_t const type) const noexcept;
};

#endif

NS_END1


NS_BEG2(top, application)

class xtop_application final : public xtrival_runnable_t<xtop_application> {
public:
    using xthread_pool_t = std::vector<xobject_ptr_t<base::xiothread_t>>;
    using xio_context_pool_t = std::vector<std::shared_ptr<xbase_io_context_wrapper_t>>;

private:
    common::xnode_id_t m_node_id;
    xpublic_key_t m_public_key;

    common::xnetwork_id_t m_network_id;

    std::unordered_map<xio_context_type_t, xio_context_pool_t> m_io_context_pools;
    std::shared_ptr<xbase_timer_driver_t> m_timer_driver;
    std::unique_ptr<elect::ElectMain> m_elect_main;
    std::unique_ptr<router::xrouter_face_t> m_router;
    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<time::xchain_time_face_t> m_logic_timer;
    // xobject_ptr_t<base::xiothread_t> m_grpc_thread{};
    xobject_ptr_t<base::xiothread_t> m_sync_thread{};
    std::unique_ptr<elect::xelect_client_imp> m_elect_client;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<base::xvtxstore_t> m_txstore;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr_ptr;
    xobject_ptr_t<base::xvcertauth_t> m_cert_ptr;
    std::unique_ptr<genesis::xgenesis_manager_t> m_genesis_manager;

    xobject_ptr_t<xtxpool_v2::xtxpool_face_t> m_txpool;
    xobject_ptr_t<store::xsyncvstore_t> m_syncstore;
    std::unordered_map<xthread_pool_type_t, xthread_pool_t> m_thread_pools;
    std::vector<xobject_ptr_t<base::xiothread_t>> m_sync_account_thread_pool{};
    std::vector<xobject_ptr_t<base::xiothread_t>> m_sync_handler_thread_pool{};

    std::shared_ptr<elect::xelect_client_process> m_elect_client_process;  // should be xobject_ptr_t
    std::unique_ptr<election::cache::xdata_accessor_face_t> m_election_cache_data_accessor;
    std::shared_ptr<vnetwork::xvhost_face_t> m_vhost;
    std::shared_ptr<vnetwork::xmessage_callback_hub_t> m_message_callback_hub;
    std::unique_ptr<sync::xsync_object_t> m_sync_obj;
    // std::unique_ptr<grpcmgr::xgrpc_mgr_t> m_grpc_mgr;
    std::shared_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> m_txpool_service_mgr;
    std::shared_ptr<state_sync::xstate_downloader_t> m_downloader;
    std::shared_ptr<vnode::xvnode_manager_face_t> m_vnode_manager;

public:
    xtop_application(xtop_application const &) = delete;
    xtop_application & operator=(xtop_application const &) = delete;
    xtop_application(xtop_application &&) = default;
    xtop_application & operator=(xtop_application &&) = default;
    ~xtop_application() = default;

    xtop_application(common::xnode_id_t const & node_id, xpublic_key_t const & public_key, std::string && sign_key);

public:
    void start() override;

    void stop() override;

public:
private:
    void on_election_data_updated(data::election::xelection_result_store_t const & election_result_store,
                                  common::xzone_id_t const & zid,
                                  std::uint64_t const associated_blk_height);

    void load_last_election_data();

    base::xauto_ptr<::top::base::xvblock_t> last_logic_time() const;

    int32_t handle_register_node(std::string const & node_addr, std::string const & node_sign);

    void update_node_size(uint64_t & node_size, std::error_code & ec);

    bool is_genesis_node() const noexcept;

    bool is_beacon_account() const noexcept;

    void top_console_init();
};
using xapplication_t = xtop_application;

NS_END2
