// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xrunnable.h"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xtimer_driver.h"
#include "xbasic/xmemory.hpp"
#include "xnetwork/xbasic_socket.h"
#include "xnetwork/xnetwork_bytes_message.h"

#include <cassert>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <type_traits>

NS_BEG3(top, network, tests)

template <typename T>
struct xtop_timer_queue_packet{
    std::chrono::milliseconds timeout;
    T data;
};

template <typename T>
using xtimer_queue_packet_t = xtop_timer_queue_packet<T>;

template <typename T>
bool
packet_timeout(xtimer_queue_packet_t<T> const & pakcet) noexcept {
    return pakcet.timeout <= std::chrono::milliseconds{ 0 };
}

NS_END3


NS_BEG1(std)

template <typename T>
struct greater<top::network::tests::xtimer_queue_packet_t<T>> final
{
    bool
    operator()(top::network::tests::xtimer_queue_packet_t<T> const & lhs,
               top::network::tests::xtimer_queue_packet_t<T> const & rhs) const {
        return lhs.timeout > rhs.timeout;
    }
};

template <typename T>
struct less<top::network::tests::xtimer_queue_packet_t<T>> final
{
    bool
    operator()(top::network::tests::xtimer_queue_packet_t<T> const & lhs,
               top::network::tests::xtimer_queue_packet_t<T> const & rhs) const {
        return lhs.timeout < rhs.timeout;
    }
};


NS_END1

NS_BEG3(top, network, tests)

template
<
    typename T,
    typename ContainerT
>
class xtop_timer_queue final
{
public:
    using container_type = ContainerT;
    using value_type = typename container_type::value_type;
    using value_compare = std::less<typename container_type::value_type>;
    using size_type = typename container_type::size_type;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;

private:
    using base_t = xbasic_runnable_t<xtop_timer_queue<T, ContainerT>>;

    value_compare m_compare{};
    container_type m_container{};
    //observer_ptr<xtimer_driver_t> m_timer_driver;
    //std::chrono::microseconds m_timeout;

public:
    xtop_timer_queue()                                     = default;
    xtop_timer_queue(xtop_timer_queue const &)             = delete;
    xtop_timer_queue & operator=(xtop_timer_queue const &) = delete;
    xtop_timer_queue(xtop_timer_queue &&)                  = default;
    xtop_timer_queue & operator=(xtop_timer_queue &&)      = default;
    ~xtop_timer_queue()                                    = default;

    const_reference
    top() const {
        return m_container.front();
    }

    void
    push(T const & value, std::chrono::milliseconds timeout) {
        m_container.push_back({ std::move(timeout), value });
        std::sort(std::begin(m_container), std::end(m_container), m_compare);
    }

    void
    push(T && value, std::chrono::milliseconds timeout) {
        m_container.push_back({ std::move(timeout), std::move(value) });
        std::sort(std::begin(m_container), std::end(m_container), m_compare);
    }

    void
    pop() {
        m_container.pop_front();
    }

    XATTRIBUTE_NODISCARD
    bool
    empty() const {
        return m_container.empty();
    }

    size_type
    size() const {
        return m_container.size();
    }

    void
    update_timeout(std::chrono::milliseconds const & sub) {
        for (auto & value : m_container) {
            if (value.timeout >= sub) {
                value.timeout -= sub;
            } else {
                value.timeout = std::chrono::milliseconds{ 0 };
            }
        }
    }
};

template <typename T, typename ContainerT = std::deque<xtimer_queue_packet_t<T>>>
using xtimer_queue_t = xtop_timer_queue<T, ContainerT>;

struct xtop_trip_delay_packet
{
    xendpoint_t peer;
    xbyte_buffer_t data;
    xdeliver_property_t deliver_property;
    std::shared_ptr<xsocket_face_t> socket;
};
using xtrip_delay_packet_t = xtop_trip_delay_packet;

class xtop_delay_executer final : public xbasic_runnable_t<xtop_delay_executer>
{
private:
    std::shared_ptr<xasio_io_context_wrapper_t> m_io_object{ std::make_shared<xasio_io_context_wrapper_t>() };
    std::shared_ptr<xtimer_driver_t> m_timer_driver{ std::make_shared<xtimer_driver_t>(m_io_object) };

    mutable std::mutex m_timer_queue_mutex{};
    xtimer_queue_t<xtrip_delay_packet_t> m_timer_queue{};

    mutable std::mutex m_queue_mutex{};
    std::vector<xtrip_delay_packet_t> m_queue{};

public:
    xtop_delay_executer()                                        = default;
    xtop_delay_executer(xtop_delay_executer const &)             = delete;
    xtop_delay_executer & operator=(xtop_delay_executer const &) = delete;
    xtop_delay_executer(xtop_delay_executer &&)                  = delete;
    xtop_delay_executer & operator=(xtop_delay_executer &&)      = delete;
    ~xtop_delay_executer()                                       = default;

    void
    start() override {
        assert(m_timer_driver);
        m_timer_driver->start();

        assert(!running());
        running(true);

        on_timer();
        // on_timer2();

        m_io_object->async_start();
    }

    void
    stop() override {
        m_io_object->stop();

        assert(running());
        running(false);

        m_timer_driver->stop();
    }

    void
    push(xtrip_delay_packet_t packet, std::chrono::milliseconds delay) {
        XLOCK_GUARD(m_timer_queue_mutex) {
            m_timer_queue.push(std::move(packet), std::move(delay));
        }
    }

