// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include "xchain_timer/xchain_timer_face.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xsync/xsync_handler.h"

NS_BEG2(top, sync)


class xsync_event_dispatcher_t : public mbus::xbase_sync_event_monitor_t {
public:
    xsync_event_dispatcher_t(
            observer_ptr<base::xiothread_t> const & iothread,
            std::string vnode_id,
            observer_ptr<mbus::xmessage_bus_face_t> const &mb,
            xsync_handler_t *sync_handler,
            int32_t max_thread_calls = xsync_event_queue_size_max);

protected:
    virtual ~xsync_event_dispatcher_t();

protected:
    void dump_queue_info(const mbus::xevent_object_t *e_obj) override;
    bool filter_event(const mbus::xevent_ptr_t& e) override;
    void before_event_pushed(const mbus::xevent_ptr_t &e, bool &discard) override;
    void check_queue_info(int64_t wait_cost, int32_t queue_size);
    void process_event(const mbus::xevent_ptr_t& e) override;

protected:
    std::string m_vnode_id;
    xsync_handler_t *m_sync_handler{nullptr};
};

using xsync_event_dispatcher_ptr_t = xobject_ptr_t<xsync_event_dispatcher_t>;

NS_END2
