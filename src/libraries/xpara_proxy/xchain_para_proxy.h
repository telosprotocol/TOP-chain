// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xpara_proxy/xpara_proxy_face.h"
#include "xpara_proxy/xpara_proxy_fwd.h"
#include "xbasic/xcrypto_key.h"
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xip.h"
#include "xcommon/xnode_id.h"
#include "xdata/xelection/xelection_data_struct.h"
#include "xelect/client/xelect_client_process.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xgrpc_mgr/xgrpc_mgr.h"
#include "xmbus/xmessage_bus.h"
#include "xstore/xstore_face.h"
#include "xsync/xsync_object.h"
#include "xunit_service/xcons_face.h"
#include "xvnetwork/xmessage_callback_hub.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnode/xvnode_manager_face.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"

#include <memory>

NS_BEG2(top, para_proxy)

class xtop_chain_para_proxy : public xpara_proxy_face_t<xtop_chain_para_proxy> {
private:
    using xbase_t = xpara_proxy_face_t<xtop_chain_para_proxy>;

protected:
    observer_ptr<xpara_proxy_t> m_para_proxy;  // top most para_proxy object. used to query node_id / public key etc....
    common::xnetwork_id_t        m_network_id;

    std::shared_ptr<elect::xelect_client_process>               m_elect_client_process;  // should be xobject_ptr_t
    std::unique_ptr<election::cache::xdata_accessor_face_t>     m_election_cache_data_accessor;
    std::shared_ptr<vnetwork::xvhost_face_t>                    m_vhost;
    std::shared_ptr<vnetwork::xmessage_callback_hub_t>          m_message_callback_hub;
    std::unique_ptr<sync::xsync_object_t>                       m_sync_obj;
    std::unique_ptr<grpcmgr::xgrpc_mgr_t>                       m_grpc_mgr;
    xunit_service::xcons_service_mgr_ptr                        m_cons_mgr{};
    std::shared_ptr<xtxpool_service_v2::xtxpool_service_mgr_face>  m_txpool_service_mgr;
    std::shared_ptr<vnode::xvnode_manager_face_t>               m_vnode_manager;

public:
    xtop_chain_para_proxy(xtop_chain_para_proxy const &) = delete;
    xtop_chain_para_proxy & operator=(xtop_chain_para_proxy const &) = delete;
    xtop_chain_para_proxy(xtop_chain_para_proxy &&) = default;
    xtop_chain_para_proxy & operator=(xtop_chain_para_proxy &&) = default;
    ~xtop_chain_para_proxy() override = default;

    xtop_chain_para_proxy(observer_ptr<xpara_proxy_t> const &                 para_proxy,
                           common::xnetwork_id_t const &                        network_id,
                           xobject_ptr_t<base::xvblockstore_t> &blockstore,
                           xobject_ptr_t<base::xvnodesrv_t> &nodesvr_ptr,
                           xobject_ptr_t<base::xvcertauth_t> &cert_ptr,
                           observer_ptr<base::xiothread_t> const &              grpc_thread,
                           observer_ptr<base::xiothread_t> const &              sync_thread,
                           std::vector<observer_ptr<base::xiothread_t>> const & sync_account_thread_pool,
                           std::vector<observer_ptr<base::xiothread_t>> const & sync_handler_thread_pool);

    void start() override;

    void stop() override;

    common::xnode_id_t const & node_id() const noexcept;

    xpublic_key_t const & public_key() const noexcept;

    std::string const & sign_key() const noexcept;

protected:
    virtual void load_last_election_data() = 0;  // should be a virtual member function? beacon chain & top chain?

    void on_election_data_updated(data::election::xelection_result_store_t const & election_result_store,
                                  common::xzone_id_t const & zid,
                                  std::uint64_t const associated_blk_height);

    virtual void top_grpc_init(uint16_t const grpc_port);
    virtual void top_console_init();

    virtual bool enable_grpc_service() const noexcept { return true; }
};
using xchain_para_proxy_t = xtop_chain_para_proxy;

NS_END2
