// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <thread>
#include <vector>
#ifdef DEBUG
#include <atomic>
#endif

#include "xbase/xthread.h"
#include "xbase/xobject_ptr.h"
#include "xmbus/xevent_reg_holder.hpp"
#include "xmbus/xmessage_bus.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, mbus)

using namespace top::base;

struct xevent_object_t : public xobject_t
{
    xevent_ptr_t event{};
    int32_t queue_size;

    xevent_object_t(const xevent_ptr_t &e, int32_t _queue_size) :
    event(e),
    queue_size(_queue_size) {
#ifdef DEBUG
        count(1);
#endif
    }

#ifdef DEBUG
    int32_t count(int32_t v) {
        static std::atomic<int32_t> c{0};
        return c.fetch_add(v)+v;
    }
#endif

protected:
    virtual ~xevent_object_t() {
#ifdef DEBUG
        count(-1);
#endif
    }
};

class xbase_sync_event_monitor_t : public xobject_t
{
protected:
    virtual ~xbase_sync_event_monitor_t()
    {
        // stop();
    }

public:
    xbase_sync_event_monitor_t(observer_ptr<xmessage_bus_face_t> const & mb = nullptr, int32_t max_thread_calls = 10000, observer_ptr<base::xiothread_t> const & thread = nullptr)
        : m_reg_holder(mb), m_observed_thread{ thread }, m_max_thread_calls(max_thread_calls)
    {
        if (m_observed_thread == nullptr) {
            if (m_thread == nullptr) {
                m_thread = make_object_ptr<base::xiothread_t>();
                // auto * iothread = base::xiothread_t::create_thread(base::xcontext_t::instance(), base::xiothread_t::enum_xthread_type_general, -1);
                // m_thread.attach(iothread);
            }

            m_observed_thread = make_observer(m_thread);
        }
        assert(m_observed_thread != nullptr);
    }

    void register_listeners()
    {
        xevent_queue_cb_t cb;

        // register listener
        std::vector<int> types;
        get_listeners(types, cb);
        for (auto m : types)
        {
            m_reg_holder.add_listener(m, cb);
        }
    }

    virtual void get_listeners(std::vector<int> &major_types,
                               xevent_queue_cb_t &cb) {}

    virtual void push_event(const xevent_ptr_t &e)
    {
        assert(m_observed_thread != nullptr);

        if (!filter_event(e))
            return;

        int64_t in, out;
        int32_t queue_size = m_observed_thread->count_calls(in, out);
        bool discard = queue_size >= m_max_thread_calls;
        before_event_pushed(e, discard);

        if(discard) {
            return; // discard event
        }

        auto func = [](xcall_t &call, const int32_t thread_id, const uint64_t timenow_ms) -> bool {
            auto eobj = (xevent_object_t*) call.get_param1().get_object();
            auto _this = (xbase_sync_event_monitor_t*) call.get_param2().get_object();
            _this->dump_queue_info(eobj);
            _this->process_event(eobj->event);
            return true;
        };

        base::xauto_ptr<xevent_object_t> event_obj = new xevent_object_t(e, queue_size);
#ifdef DEBUG
        auto count = event_obj->count(0);
        if((count % 10) == 0) {
            xdbg("unprocessed callback event: %d", count);
        }
#endif
        xcall_t c((xcallback_t) func, event_obj.get(), this);
        m_observed_thread->send_call(c);

        after_event_pushed(e);
    }

protected:
    virtual bool filter_event(const xevent_ptr_t &e) = 0;
    virtual void process_event(const xevent_ptr_t &e) = 0;
    virtual void before_event_pushed(const xevent_ptr_t &e, bool &discard) {}
    virtual void after_event_pushed(const xevent_ptr_t& e) {}
    virtual void dump_queue_info(const xevent_object_t *e_obj) {}

protected:
    xevent_reg_holder_t m_reg_holder;
    xobject_ptr_t<xiothread_t> m_thread{};
    observer_ptr<xiothread_t> m_observed_thread{ nullptr };
    int32_t m_max_thread_calls{10000}; // total max thread calls for all instances shared same thread
};

NS_END2
