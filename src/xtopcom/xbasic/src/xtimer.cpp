#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer.h"

#include <cassert>
#include <cinttypes>


NS_BEG1(top)

xtop_timer::xtop_timer(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                       std::chrono::milliseconds const & ms_in_future,
                       timeout_callback_t callback)
    : m_timer{ io_object->create_timer<asio::steady_timer>(ms_in_future) }
{
    assert(io_object);
    m_timer.async_wait(std::move(callback));
}

xtop_timer::~xtop_timer() noexcept {
    std::error_code ec;
    m_timer.cancel(ec);
}

bool
xtop_timer::expired() const noexcept {
    std::lock_guard<std::mutex> lock{ m_timer_mutex };
    return m_timer.expiry() < asio::steady_timer::clock_type::now();
}

void
xtop_timer::wait() {
    std::lock_guard<std::mutex> lock{ m_timer_mutex };
    asio::error_code ec;
    m_timer.wait(ec);
    assert(!ec || ec == asio::error::operation_aborted);
}


xtop_base_timer_wrapper::xtop_base_timer_wrapper(observer_ptr<xbase_io_context_wrapper_t> io_context, std::chrono::milliseconds ms_in_future, timeout_callback_t callback)
    : base_t(io_context->context(), io_context->thread_id()), m_callback{std::move(callback)}, m_timeout{ms_in_future} {
}

bool xtop_base_timer_wrapper::expired() const noexcept {
    return !const_cast<xtop_base_timer_wrapper *>(this)->is_active();
}

void xtop_base_timer_wrapper::start() {
    assert(!this->running());
    this->running(true);
    base_t::start(m_timeout.count(), 0);
}

void xtop_base_timer_wrapper::stop() {
    base_t::stop();
    close();
    assert(this->running());
    this->running(false);
}

bool xtop_base_timer_wrapper::on_timer_fire(int32_t const thread_id,
                                            int64_t const timer_id,
                                            int64_t const current_time_ms,
                                            int32_t const start_timeout_ms,
                                            int32_t & /*in_out_cur_interval_ms*/) {
    assert(m_callback);
    try {
        m_callback(std::chrono::milliseconds{current_time_ms});
    } catch (std::exception const & eh) {
        xwarn("xxtimer callback throws exception %s. thread_id = %" PRIi32 " timer_id %" PRIi64 " current_time_ms %" PRIi64 " start_timeout_ms %" PRIi32, eh.what(), thread_id, timer_id, current_time_ms, start_timeout_ms);
    } catch (...) {
        xerror("xxtimer callback throws unknown exception. thread_id = %" PRIi32 " timer_id %" PRIi64 " current_time_ms %" PRIi64 " start_timeout_ms %" PRIi32, thread_id, timer_id, current_time_ms, start_timeout_ms);
    }
    return true;
}

xtop_base_timer::xtop_base_timer(observer_ptr<xbase_io_context_wrapper_t> io_object, std::chrono::milliseconds ms_in_future, timeout_callback_t callback)
  : m_timer{make_object_ptr<xbase_timer_wrapper_t>(std::move(io_object), std::move(ms_in_future), std::move(callback))} {
    assert(io_object);
    m_timer->start();
}

xtop_base_timer::~xtop_base_timer() noexcept {
    m_timer->stop();
}

bool xtop_base_timer::expired() const noexcept {
    std::lock_guard<std::mutex> lock{m_timer_mutex};
    return m_timer->expired();
}

void xtop_base_timer::wait() {
    // this API is useless, jsut to make the xbase_timer_t in a uniform style
    assert(false);
}

NS_END1
