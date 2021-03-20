// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xobject_ptr.h"
#include "xnetwork/xbasic_socket.h"
#include "xnetwork/xendpoint.h"
#include "xnetwork/xnetwork_bytes_message.h"
#include "xnetwork/xsocket_wrapper.h"

#include <asio/ip/udp.hpp>

#include <deque>
#include <memory>
#include <mutex>

NS_BEG2(top, network)

class xtop_udp_socket final : public std::enable_shared_from_this<xtop_udp_socket>
                            , public xbasic_socket_t
{
private:
    using base_t = xbasic_socket_t;

protected:
    asio::ip::udp::endpoint m_sender_endpoint{};                    // the remote sending endpoint
    static constexpr std::size_t pack_size_upper_limit{ 60000 };    // the buffer size upper limit for an UDP packet
    std::array<xbyte_t, pack_size_upper_limit> m_recv_buffer{};     // the receiving buffer

    mutable std::mutex m_send_queue_mutex{};                        // sending queue mutex
    std::deque<xnetwork_bytes_message_t> m_send_queue{};            // sending queue

    asio::ip::udp::socket m_socket;

public:
    xtop_udp_socket(xtop_udp_socket const &)             = delete;
    xtop_udp_socket & operator=(xtop_udp_socket const &) = delete;
    xtop_udp_socket(xtop_udp_socket &&)                  = default;
    xtop_udp_socket & operator=(xtop_udp_socket &&)      = default;
    ~xtop_udp_socket() override                          = default;

    xtop_udp_socket(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                    std::uint16_t const port);

    xtop_udp_socket(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                    xendpoint_t const & endpoint);

    xendpoint_t
    local_endpoint() const noexcept override;

    void
    send_to(xendpoint_t const & peer,
            xbyte_buffer_t data,
            xdeliver_property_t const & deliver_property) override;

    void
    start() override;

    void
    stop() override;

protected:
    void
    do_read();

    void
    do_write();
};

class xtop_basic_udp_socket : public std::enable_shared_from_this<xtop_basic_udp_socket>
                            , public xbasic_socket_t
{
private:
    xobject_ptr_t<xsocket_t<base::udp_t>> m_socket_ptr{ nullptr };

public:
    xtop_basic_udp_socket(xtop_basic_udp_socket const &)             = delete;
    xtop_basic_udp_socket & operator=(xtop_basic_udp_socket const &) = delete;
    xtop_basic_udp_socket(xtop_basic_udp_socket &&)                  = default;
    xtop_basic_udp_socket & operator=(xtop_basic_udp_socket &&)      = default;
    ~xtop_basic_udp_socket() override                                = default;

    xtop_basic_udp_socket(std::shared_ptr<xbase_io_context_wrapper_t> io_wrapper,
                          std::uint16_t const port);

    xtop_basic_udp_socket(std::shared_ptr<xbase_io_context_wrapper_t> io_wrapper,
                          network::xendpoint_t const & endpoint);

    void
    start() override final;

    void
    stop() override final;

    network::xendpoint_t
    local_endpoint() const noexcept override final;

    void
    send_to(xendpoint_t const & peer,
            xbyte_buffer_t data,
            xdeliver_property_t const & deliver_property) override final;

private:
    std::int32_t
    on_receive(std::uint64_t,
               std::uint64_t,
               std::uint64_t,
               std::uint64_t,
               base::xpacket_t &,
               std::int32_t,
               std::chrono::milliseconds,
               base::xendpoint_t *);
};

using xudp_socket_t = xtop_udp_socket;
using xbasic_udp_socket_t = xtop_basic_udp_socket;

NS_END2
