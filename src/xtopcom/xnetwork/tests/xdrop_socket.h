// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_io_context_wrapper.h"
#include "xnetwork/xsocket_face.h"

#include <cassert>
#include <memory>
#include <random>

NS_BEG3(top, network, tests)

template <typename SocketT, std::size_t Ratio>
class xtop_drop_socket final : public xsocket_face_t
                             , public std::enable_shared_from_this<xtop_drop_socket<SocketT, Ratio>>
{
    XSTATIC_ASSERT((std::is_base_of<xsocket_face_t, SocketT>::value));

    std::shared_ptr<SocketT> m_socket;
    std::random_device m_rd{};
    std::mt19937_64 m_gen{ m_rd() };
    std::uniform_int_distribution<std::size_t> m_distribution{ 1, 100 };

public:
    xtop_drop_socket(xtop_drop_socket const &)             = delete;
    xtop_drop_socket & operator=(xtop_drop_socket const &) = delete;
    xtop_drop_socket(xtop_drop_socket &&)                  = default;
    xtop_drop_socket & operator=(xtop_drop_socket &&)      = default;
    ~xtop_drop_socket() override                           = default;

    xtop_drop_socket(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                     std::uint16_t const port)
        : m_socket{ std::make_shared<SocketT>(io_object, port) }
    {
    }

    xtop_drop_socket(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                     xendpoint_t const & endpoint)
        : m_socket{ std::make_shared<SocketT>(io_object, endpoint) }
    {
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
        assert(m_socket);
        m_socket->start();
    }

    void
    stop() override {
        assert(m_socket);
        m_socket->stop();
    }

    bool
    running() const noexcept override {
        assert(m_socket);
        return m_socket->running();
    }

    void
    running(bool const v) noexcept override {
        assert(m_socket);
        m_socket->running(v);
    }

    void
    send_to(xendpoint_t const & peer,
            xbyte_buffer_t data,
            xdeliver_property_t const & deliver_property) override {
        if (m_distribution(m_gen) <= Ratio) {
            xdbg("[drop socket] drops a packet");
        } else {
            m_socket->send_to(peer, std::move(data), deliver_property);
        }
    }
};

template <typename SocketT, std::size_t Ratio = 10>
using xdrop_socket_t = xtop_drop_socket<SocketT, Ratio>;     // compile-time drop ratio, defaults to 10%

NS_END3
