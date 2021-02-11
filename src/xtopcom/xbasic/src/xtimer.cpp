#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer.h"

#include <cassert>


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

NS_END1
