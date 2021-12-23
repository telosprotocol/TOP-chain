// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service_v2/xtxpool_service_mgr.h"

#include "xblockstore/xblockstore_face.h"
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtableblock.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xstore/xstore.h"
#include "xtxpool_service_v2/xcons_utl.h"
#include "xtxpool_service_v2/xreceipt_strategy.h"
#include "xtxpool_service_v2/xtxpool_proxy.h"
#include "xtxpool_service_v2/xtxpool_service.h"
#include "xtxpool_v2/xreceipt_resend.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xchain_fork/xchain_upgrade_center.h"

#include <cinttypes>

NS_BEG2(top, xtxpool_service_v2)

#define fast_thread_mailbox_size_max (8192)
#define slow_thread_mailbox_size_max (8192)

#define print_txpool_statistic_values_freq (300)  // print txpool statistic values every 5 minites

xtxpool_service_mgr::xtxpool_service_mgr(const observer_ptr<store::xstore_face_t> & store,
                                         const observer_ptr<base::xvblockstore_t> & blockstore,
                                         const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                         const observer_ptr<base::xiothread_t> & iothread_timer,
                                         const observer_ptr<base::xiothread_t> & iothread_dispather,
                                         const observer_ptr<mbus::xmessage_bus_face_t> & mbus,
                                         const observer_ptr<time::xchain_time_face_t> & clock)
  : m_para(make_object_ptr<xtxpool_svc_para_t>(store, blockstore, txpool, mbus))
  , m_iothread_timer(iothread_timer)
  , m_iothread_dispatcher(iothread_dispather)
  , m_mbus(mbus)
  , m_clock(clock) {
}

void xtxpool_service_mgr::on_block_to_db_event(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_committed) {
        return;
    }

    mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);

    if (block_event->blk_level != base::enum_xvblock_level_table) {
        return;
    }

    // use slow thread to deal with block event
    if (m_timer->is_mailbox_over_limit()) {
        xwarn("xtxpool_service_mgr::on_block_to_db_event txpool mailbox limit,drop block=%s,height:%llu", block_event->blk_hash.c_str(), block_event->blk_height);
        return;
    }

    auto event_handler = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        mbus::xevent_object_t* _event_obj = dynamic_cast<mbus::xevent_object_t*>(call.get_param1().get_object());
        xassert(_event_obj != nullptr);
        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(_event_obj->event);
        const xblock_ptr_t & block = mbus::extract_block_from(block_event, metrics::blockstore_access_from_mbus_txpool_db_event_on_block);
        base::xvaccount_t _vaccount(block->get_account());
        // TODO(jimmy) load block input for get raw tx nonce
        if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, block.get(), metrics::blockstore_access_from_txpool_on_block_event)) {
            xerror("xtxpool_service_mgr::on_block_to_db_event fail-load block input output, block=%s", block->dump().c_str());
            return true;
        }
        xassert(block->check_block_flag(base::enum_xvblock_flag_committed));
        on_block_confirmed(block.get());
        return true;
    };

    base::xauto_ptr<mbus::xevent_object_t> event_obj = new mbus::xevent_object_t(e, 0);
    base::xcall_t asyn_call(event_handler, event_obj.get());
    m_dispatcher->dispatch(asyn_call);
}

void xtxpool_service_mgr::on_block_confirmed(xblock_t * block) {
    xinfo("xtxpool_service_mgr::on_block_confirmed process,level:%d,class:%d,block:%s", block->get_block_level(), block->get_block_class(), block->dump().c_str());

    if (block->is_lighttable()) {
        m_para->get_txpool()->on_block_confirmed(block);
    }
}

// create txpool proxy by networkdriver
xtxpool_proxy_face_ptr xtxpool_service_mgr::create(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver, const observer_ptr<router::xrouter_face_t> & router) {
    auto xip = xcons_utl::to_xip2(vnet_driver->address(), true);
    // auto key = xcons_utl::erase_version(xip);
    xinfo("xtxpool_service_mgr::create network proxy %s xip:{%" PRIu64 ", %" PRIu64 "} ", vnet_driver->address().to_string().c_str(), xip.high_addr, xip.low_addr);

    // auto service = find(xip);
    // if (service != nullptr) {
    //     return std::make_shared<xtxpool_proxy>(xip, vnet_driver, this->shared_from_this(), service);
    // }

    std::shared_ptr<xtxpool_service_face> txpool_service = std::make_shared<xtxpool_service>(router, make_observer(m_para.get()));
    txpool_service->set_params(xip, vnet_driver);
    {
        // store to map
        std::lock_guard<std::mutex> lock(m_mutex);
        m_service_map[xip] = txpool_service;
    }
    base::enum_xchain_zone_index zone_id;
    uint32_t fount_table_id;
    uint32_t back_table_id;
    common::xnode_type_t node_type;
    txpool_service->get_service_table_boundary(zone_id, fount_table_id, back_table_id, node_type);
    m_para->get_txpool()->subscribe_tables(zone_id, fount_table_id, back_table_id, node_type);

    return std::make_shared<xtxpool_proxy>(xip, vnet_driver, this->shared_from_this(), txpool_service);
}