    void
    push(xtrip_delay_packet_t packet) {
        XLOCK_GUARD(m_queue_mutex) {
            m_queue.push_back(std::move(packet));
        }
    }

private:
    void
    on_timer() {
        if (!running()) {
            return;
        }

        assert(m_timer_driver);
        m_timer_driver->schedule(std::chrono::milliseconds{ 100 }, [this](std::error_code const & ec) {
            if (ec && ec == std::errc::operation_canceled) {
                return;
            }

            if (!running()) {
                return;
            }

            do {
                XLOCK_GUARD(m_timer_queue_mutex) {
                    if (m_timer_queue.empty()) {
                        break;
                    }

                    m_timer_queue.update_timeout(std::chrono::milliseconds{ 100 });
                    while (packet_timeout(m_timer_queue.top())) {
                        auto & delay_packet = m_timer_queue.top().data;
                        delay_packet.socket->send_to(delay_packet.peer, std::move(delay_packet.data), delay_packet.deliver_property);

                        m_timer_queue.pop();
                    }
                }
            } while (false);

            on_timer();
        });
    }

    void
    on_timer2() {
        if (!running()) {
            return;
        }

        assert(m_timer_driver);
        m_timer_driver->schedule(std::chrono::milliseconds{ 200 }, [this](std::error_code const & ec) {
            if (ec && ec == std::errc::operation_canceled) {
                return;
            }

            if (!running()) {
                return;
            }

            do {
                XLOCK_GUARD(m_queue_mutex) {
                    if (m_queue.empty()) {
                        break;
                    }

                    for (auto & delay_packet : m_queue) {
                        delay_packet.socket->send_to(delay_packet.peer, std::move(delay_packet.data), delay_packet.deliver_property);
                    }

                    m_queue.clear();
                }
            } while (false);

            on_timer2();
        });
    }
};
using xdelay_executer_t = xtop_delay_executer;

extern xdelay_executer_t delay_executer;

template <typename SocketT, std::size_t MinDelay, std::size_t MaxDelay>
class xtop_trip_delay_socket final : public xsocket_face_t
{
private:
    XSTATIC_ASSERT((std::is_base_of<xsocket_face_t, SocketT>::value));

    std::shared_ptr<SocketT> m_socket;
    observer_ptr<xdelay_executer_t> m_delay_executer;

public:
    xtop_trip_delay_socket(xtop_trip_delay_socket const &)             = delete;
    xtop_trip_delay_socket & operator=(xtop_trip_delay_socket const &) = delete;
    xtop_trip_delay_socket(xtop_trip_delay_socket &&)                  = default;
    xtop_trip_delay_socket & operator=(xtop_trip_delay_socket &&)      = default;
    ~xtop_trip_delay_socket() override                          = default;

    xtop_trip_delay_socket(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                           std::uint16_t const port)
        : m_socket{ std::make_shared<SocketT>(io_object, port) }
    {
    }

    xtop_trip_delay_socket(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                           xendpoint_t const & endpoint)
        : m_socket{ std::make_shared<SocketT>(io_object, endpoint) }
    {
    }

    void
    set_delay_executer(xdelay_executer_t * executer) {
        m_delay_executer = make_observer(executer);
    }

    void
    register_data_ready_notify(xsocket_data_ready_callback_t callback) override {
        assert(m_socket);
        m_socket->register_data_ready_notify(std::move(callback));
    }

    void
    unregister_data_ready_notify() override {
        assert(m_socket);
        m_socket->unregister_data_ready_notify();
    }

    xendpoint_t
    local_endpoint() const noexcept {
        return m_socket->local_endpoint();
    }

    void
    start() override {
        set_delay_executer(&delay_executer);
        m_socket->start();
    }

    void
    stop() override {
        m_socket->stop();
        set_delay_executer(nullptr);
    }

    bool
    running() const noexcept override {
        return m_socket->running();
    }

    void
    running(bool const v) noexcept override {
        m_socket->running(v);
    }

    void
    send_to(xendpoint_t const & peer,
           xbyte_buffer_t data,
           xdeliver_property_t const & deliver_property) override {
       std::random_device rd{};
       std::mt19937 gen(rd());
       std::uniform_int_distribution<std::size_t> distribution{ MinDelay, MaxDelay };

       delay_send_to(peer, std::move(data), deliver_property, std::chrono::milliseconds{ distribution(gen) });
    }

    // void
    // send_to(xendpoint_t const & peer,
    //         xbyte_buffer_t data,
    //         xdeliver_property_t const & deliver_property) override {

    //     delay_send_to(peer, std::move(data), deliver_property, std::chrono::milliseconds{ 0 });
    // }

private:
    void
    delay_send_to(xendpoint_t peer,
                  xbyte_buffer_t data,
                  xdeliver_property_t deliver_property,
                  std::chrono::milliseconds delay) {
        if (m_delay_executer) {
            if (delay != std::chrono::milliseconds{ 0 }) {
                m_delay_executer->push(xtrip_delay_packet_t{ std::move(peer), std::move(data), std::move(deliver_property), m_socket },
                                       std::move(delay));
            } else {
                m_delay_executer->push(xtrip_delay_packet_t{ std::move(peer), std::move(data), std::move(deliver_property), m_socket });
            }
        }
    }
};

template <typename SocketT, std::size_t MinDelay, std::size_t MaxDelay>
using xtrip_delay_socket_t = xtop_trip_delay_socket<SocketT, MinDelay, MaxDelay>;

NS_END3
