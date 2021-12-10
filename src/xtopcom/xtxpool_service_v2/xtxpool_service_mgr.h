// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xbase/xobject.h"
#include "xbase/xthread.h"
#include "xtxpool_service_v2/xcons_utl.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xtxpool_service_v2/xtxpool_svc_para.h"
#include "xtxpool_v2/xtxpool_face.h"

#include <map>

NS_BEG2(top, xtxpool_service_v2)

class xtxpool_service_timer_t;
class xtxpool_service_dispatcher_imp_t;

// txpool proxy, bridge for txpool and txpool service
class xtxpool_service_mgr
  : public xtxpool_service_mgr_face
  , public std::enable_shared_from_this<xtxpool_service_mgr> {
public:
    explicit xtxpool_service_mgr(const observer_ptr<store::xstore_face_t> & store,
                                 const observer_ptr<base::xvblockstore_t> & blockstore,
                                 const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                 const observer_ptr<base::xiothread_t> & iothread_timer,
                                 const observer_ptr<base::xiothread_t> & iothread_dispather,
                                 const observer_ptr<mbus::xmessage_bus_face_t> & mbus,
                                 const observer_ptr<time::xchain_time_face_t> & clock);

    xtxpool_proxy_face_ptr create(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver, const observer_ptr<router::xrouter_face_t> & router) override;
    bool destroy(const xvip2_t & xip) override;
    bool start(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) override;
    bool unreg(const xvip2_t & xip) override;
    bool fade(const xvip2_t & xip) override;
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override;
    void start() override;
    void stop() override;
    void on_timer();

private:
    std::shared_ptr<xtxpool_service_face> find(const xvip2_t & xip);
    void on_block_to_db_event(mbus::xevent_ptr_t e);
    void on_block_confirmed(xblock_t * block);

private:
    xobject_ptr_t<xtxpool_svc_para_t> m_para;
    observer_ptr<base::xiothread_t> m_iothread_timer;
    observer_ptr<base::xiothread_t> m_iothread_dispatcher;
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    observer_ptr<time::xchain_time_face_t> m_clock;
    std::map<xvip2_t, std::shared_ptr<xtxpool_service_face>, xvip2_compare> m_service_map;
    std::mutex m_mutex;
    uint32_t m_bus_listen_id;
    xtxpool_service_timer_t * m_timer{nullptr};
    xtxpool_service_dispatcher_imp_t * m_dispatcher{nullptr};
};

class xtxpool_service_timer_t
  : public xtxpool_service_dispatcher_t
  , public top::base::xxtimer_t {
public:
    xtxpool_service_timer_t(base::xcontext_t & _context, int32_t timer_thread_id, xtxpool_service_mgr * txpool_service_mgr)
      : base::xxtimer_t(_context, timer_thread_id), m_txpool_service_mgr(txpool_service_mgr) {
    }

    void dispatch(base::xcall_t & call) override;

    bool is_mailbox_over_limit() override;

protected:
    ~xtxpool_service_timer_t() override {
    }

protected:
    bool on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms, const int32_t start_timeout_ms, int32_t & in_out_cur_interval_ms) override;

    observer_ptr<xtxpool_service_mgr> m_txpool_service_mgr;
};

class xtxpool_service_dispatcher_imp_t
  : public xtxpool_service_dispatcher_t
  , public base::xiobject_t {
public:
    xtxpool_service_dispatcher_imp_t(base::xcontext_t & _context, int32_t thread_id, xtxpool_service_mgr * txpool_service_mgr)
      : base::xiobject_t(_context, thread_id, base::enum_xobject_type_woker), m_txpool_service_mgr(txpool_service_mgr) {
    }

    void dispatch(base::xcall_t & call) override;

    bool is_mailbox_over_limit() override;

protected:
    ~xtxpool_service_dispatcher_imp_t() override {
    }

protected:
    observer_ptr<xtxpool_service_mgr> m_txpool_service_mgr;
};

NS_END2