// destroy useless txpool services by networkdriver, call by vnode manager while detemine some service useless
// must call uninit before
bool xtxpool_service_mgr::destroy(const xvip2_t & xip) {
    xinfo("xtxpool_service_mgr::destroy xip:{%" PRIu64 ", %" PRIu64 "} ", xip.high_addr, xip.low_addr);
    return true;
}

std::shared_ptr<xtxpool_service_face> xtxpool_service_mgr::find(const xvip2_t & xip) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_service_map.find(xip);
        if (iter != m_service_map.end()) {
            return iter->second;
        }
    }
    return nullptr;
}

// init txpool service
bool xtxpool_service_mgr::start(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) {
    // auto key = xcons_utl::erase_version(xip);
    xinfo("xtxpool_service_mgr::start xip:{%" PRIu64 ", %" PRIu64 "} ", xip.high_addr, xip.low_addr);
    std::shared_ptr<xtxpool_service_face> service = find(xip);
    if (service != nullptr) {
        service->set_params(xip, vnet_driver);
        service->start(xip);
        return true;
    }
    return false;
}

bool xtxpool_service_mgr::fade(const xvip2_t & xip) {
    xinfo("xtxpool_service_mgr::fade xip:{%" PRIu64 ", %" PRIu64 "} ", xip.high_addr, xip.low_addr);
    std::shared_ptr<xtxpool_service_face> service = find(xip);
    if (service != nullptr) {
        service->fade(xip);
        return true;
    }
    return false;
}

// uninit data
bool xtxpool_service_mgr::unreg(const xvip2_t & xip) {
    // auto key = xcons_utl::erase_version(xip);
    xinfo("xtxpool_service_mgr::unreg xip:{%" PRIu64 ", %" PRIu64 "} ", xip.high_addr, xip.low_addr);
    bool need_cleanup = false;
    base::enum_xchain_zone_index zone_id;
    uint32_t fount_table_id;
    uint32_t back_table_id;
    common::xnode_type_t node_type;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_service_map.find(xip);
        if (iter != m_service_map.end()) {
            auto txpool_service = iter->second;
            txpool_service->get_service_table_boundary(zone_id, fount_table_id, back_table_id, node_type);
            txpool_service->unreg(xip);
            need_cleanup = true;
            m_service_map.erase(iter);
        }
    }

    if (need_cleanup) {
        // clear txpool tables corresponding to this service
        m_para->get_txpool()->unsubscribe_tables(zone_id, fount_table_id, back_table_id, node_type);
    } else {
        xwarn("xtxpool_service_mgr::unreg xip not found:{%" PRIu64 ", %" PRIu64 "}", xip.high_addr, xip.low_addr);
    }
    return false;
}

xcons_transaction_ptr_t xtxpool_service_mgr::query_tx(const std::string & account, const uint256_t & hash) const {
    auto & tx_ent = m_para->get_txpool()->query_tx(account, hash);
    if (tx_ent == nullptr) {
        return nullptr;
    }
    return tx_ent->get_tx();
}

void xtxpool_service_mgr::start() {
    xinfo("xtxpool_service_mgr::start");
    m_bus_listen_id = m_mbus->add_listener(top::mbus::xevent_major_type_store, std::bind(&xtxpool_service_mgr::on_block_to_db_event, this, std::placeholders::_1));
    m_timer = new xtxpool_service_timer_t(top::base::xcontext_t::instance(), m_iothread_timer->get_thread_id(), this);
    m_timer->create_mailbox(256, slow_thread_mailbox_size_max, slow_thread_mailbox_size_max);
    m_timer->start(0, 1000);

    m_dispatcher = new xtxpool_service_dispatcher_imp_t(top::base::xcontext_t::instance(), m_iothread_dispatcher->get_thread_id(), this);
    m_dispatcher->create_mailbox(256, fast_thread_mailbox_size_max, fast_thread_mailbox_size_max);  // create dedicated mailbox for txpool
    m_para->set_dispatchers(make_observer(m_dispatcher), make_observer(m_timer));
}

void xtxpool_service_mgr::stop() {
    xinfo("xtxpool_service_mgr::stop");
    m_timer->close();
    m_timer->release_ref();
}

