// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xnetwork/xasio_extension.h"
#include "xnetwork/xnetwork_driver.h"
#include "xnetwork/xudp_socket.h"
#include "xnetwork/xutility.hpp"

#include <functional>

NS_BEG2(top, network)

xtop_udp_socket::xtop_udp_socket(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                                 std::uint16_t const port)
    : m_socket{ io_object->create<asio::ip::udp::socket>() }
{
    m_socket.open(asio::ip::udp::v4());
    m_socket.non_blocking(true);

#ifdef __APPLE__
    asio::socket_base::receive_buffer_size const rbs{ 838860 };
#else
    asio::socket_base::receive_buffer_size const rbs{ 8388608 };
#endif
    m_socket.set_option(rbs);

    socket_base::reuse_port const reuse_port{ true };
    m_socket.set_option(reuse_port);

    m_socket.bind({ asio::ip::udp::v4(), port });
}

xtop_udp_socket::xtop_udp_socket(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
                                 xendpoint_t const & endpoint)
    : m_socket{ io_object->create<asio::ip::udp::socket>() }
{
    m_socket.open(asio::ip::udp::v4());
    m_socket.non_blocking(true);

#ifdef __APPLE__
    asio::socket_base::receive_buffer_size const rbs{ 838860 };
#else
    asio::socket_base::receive_buffer_size const rbs{ 8388608 };
#endif
    m_socket.set_option(rbs);

    socket_base::reuse_port const reuse_port{ true };
    m_socket.set_option(reuse_port);

    m_socket.bind(convert_to<asio::ip::udp::endpoint>::from(endpoint));
}


xendpoint_t
xtop_udp_socket::local_endpoint() const noexcept {
    try {
        return convert_to<xendpoint_t>::from(m_socket.local_endpoint());
    } catch (std::exception const & eh) {
        xerror("[udp socket] %s", eh.what());
    } catch (...) {
        xerror(u8"[udp socket] unknown error");
    }

    return xendpoint_t{};
}

void
xtop_udp_socket::start() {
    base_t::start();

    do_read();
}

void
xtop_udp_socket::stop() {
    base_t::stop();

    try {
        m_socket.close();
    } catch (asio::system_error const & eh) {
        std::throw_with_nested(std::runtime_error{ "xudp_socket" });
    }
}

void
xtop_udp_socket::do_read() {
    if (!running()) {
        xwarn("[udp socket] read: not running");
        return;
    }

    auto self = shared_from_this();
    m_socket.async_receive_from(asio::buffer(m_recv_buffer), m_sender_endpoint,
                                [this, self](asio::error_code const & ec, std::size_t len)
    {
        //xkinfo("[socket] received from %s:%hu, length %zu", m_sender_endpoint.address().to_string().c_str(), m_sender_endpoint.port(), len);

        do {
            if (!running()) {
                xwarn("[udp socket] async_receive_from: not running");
                return;
            }

            if (ec && ec != asio::error::operation_aborted) {
                xwarn("[udp socket] receiving UDP message failed. ec:%d error msg:%s\n", ec.value(), ec.message().c_str());
                break;
            }

            if (len) {
                assert(m_data_ready_callback);

                auto const data_begin = std::begin(m_recv_buffer);
                auto const data_end = std::next(data_begin, len);

                auto const endpoint = convert_to<xendpoint_t>::from(m_sender_endpoint);
                m_data_ready_callback(endpoint, xbyte_buffer_t{ data_begin, data_end });
            }
        } while (false);

        do_read();
    });
}

void
xtop_udp_socket::do_write() {
    if (!running()) {
        return;
    }

    auto const & bytes_message = m_send_queue.front();

    auto self = shared_from_this();
    m_socket.async_send_to(asio::buffer(bytes_message.payload()), convert_to<asio::ip::udp::endpoint>::from(bytes_message.endpoint()),
                           [this, self](std::error_code ec, std::size_t)
    {
        std::lock_guard<std::mutex> lock{ m_send_queue_mutex };
        if (ec) {
            if (ec != asio::error::operation_aborted) {
                xwarn("[udp socket] failed delivering UDP message. ec: %d error msg:%s package size:%zu", ec.value(), ec.message().c_str(), m_send_queue.front().payload().size());
            } else {
                xwarn("[udp socket] %s", ec.message().c_str());
                return;
            }
        }

        if (!ec) {
            m_send_queue.pop_front();
            if (m_send_queue.empty()) {
                return;
            }
        }

        do_write();
    });
}

