#include <gtest/gtest.h>
#include <map>
#include <list>
#include <memory>
#include "xdata/xdata_common.h"
#include "xsync/xsync_ratelimit.h"
#include "xbase/xthread.h"
#include "xbase/xobject_ptr.h"
#include "xbasic/xmemory.hpp"
#include "xpbase/base/top_utils.h"

using namespace top;
using namespace top::data;
using namespace top::sync;

class xfake_account_t {
public:
    xfake_account_t(uint32_t id, sync::xsync_ratelimit_t *ratelimit):
    m_id(id),
    m_ratelimit(ratelimit) {
    }

    void on_timer() {

        int64_t now = base::xtime_utl::gmttime_ms();

        if (m_send_request_time > 0) {
            on_response(now);
        } else {
            send_request(now);
        }
    }

    void send_request(int64_t now) {
        assert(m_send_request_time == 0);
        if (m_ratelimit->get_token(now)) {
            m_send_request_time = now;
            // 10ms response
            m_next_timeout = now + 10;
            //printf("send_request00 id=%u now=%ld next=%ld request=%ld\n", m_id, now, m_next_timeout, m_send_request_time);
        } else {
            // 6000ms retry
            m_next_timeout = now + 1000;
            //printf("send_request11 id=%u now=%ld next=%ld request=%ld\n", m_id, now, m_next_timeout, m_send_request_time);
        }
    }

    void on_response(int64_t now) {
        // sleep 1-20ms
        assert(m_send_request_time > 0);

        int64_t cost = now - m_send_request_time;

        m_ratelimit->feedback(cost, now);

        for (uint64_t i=1; i<1000; i++)
            for (uint64_t j=1; j<10000; j++)
                ;

        m_send_request_time = 0;

        int64_t now2 = base::xtime_utl::gmttime_ms();
        send_request(now2);
    }

    int64_t get_next_timeout() {
        return m_next_timeout;
    }

    uint32_t get_id() {
        return m_id;
    }

private:
    uint32_t m_id{0};
    sync::xsync_ratelimit_t *m_ratelimit;
    int64_t m_send_request_time{0};
    int64_t m_next_timeout{0};
    uint64_t m_height{0};
};

class xfake_account_timer_t : public top::base::xxtimer_t {
public:
    xfake_account_timer_t(base::xcontext_t &_context, int32_t timer_thread_id):
    base::xxtimer_t(_context, timer_thread_id) {
    }

    void set_timeout_event(xfake_account_t *account) {

        uint32_t id = account->get_id();
        {
            auto it = m_accounts.find(id);
            assert(it == m_accounts.end());
        }

        int64_t next_timeout = account->get_next_timeout();

        auto it = m_timeout_events.find(next_timeout);
        if (it == m_timeout_events.end()) {
            std::list<xfake_account_t*> list_accounts;
            list_accounts.push_back(account);
            m_timeout_events[next_timeout] = list_accounts;
        } else {
            it->second.push_back(account);
        }

        m_accounts[id] = next_timeout;
    }

protected:
    ~xfake_account_timer_t() override {

    }

protected:
    bool on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override {

        int64_t now = base::xtime_utl::gmttime_ms();
        std::list<xfake_account_t*> list_timeout_events;

        std::map<int64_t,std::list<xfake_account_t*>>::iterator it = m_timeout_events.begin();
        for (;it!=m_timeout_events.end();) {

            if (now < it->first) {
                break;
            }

            while (!it->second.empty()) {
                xfake_account_t* account = it->second.front();

                account->on_timer();
                list_timeout_events.push_back(account);

                uint32_t id = account->get_id();
                m_accounts.erase(id);

                it->second.pop_front();
            }

            m_timeout_events.erase(it++);
        }

        for (auto &it : list_timeout_events) {
            set_timeout_event(it);
        }

        return true;
    }

private:
    std::map<int64_t,std::list<xfake_account_t*>> m_timeout_events;
    std::unordered_map<uint32_t,int64_t> m_accounts;
};

#if 0
TEST(xsync_ratelimit, test) {

    uint32_t max_allowed_parallels = 500;
    uint32_t account_count = 200;
    uint32_t thread_count = 2;

    xobject_ptr_t<base::xiothread_t> thread = make_object_ptr<base::xiothread_t>();

    std::shared_ptr<sync::xsync_ratelimit_t> ratelimit = std::make_shared<sync::xsync_ratelimit_t>(make_observer(thread), max_allowed_parallels);
    ratelimit->start();

    // create 10000 account
    std::vector<xfake_account_t*> vector_accounts;
    for (uint32_t i=1; i<=account_count; i++) {
        xfake_account_t *account = new xfake_account_t(i, ratelimit.get());
        vector_accounts.push_back(account);
    }
    // create 2 threads
    std::vector<observer_ptr<base::xiothread_t>> vector_threads;
    std::vector<xfake_account_timer_t*> vector_timers;
    for (uint32_t i=0; i<thread_count; i++) {
        top::xobject_ptr_t<base::xiothread_t> thread = top::make_object_ptr<base::xiothread_t>();
        vector_threads.push_back(top::make_observer(thread));
        xfake_account_timer_t *timer = new xfake_account_timer_t(top::base::xcontext_t::instance(), thread->get_thread_id());
        timer->start(0, 1);
        vector_timers.push_back(timer);
    }

    int64_t now = base::xtime_utl::gmttime_ms();
    for (auto &it: vector_accounts) {
        xfake_account_t* account = it;
        account->send_request(now);
        uint32_t idx = account->get_id()%thread_count;
        vector_timers[idx]->set_timeout_event(account);
        top::base::xtime_utl::sleep_ms(5);
    }

    printf("######all ready########\n");

    while (1)
        sleep(1);
}
#endif