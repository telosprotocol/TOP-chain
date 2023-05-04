// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cassert>
#include <thread>
#include "xsync/xsync_event_dispatcher.h"
#include "xmbus/xevent_common.h"
#include "xmbus/xevent_role.h"

#include "xmbus/xevent.h"
#include "xcommon/xmessage_id.h"
#include "xbase/xbase.h"
#include "xdata/xgenesis_data.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

using namespace mbus;
using namespace vnetwork;
using namespace data;

xsync_event_dispatcher_t::xsync_event_dispatcher_t(
        observer_ptr<base::xiothread_t> const & iothread,
        std::string vnode_id,
        observer_ptr<mbus::xmessage_bus_face_t> const &mb,
        xsync_handler_t *sync_handler,
        int32_t max_thread_calls) :
xbase_sync_event_monitor_t(mb, max_thread_calls, iothread),
m_vnode_id(vnode_id),
m_sync_handler(sync_handler) {

    mbus::xevent_queue_cb_t cb = std::bind(&xsync_event_dispatcher_t::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_timer, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_behind, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_role, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_consensus, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_chain_timer, cb);

    xsync_kinfo("xsync_event_dispatcher_t create");
}

xsync_event_dispatcher_t::~xsync_event_dispatcher_t() {
}

bool xsync_event_dispatcher_t::filter_event(const mbus::xevent_ptr_t& e) {

    bool ret = false;

    switch(e->major_type) {
        case mbus::xevent_major_type_timer:
            ret = true;
            break;
        case mbus::xevent_major_type_behind:
            ret = e->minor_type == mbus::xevent_behind_t::type_download ||
                    e->minor_type == mbus::xevent_behind_t::type_check ||
                    e->minor_type == mbus::xevent_behind_t::type_on_demand ||
                    e->minor_type == mbus::xevent_behind_t::type_on_demand_by_hash;
            break;
        case mbus::xevent_major_type_role:
            ret = true;
            break;
        case mbus::xevent_major_type_consensus:
            ret = true;
            break;
        case mbus::xevent_major_type_chain_timer:
            ret = true;
            break;
        default:
            ret = false;
            break;
    }

#ifdef ENABLE_METRICS
    int64_t in, out;
    int32_t queue_size = m_observed_thread->count_calls(in, out);
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_xsync_cur, queue_size);
#endif
    return ret;
}

void xsync_event_dispatcher_t::before_event_pushed(const mbus::xevent_ptr_t &e, bool &discard) {
    if (e->major_type == mbus::xevent_major_type_role) {
        discard = false;
    }
    XMETRICS_GAUGE(metrics::mailbox_xsync_total, discard ? 0 : 1);
}

void xsync_event_dispatcher_t::dump_queue_info(const mbus::xevent_object_t *e_obj) {
    int64_t now = base::xtime_utl::gmttime_ms();
    int64_t wait_cost = now - e_obj->event->m_time;
    check_queue_info(wait_cost, e_obj->queue_size);
}

void xsync_event_dispatcher_t::check_queue_info(int64_t wait_cost, int32_t queue_size) {
    XMETRICS_GAUGE(metrics::xsync_cost_event_queue, wait_cost);
    if (wait_cost > 200) {
        xsync_warn("xsync_event_dispatcher_t too long to wait. wait_cost(%ldms) event_queue(%d)", 
                wait_cost, queue_size);
    }
}

void xsync_event_dispatcher_t::process_event(const mbus::xevent_ptr_t& e) {
    m_sync_handler->on_event(e);
}

NS_END2