void
xtop_udp_socket::send_to(xendpoint_t const & peer,
                         xbyte_buffer_t data,
                         xdeliver_property_t const &) {
    if (!running()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_send_queue_mutex);
    assert(data.size() <= pack_size_upper_limit);
    m_send_queue.push_back({ std::move(data), peer });

    if (m_send_queue.size() == 1) {
        do_write();
    }
}

xtop_basic_udp_socket::xtop_basic_udp_socket(std::shared_ptr<xbase_io_context_wrapper_t> io_wrapper,
                                             std::uint16_t const port)
{
    auto tmp_port = port;
    m_socket_ptr.attach(new xsocket_t<base::udp_t>(io_wrapper->context(), nullptr, io_wrapper->thread_id(), base::xsocket_utl::udp_listen(u8"0.0.0.0", tmp_port)));
}

xtop_basic_udp_socket::xtop_basic_udp_socket(std::shared_ptr<xbase_io_context_wrapper_t> io_wrapper,
                                             network::xendpoint_t const & endpoint)
{
    auto tmp_port = endpoint.port();
    m_socket_ptr.attach(new xsocket_t<base::udp_t>{ io_wrapper->context(), nullptr, io_wrapper->thread_id(), base::xsocket_utl::udp_listen(endpoint.address(), tmp_port) });
}

void
xtop_basic_udp_socket::start() {
    m_socket_ptr->register_on_receive_callback(std::bind(&xtop_basic_udp_socket::on_receive,
                                                         shared_from_this(),
                                                         std::placeholders::_1,
                                                         std::placeholders::_2,
                                                         std::placeholders::_3,
                                                         std::placeholders::_4,
                                                         std::placeholders::_5,
                                                         std::placeholders::_6,
                                                         std::placeholders::_7,
                                                         std::placeholders::_8));
    xbasic_socket_t::start();

    m_socket_ptr->start_read(0);
}

void
xtop_basic_udp_socket::stop() {
    m_socket_ptr->stop_read(0);
    xbasic_socket_t::stop();
    m_socket_ptr->unregister_on_receiver_callback();
}

void
xtop_basic_udp_socket::send_to(xendpoint_t const & peer,
                               xbyte_buffer_t data,
                               network::xdeliver_property_t const & /*deliver_property*/) {
    base::xpacket_t packet;
    packet.get_body().push_back(data.data(), static_cast<std::uint32_t>(data.size()));
    packet.set_to_ip_addr(peer.address());
    packet.set_to_ip_port(peer.port());

    if (m_socket_ptr->send(0, 0, 0, 0, packet, 0, 0, nullptr) != enum_xcode_successful) {
        // todo log error;
    }
}

network::xendpoint_t
xtop_basic_udp_socket::local_endpoint() const noexcept {
    network::xendpoint_t endpoint{ m_socket_ptr->get_local_ip_address(), m_socket_ptr->get_local_real_port() };
    return endpoint;
}

std::int32_t
xtop_basic_udp_socket::on_receive(std::uint64_t,
                                  std::uint64_t,
                                  std::uint64_t,
                                  std::uint64_t,
                                  base::xpacket_t & packet,
                                  std::int32_t,
                                  std::chrono::milliseconds,
                                  base::xendpoint_t *) {
    xsocket_data_ready_callback_t callback;
    XLOCK_GUARD(m_data_ready_callback_mutex) {
        if (m_data_ready_callback != nullptr) {
            callback = m_data_ready_callback;
        }
    }

    // xendpoint_t const &, xbyte_buffer_t const &

    if (callback) {
        try {
            auto const & packet_body = packet.get_body();
            xbyte_buffer_t bytes{ packet_body.data() + enum_xip2_header_len, packet_body.data() + packet_body.size() };
            callback({ packet.get_from_ip_addr(), (uint16_t)packet.get_from_ip_port() }, bytes);
        } catch (std::exception const & eh) {
            return -1;
        }
    }

    return 0;
}

NS_END2
