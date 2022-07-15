// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xmbus/xmessage_bus.h"
#include "xunit_service/xcons_face.h"
#include "xunit_service/xcons_utl.h"

#include <map>
#include <mutex>
#include <vector>

NS_BEG2(top, xunit_service)

// consensus service manager

class xdispatcher_builder : public xunit_service::xcons_dispatcher_builder_face {
public:
    xunit_service::xcons_dispatcher_ptr build(observer_ptr<mbus::xmessage_bus_face_t> const & mb,
                                              std::shared_ptr<xunit_service::xcons_service_para_face> const &,
                                              xunit_service::e_cons_type cons_type) override;
};

xcons_service_mgr_ptr xcons_mgr_build(std::string const & node_account,
                                      observer_ptr<store::xstore_face_t> const & store,
                                      observer_ptr<base::xvblockstore_t> const & blockstore,
                                      observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                                      observer_ptr<time::xchain_time_face_t> const & tx_timer,
                                      xobject_ptr_t<base::xvcertauth_t> const & certauth,
                                      observer_ptr<election::cache::xdata_accessor_face_t> const & accessor,
                                      observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
                                      observer_ptr<router::xrouter_face_t> const & router);

class xcons_service_mgr
  : public xcons_service_mgr_face
  , public std::enable_shared_from_this<xcons_service_mgr> {
    using xcons_services = std::vector<std::shared_ptr<xcons_service_face>>;

public:
    xcons_service_mgr(observer_ptr<mbus::xmessage_bus_face_t> const & mb,
                      const std::shared_ptr<xnetwork_proxy_face> & network_proxy,
                      const xcons_dispatcher_builder_ptr & dispatcher_builder,
                      const std::shared_ptr<xcons_service_para_face> & para);
    virtual ~xcons_service_mgr();

public:
    // create consensus proxy by networkdriver
    virtual void create(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & network);

    // destroy useless cons services by networkdriver
    virtual bool destroy(const xvip2_t & xip);

    // init reference data
    virtual bool start(const xvip2_t & xip, const common::xlogic_time_t & start_time);

    virtual bool fade(const xvip2_t & xip);

    // uninit reference data
    virtual bool unreg(const xvip2_t & xip);

protected:
    bool find(const xvip2_t & xip, std::vector<std::shared_ptr<xcons_service_face>> * p_services);

protected:
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    // std::map<e_cons_type, xcons_dispatcher_ptr>     m_dispatchers;
    xcons_dispatcher_builder_ptr m_dispachter_builder;
    std::map<xvip2_t, xcons_services, xvip2_compare> m_cons_map;
    std::shared_ptr<xnetwork_proxy_face> m_network_proxy;
    std::shared_ptr<xcons_service_para_face> m_para;
    std::mutex m_mutex;
};

NS_END2
