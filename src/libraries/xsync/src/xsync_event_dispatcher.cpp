// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cassert>
#include <thread>
#include "xsync/xsync_event_dispatcher.h"
#include "xmbus/xevent_common.h"
#include "xmbus/xevent_role.h"

#include "xmbus/xevent.h"
#include "xsyncbase/xmessage_ids.h"
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

    if (ret)
        XMETRICS_COUNTER_INCREMENT("sync_eventdispatcher_event_count", 1);

    return ret;
}

void xsync_event_dispatcher_t::before_event_pushed(const mbus::xevent_ptr_t &e, bool &discard) {
    XMETRICS_COUNTER_INCREMENT("sync_event_total", 1);
    if (e->get_type() == mbus::xevent_major_type_role) {
        discard = false;
    }
#ifdef DEBUG
    if(discard) {
        XMETRICS_COUNTER_INCREMENT("sync_total_discard", 1);
    } else {
        switch(e->major_type) {
            case mbus::xevent_major_type_behind:
                switch(e->minor_type) {
                    case mbus::xevent_behind_t::type_download: XMETRICS_COUNTER_INCREMENT("sync_event_behind_known", 1); break;
                    case mbus::xevent_behind_t::type_check: XMETRICS_COUNTER_INCREMENT("sync_event_behind_table", 1); break;
                    case mbus::xevent_behind_t::type_on_demand: XMETRICS_COUNTER_INCREMENT("sync_event_behind_on_demand", 1); break;
                }
                break;
            case mbus::xevent_major_type_role:
                if (e->minor_type == xevent_role_t::add_role)
                    XMETRICS_COUNTER_INCREMENT("sync_event_add_role", 1);
                else if (e->minor_type == xevent_role_t::remove_role)
                    XMETRICS_COUNTER_INCREMENT("sync_event_remove_role", 1);
                break;
            case mbus::xevent_major_type_consensus:
                XMETRICS_COUNTER_INCREMENT("sync_event_consensus", 1);
                break;
            case mbus::xevent_major_type_chain_timer:
                XMETRICS_COUNTER_INCREMENT("sync_event_chain_timer", 1);
                break;
            default:
                break;
        }
    }
#endif
}

void xsync_event_dispatcher_t::dump_queue_info(const mbus::xevent_object_t *e_obj) {
    int64_t now = base::xtime_utl::gmttime_ms();
    int64_t wait_cost = now - e_obj->event->m_time;
    check_queue_info(wait_cost, e_obj->queue_size);
}

void xsync_event_dispatcher_t::check_queue_info(int64_t wait_cost, int32_t queue_size) {
    XMETRICS_COUNTER_INCREMENT("sync_cost_event_queue", wait_cost);
    if (wait_cost > 200) {
        xsync_warn("xsync_event_dispatcher_t too long to wait. wait_cost(%ldms) event_queue(%d)", 
                wait_cost, queue_size);
    }
}

void xsync_event_dispatcher_t::process_event(const mbus::xevent_ptr_t& e) {

    XMETRICS_COUNTER_INCREMENT("sync_eventdispatcher_event_count", -1);

    XMETRICS_TIME_RECORD("sync_cost_event_process");
    m_sync_handler->on_event(e);
}

NS_END2
