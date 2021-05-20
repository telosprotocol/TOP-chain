#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include "xdata/xdata_common.h"
#include "xbasic/xmemory.hpp"
#include "xbase/xutl.h"
#include "xsync/xsync_time_rejecter.h"

NS_BEG2(top, sync)

class xsync_ratelimit_timer_t;

class xsync_ratelimit_face_t {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool get_token(int64_t now) = 0;
    virtual void feedback(uint32_t cost, int64_t now) = 0;
    virtual void resume() = 0;
};

class response_info {
public:
    uint32_t cost;
    int64_t time;
};

class xsync_ratelimit_ctx_t {
public:
    // last sample
    uint32_t last_success_count{0};
    uint32_t last_fail_count{0};
    uint32_t last_response_count{0};
    uint32_t last_response_cost{0};
    uint32_t last_response_average_cost{0};

    // total sample
    uint32_t total_success_count{0};
    uint32_t total_fail_count{0};
    uint32_t total_response_count{0};
    float total_response_average_count{0};
    uint32_t total_response_average_cost{0};

    // decision result
    float calc_bucket_size{0};
    float real_bucket_size{0};
};

class xsync_ratelimit_t : public xsync_ratelimit_face_t {
public:
    xsync_ratelimit_t(observer_ptr<base::xiothread_t> const & iothread, uint32_t max_allowed_parallels);
    void start() override;
    void stop() override;
    void on_timer();
    bool get_token(int64_t now) override;
    void feedback(uint32_t cost, int64_t now) override;
    void resume() override;

private:
    void data_statistics();
    void make_decision();

private:
    observer_ptr<base::xiothread_t> m_iothread;
    xsync_ratelimit_timer_t *m_timer{nullptr};
    bool m_is_start{false};
    std::mutex m_lock;
    float m_bucket_config_upper;
    float m_bucket;
    // range info
    uint32_t m_last_success_count{0};
    uint32_t m_last_fail_count{0};
    std::list<response_info> m_last_response_list;
    std::list<xsync_ratelimit_ctx_t> m_list_ctx;
    xsync_time_rejecter_t m_time_rejecter{100};// the callback thread of timer maybe blocked
};

class xsync_ratelimit_timer_t : public top::base::xxtimer_t {
public:
    xsync_ratelimit_timer_t(base::xcontext_t &_context, int32_t timer_thread_id, xsync_ratelimit_t *item):
    base::xxtimer_t(_context, timer_thread_id),
    m_item(item) {
    }

protected:
    ~xsync_ratelimit_timer_t() override {
    }

protected:
    bool on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override {
        //printf("on_timer_fire,timer_id=%lld,current_time_ms =%lld,start_timeout_ms=%d, in_out_cur_interval_ms=%d \n",get_timer_id(), current_time_ms,start_timeout_ms,in_out_cur_interval_ms);
        m_item->on_timer();
        return true;
    }

    xsync_ratelimit_t *m_item;
};

NS_END2