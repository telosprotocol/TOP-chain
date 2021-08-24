#include "xsync/xsync_ratelimit.h"

NS_BEG2(top, sync)

const float min_val = 5;
const uint32_t sample_count = 10;
const uint32_t cost_upper = 100;
const uint32_t clean_time_ms = 100;

xsync_ratelimit_t::xsync_ratelimit_t(observer_ptr<base::xiothread_t> const & iothread, uint32_t max_allowed_parallels):
m_iothread(iothread) {

    m_bucket_config_upper = max_allowed_parallels/sample_count;
    m_bucket = min_val;

    xsync_ratelimit_ctx_t ctx;
    ctx.calc_bucket_size = min_val;
    ctx.real_bucket_size = min_val;

    m_list_ctx.push_back(ctx);
}

void xsync_ratelimit_t::start() {
    m_timer = new xsync_ratelimit_timer_t(top::base::xcontext_t::instance(), m_iothread->get_thread_id(), this);
    m_timer->start(0, clean_time_ms);
    m_is_start = true;
}

void xsync_ratelimit_t::stop() {
    m_is_start = false;
    m_timer->close();
    m_timer->release_ref();
}

void xsync_ratelimit_t::resume() {
    xsync_ratelimit_ctx_t new_ctx;

    m_list_ctx.push_back(new_ctx);
    if (m_list_ctx.size() > sample_count) {
        m_list_ctx.pop_front();
    }

    data_statistics();
    make_decision();
}

void xsync_ratelimit_t::on_timer() {
    std::unique_lock<std::mutex> lock(m_lock);
    
    if (m_time_rejecter.reject()){
        return;
    }

    resume();
}

bool xsync_ratelimit_t::get_token(int64_t now) {
    std::unique_lock<std::mutex> lock(m_lock);
    if (!m_time_rejecter.reject()){
        resume();
    }
    
    if (m_bucket >= 1.0) {
        m_last_success_count++;
        m_bucket--;
        return true;
    }

    m_last_fail_count++;

    return false;
}

void xsync_ratelimit_t::feedback(uint32_t cost, int64_t now) {
    std::unique_lock<std::mutex> lock(m_lock);

    response_info info;
    info.cost = cost;
    info.time = now;
    m_last_response_list.push_back(info);
}

void xsync_ratelimit_t::data_statistics() {
    uint32_t last_response_cost = 0;
    uint32_t last_response_count = 0;

    for (auto &it: m_last_response_list) {
        response_info &info = it;
        last_response_count++;
        last_response_cost += info.cost;
    }

    m_list_ctx.back().last_success_count = m_last_success_count;
    m_list_ctx.back().last_fail_count = m_last_fail_count;
    m_list_ctx.back().last_response_count = last_response_count;
    m_list_ctx.back().last_response_cost = last_response_cost;
    if (last_response_count != 0) {
        m_list_ctx.back().last_response_average_cost = last_response_cost/last_response_count;
    }

    m_last_success_count = 0;
    m_last_fail_count = 0;
    m_last_response_list.clear();


    uint32_t total_success_count = 0;
    uint32_t total_fail_count = 0;
    uint32_t total_response_count = 0;
    uint32_t total_response_cost = 0;

    uint32_t count = 0;
    for (std::list<xsync_ratelimit_ctx_t>::reverse_iterator it=m_list_ctx.rbegin(); it!=m_list_ctx.rend(); it++) {

        total_success_count += it->last_success_count;
        total_fail_count += it->last_fail_count;
        total_response_count += it->last_response_count;
        total_response_cost += it->last_response_cost;

        count++;
        if (count >= sample_count)
            break;
    }

    m_list_ctx.back().total_success_count = total_success_count;
    m_list_ctx.back().total_fail_count = total_fail_count;
    m_list_ctx.back().total_response_count = total_response_count;
    m_list_ctx.back().total_response_average_count = (float)total_response_count/count;

    if (total_response_count > 0) {
        m_list_ctx.back().total_response_average_cost = total_response_cost/total_response_count;
    }
}

void xsync_ratelimit_t::make_decision() {

    xsync_ratelimit_ctx_t &ctx = m_list_ctx.back();

    float rate = (float)ctx.last_response_average_cost/(float)cost_upper;
    float factor = 1.0;

    if (rate < 1.0) {

        if (rate < 0.6)
            factor = 1.4;
        else if (rate < 0.7)
            factor = 1.3;
        else if (rate < 0.8)
            factor = 1.2;
        else
            factor = 1.1;

    } else if (rate > 1.0) {

        if (rate > 1.4)
            factor = 0.7;
        else if (rate > 1.3)
            factor = 0.8;
        else if (rate > 1.2)
            factor = 0.9;
        else
            factor = 0.9;
    }

    ctx.calc_bucket_size = ctx.total_response_average_count*factor;

    ctx.real_bucket_size = ctx.calc_bucket_size;
    if (ctx.real_bucket_size > m_bucket_config_upper) {
        ctx.real_bucket_size = m_bucket_config_upper;
    }

    if (ctx.real_bucket_size < min_val)
       ctx.real_bucket_size = min_val; 

    m_bucket = ctx.real_bucket_size;
}

NS_END2