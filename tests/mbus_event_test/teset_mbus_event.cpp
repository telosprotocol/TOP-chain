#include "gtest/gtest.h"
#include "xbase/xthread.h"
#include "xbase/xtimer.h"
#include "xmbus/xevent_account.h"
#include "xmbus/xevent_timer.h"
#include "xbase/xcontext.h"
#include "xmetrics/xmetrics.h"
#include "xmbus/xbase_sync_event_monitor.hpp"

using namespace top;
using namespace top::base;
using namespace mbus;

#define TEST_MAX_QUEUE_SIZE (2000)

class event_face_t {
public:
    virtual void process_event(const mbus::xevent_ptr_t& e) = 0;
};

class xaccount_timer_t : public top::base::xxtimer_t {
public:
    xaccount_timer_t(base::xcontext_t& _context, int32_t timer_thread_id)
        : base::xxtimer_t(_context, timer_thread_id)
    {
    }

protected:
    ~xaccount_timer_t() override
    {
    }

protected:
    bool on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms,
        const int32_t start_timeout_ms, int32_t& in_out_cur_interval_ms) override
    {
        int64_t now = base::xtime_utl::gmttime_ms();
        /*xinfo("on_timer_fire,thread_id=%lld,timer_id=%lld,current_time_ms =%lld,start_timeout_ms=%d, in_out_cur_interval_ms=%d now=%d \n", 
                thread_id, get_timer_id(), current_time_ms, start_timeout_ms, in_out_cur_interval_ms, now);*/
        return true;
    }
};

class xevent_monitor_t : public mbus::xbase_sync_event_monitor_t {
public:
    xevent_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const& mb,
        observer_ptr<base::xiothread_t> const& iothread,
        event_face_t* _test_mbus_event);

    void before_event_pushed(const mbus::xevent_ptr_t& e, bool& discard) override;
    bool filter_event(const mbus::xevent_ptr_t& e) override;
    void process_event(const mbus::xevent_ptr_t& e) override;

private:
    event_face_t* m_test_mbus;
};

xevent_monitor_t::xevent_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const& mb,
    observer_ptr<base::xiothread_t> const& iothread,
    event_face_t* _test_mbus_event)
    : xbase_sync_event_monitor_t(mb, TEST_MAX_QUEUE_SIZE, iothread)
    , m_test_mbus { _test_mbus_event }
{
    mbus::xevent_queue_cb_t cb = std::bind(&xevent_monitor_t::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int)mbus::xevent_major_type_timer, cb);
    m_reg_holder.add_listener((int)mbus::xevent_major_type_account, cb);
}

void xevent_monitor_t::before_event_pushed(const mbus::xevent_ptr_t& e, bool& discard)
{
    //xdbg("xevent_monitor_t::before_event_pushed  type = %d , major_type = %d",e->get_type() , e->major_type);
    if (e->major_type == mbus::xevent_major_type_account) {
        discard = true;
    }
    
    if (e->major_type == mbus::xevent_major_type_account) {
        EXPECT_EQ(discard, true);
    }
    //xdbg("before_event_pushed discard %d ", discard);
}

bool xevent_monitor_t::filter_event(const mbus::xevent_ptr_t& e)
{
    int64_t in, out;
    int32_t queue_size = m_observed_thread->count_calls(in, out);
    //xdbg("xevent_monitor_t::filter_event queue_size:%d in:%ld out:%ld ", queue_size, in, out);

    return true;
}

void xevent_monitor_t::process_event(const mbus::xevent_ptr_t& e)
{
    //xdbg("xevent_monitor_t::process_event start");
    m_test_mbus->process_event(e);
    //xdbg("xevent_monitor_t::process_event end");
}

class test_mbus_event : public testing::Test, public event_face_t {
protected:
    void SetUp() override
    {
    }

    void start_thread()
    {
        m_self_bus = std::make_shared<xmessage_bus_t>();
        m_self_thread = make_object_ptr<base::xiothread_t>();
       
        m_self_timer = new xaccount_timer_t(top::base::xcontext_t::instance(), m_self_thread->get_thread_id());
        m_self_timer->start(0, 100);
        m_self_monitor = std::make_shared<xevent_monitor_t>(make_observer(m_self_bus.get()), make_observer(m_self_thread), this);
        
    }

    void TearDown() override
    {
    }

    void process_event(const mbus::xevent_ptr_t& e)
    {
        index++;
        //xdbg("test_mbus_event::process_event index %ld .", index);
    }

public:
    void test_event_cb(mbus::xevent_ptr_t e)
    {
        xinfo("xsync_event_cb");
        m_self_bus->push_event(e);
    }

public:
    std::shared_ptr<xmessage_bus_t> m_self_bus;
    xobject_ptr_t<base::xiothread_t> m_self_thread;
    xaccount_timer_t* m_self_timer { NULL };
    std::shared_ptr<xevent_monitor_t> m_self_monitor;
    uint64_t index { 0 };

    //  std::shared_ptr<xmessage_bus_t> m_main_bus;
};

void mock_event_push(std::shared_ptr<xmessage_bus_t> bus, int event_type)
{
    int i = 2000;
    do {
        if (event_type % 2 == 0) {
            xevent_ptr_t ev = make_object_ptr<mbus::xevent_account_add_role_t>("Ta0000@0");
            bus->push_event(ev);
        } else {
            xevent_ptr_t ev = make_object_ptr<xevent_timer_t>();
            bus->push_event(ev);
        }
        i--;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (i > 0);
}

TEST_F(test_mbus_event, test_1)
{
    start_thread();

    std::shared_ptr<xmessage_bus_t> m_main_bus = std::make_shared<xmessage_bus_t>();
    m_main_bus->add_listener((int)mbus::xevent_major_type_timer, std::bind(&test_mbus_event::test_event_cb, this, std::placeholders::_1));
    m_main_bus->add_listener((int)mbus::xevent_major_type_account, std::bind(&test_mbus_event::test_event_cb, this, std::placeholders::_1));
    std::thread t1 = std::thread(&mock_event_push, m_main_bus, 1);
    std::thread t2 = std::thread(&mock_event_push, m_main_bus, 2);

    t1.join();
    t2.join();
}

// todo test create_mailbox later