void xtxpool_service_mgr::on_timer() {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    bool is_time_for_refresh_table = xreceipt_strategy_t::is_time_for_refresh_table(now);
    typedef std::tuple<base::enum_xchain_zone_index, uint32_t, uint32_t, bool> table_boundary_t;
    std::vector<table_boundary_t> table_boundarys;
    std::vector<std::shared_ptr<xtxpool_service_face>> pull_lacking_receipts_service_vec;
    std::vector<std::shared_ptr<xtxpool_service_face>> receipts_recender_service_vec;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto & iter : m_service_map) {
            auto service = iter.second;
            if (!service->is_running()) {
                continue;
            }
            // only receipt sender need recover unconfirmed txs.
            if (service->is_send_receipt_role()) {
                pull_lacking_receipts_service_vec.insert(pull_lacking_receipts_service_vec.begin(), service);
                receipts_recender_service_vec.push_back(service);
            } else {
                pull_lacking_receipts_service_vec.push_back(service);
            }

            if (is_time_for_refresh_table) {
                base::enum_xchain_zone_index zone_id;
                uint32_t fount_table_id;
                uint32_t back_table_id;
                common::xnode_type_t node_type;
                service->get_service_table_boundary(zone_id, fount_table_id, back_table_id, node_type);
                table_boundary_t table_boundary(zone_id, fount_table_id, back_table_id, service->is_send_receipt_role());
                table_boundarys.push_back(table_boundary);
            }
        }
    }

    // because recover might be very time-consuming, recover should not in lock of "m_mutex", or else xtxpool_service_mgr::create may be blocked.
    for (auto & table_boundary : table_boundarys) {
        base::enum_xchain_zone_index zone_id = std::get<0>(table_boundary);
        uint32_t fount_table_id = std::get<1>(table_boundary);
        uint32_t back_table_id = std::get<2>(table_boundary);
        bool refresh_unconfirm_txs = std::get<3>(table_boundary);
        xinfo("xtxpool_service_mgr::on_timer, refresh table zone:%d table:%d:%d refresh_unconfirm_txs:%d", zone_id, fount_table_id, back_table_id, refresh_unconfirm_txs);
        for (uint32_t table_id = fount_table_id; table_id <= back_table_id; table_id++) {
            m_para->get_txpool()->refresh_table(zone_id, table_id);
            // m_para->get_txpool()->update_non_ready_accounts(zone_id, table_id);
        }
    }

    xcovered_tables_t covered_tables;
    for (auto & service : pull_lacking_receipts_service_vec) {
        service->pull_lacking_receipts(now, covered_tables);
    }
    for (auto & service : receipts_recender_service_vec) {
        service->send_receipt_id_state(now);
    }

    if ((now % print_txpool_statistic_values_freq) == 0) {
        m_para->get_txpool()->print_statistic_values();
    }
}

std::shared_ptr<xtxpool_service_mgr_face> xtxpool_service_mgr_instance::create_xtxpool_service_mgr_inst(const observer_ptr<store::xstore_face_t> & store,
                                                                                                        const observer_ptr<base::xvblockstore_t> & blockstore,
                                                                                                        const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                                                                                        const std::vector<xobject_ptr_t<base::xiothread_t>> & iothreads,
                                                                                                        const observer_ptr<mbus::xmessage_bus_face_t> & mbus,
                                                                                                        const observer_ptr<time::xchain_time_face_t> & clock) {
    xassert(iothreads.size() == 2);
    auto txpool_service_mgr = std::make_shared<xtxpool_service_mgr>(store, blockstore, txpool, make_observer(iothreads[0].get()), make_observer(iothreads[1].get()), mbus, clock);
    return txpool_service_mgr;
}

bool xtxpool_service_timer_t::on_timer_fire(const int32_t thread_id,
                                            const int64_t timer_id,
                                            const int64_t current_time_ms,
                                            const int32_t start_timeout_ms,
                                            int32_t & in_out_cur_interval_ms) {
    // printf("on_timer_fire,timer_id=%lld,current_time_ms =%lld,start_timeout_ms=%d, in_out_cur_interval_ms=%d \n",get_timer_id(),
    // current_time_ms,start_timeout_ms,in_out_cur_interval_ms);
    m_txpool_service_mgr->on_timer();
    return true;
}

void xtxpool_service_dispatcher_imp_t::dispatch(base::xcall_t & call) {
    send_call(call);
}

bool xtxpool_service_dispatcher_imp_t::is_mailbox_over_limit() {
    int64_t in, out;
    int32_t queue_size = count_calls(in, out);
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_txpool_fast_cur, queue_size);
    bool discard = queue_size >= fast_thread_mailbox_size_max;
    if (discard) {
        XMETRICS_GAUGE(metrics::mailbox_txpool_fast_total, 0);
        xwarn("xtxpool_service_dispatcher_imp_t::is_mailbox_over_limit in=%ld,out=%ld", in, out);
        return true;
    }
    XMETRICS_GAUGE(metrics::mailbox_txpool_fast_total, 1);
    return false;
}

void xtxpool_service_timer_t::dispatch(base::xcall_t & call) {
    send_call(call);
}

bool xtxpool_service_timer_t::is_mailbox_over_limit() {
    int64_t in, out;
    int32_t queue_size = count_calls(in, out);
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_txpool_slow_cur, queue_size);
    bool discard = queue_size >= slow_thread_mailbox_size_max;
    if (discard) {
        XMETRICS_GAUGE(metrics::mailbox_txpool_slow_total, 0);
        xwarn("xtxpool_service_timer_t::is_mailbox_over_limit in=%ld,out=%ld", in, out);
        return true;
    }
    XMETRICS_GAUGE(metrics::mailbox_txpool_slow_total, 1);
    return false;
}
NS_END2
