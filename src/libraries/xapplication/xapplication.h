// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xapplication/xapplication_face.h"
#include "xapplication/xapplication_fwd.h"
#include "xapplication/xchain_application.h"
#include "xbasic/xcrypto_key.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xip.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xnode_info.h"
#include "xconfig/xconfig_register.h"
#include "xelect/client/xelect_client.h"
#include "xelect_net/include/elect_main.h"
#include "xmbus/xmessage_bus.h"
#include "xnetwork/xnetwork_driver_face.h"
#include "xrouter/xrouter_face.h"
#include "xstore/xstore_face.h"
#include "xsync/xsync_object.h"
#include "xtxpool_v2/xtxpool_face.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

NS_BEG2(top, application)

enum class xtop_thread_pool_type : std::uint8_t { invalid, unit_service, synchronization, txpool_service };
using xthread_pool_type_t = xtop_thread_pool_type;

enum class xtop_io_context_type : uint8_t { invalid, general };
using xio_context_type_t = xtop_io_context_type;

NS_END2

#if !defined XCXX14_OR_ABOVE

NS_BEG1(std)

template <>
struct hash<top::application::xthread_pool_type_t> final {
    std::size_t operator()(top::application::xthread_pool_type_t const type) const noexcept;
};

template <>
struct hash<top::application::xio_context_type_t> final {
    std::size_t operator()(top::application::xio_context_type_t const type) const noexcept;
};

NS_END1

#endif

NS_BEG2(top, application)

class xtop_application final : public xapplication_face_t<xtop_application> {
public:
    using xthread_pool_t = std::vector<xobject_ptr_t<base::xiothread_t>>;
    using xio_context_pool_t = std::vector<std::shared_ptr<xbase_io_context_wrapper_t>>;

private:
    using xbase_t = xapplication_face_t<xtop_application>;

    common::xnode_id_t m_node_id;
    xpublic_key_t m_public_key;
    std::string m_sign_key;

    std::unordered_map<xio_context_type_t, xio_context_pool_t> m_io_context_pools;

    std::shared_ptr<xbase_timer_driver_t> m_timer_driver;
    std::unique_ptr<elect::ElectMain> m_elect_main;

    std::unique_ptr<router::xrouter_face_t> m_router;
    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<time::xchain_time_face_t> m_logic_timer;
    xobject_ptr_t<base::xiothread_t> m_grpc_thread{};
    xobject_ptr_t<base::xiothread_t> m_sync_thread{};
    std::vector<xobject_ptr_t<base::xiothread_t>> m_sync_account_thread_pool{};
    std::vector<xobject_ptr_t<base::xiothread_t>> m_sync_handler_thread_pool{};
    std::unique_ptr<elect::xelect_client_imp> m_elect_client;
    xobject_ptr_t<xtxpool_v2::xtxpool_face_t> m_txpool;
    std::unordered_map<xthread_pool_type_t, xthread_pool_t> m_thread_pools;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr_ptr;
    xobject_ptr_t<base::xvcertauth_t> m_cert_ptr;
    xobject_ptr_t<store::xsyncvstore_t> m_syncstore;
    std::vector<std::unique_ptr<xchain_application_t>> m_chain_applications{};

public:
    xtop_application(xtop_application const &) = delete;
    xtop_application & operator=(xtop_application const &) = delete;
    xtop_application(xtop_application &&) = default;
    xtop_application & operator=(xtop_application &&) = default;
    ~xtop_application() override = default;

    xtop_application(common::xnode_id_t const & node_id, xpublic_key_t const & public_key, std::string const & sign_key);

    void start() override;

    void stop() override;

    common::xnode_id_t const & node_id() const noexcept;

    xpublic_key_t const & public_key() const noexcept;

    std::string const & sign_key() const noexcept;

    observer_ptr<xbase_timer_driver_t> timer_driver() const noexcept;

    observer_ptr<network::xnetwork_driver_face_t> network_driver(common::xnetwork_id_t const & network_id) const noexcept;

    observer_ptr<time::xchain_time_face_t> logic_timer() const noexcept;

    observer_ptr<mbus::xmessage_bus_face_t> message_bus() const noexcept;

    observer_ptr<store::xstore_face_t> store() const noexcept;

    observer_ptr<base::xvblockstore_t> blockstore() const noexcept;

    observer_ptr<router::xrouter_face_t> router() const noexcept;

    observer_ptr<base::xiothread_t> thread(std::size_t const index) const noexcept;

    std::shared_ptr<top::elect::ElectManager> elect_manager() const noexcept;

    observer_ptr<elect::ElectMain> elect_main() const noexcept;

    xthread_pool_t const & thread_pool(xthread_pool_type_t const thread_pool_type) const noexcept;

    observer_ptr<xtxpool_v2::xtxpool_face_t> txpool() const noexcept;

    xobject_ptr_t<base::xvnodesrv_t> node_service() const noexcept;

    xobject_ptr_t<base::xvcertauth_t> cert_serivce() const noexcept;

    xobject_ptr_t<store::xsyncvstore_t> syncstore() const noexcept;

private:
    base::xauto_ptr<top::base::xvblock_t> last_logic_time() const;

    bool check_rootblock();
    bool create_genesis_accounts();

    bool create_genesis_account(std::string const & address, uint64_t const init_balance);

    int32_t handle_register_node(std::string const & node_addr, std::string const & node_sign);

    void create_thread_pools();

    bool is_genesis_node() const noexcept;

    bool is_beacon_account() const noexcept;
};
using xtop_application_t = xtop_application;

NS_END2
