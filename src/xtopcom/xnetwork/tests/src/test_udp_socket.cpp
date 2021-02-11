// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xnetwork/xudp_socket.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

TEST(xnetwork, udp_socket) {
    using namespace top;
    using namespace network;

    auto io_context_wrapper = std::make_shared<xasio_io_context_wrapper_t>();
    io_context_wrapper->async_start();

    std::shared_ptr<xsocket_face_t> socket1 = std::make_shared<xudp_socket_t>(io_context_wrapper, 10100);
    std::shared_ptr<xsocket_face_t> socket2 = std::make_shared<xudp_socket_t>(io_context_wrapper, 10101);

    std::string message_sent{ u8"have a nice test\n\n\n" };
    std::string message_recv;

    socket1->register_data_ready_notify([&message_recv](xendpoint_t const &,
                                                        xbyte_buffer_t const & bytes) {
        message_recv = { std::begin(bytes), std::end(bytes) };
    });

    socket1->start();
    socket2->start();

    socket2->send_to({ "0.0.0.0", 10100 },
                     { std::begin(message_sent), std::end(message_sent) },
                     xdeliver_property_t{});

    std::this_thread::sleep_for(std::chrono::seconds{1});

    EXPECT_EQ(message_recv, message_sent);

    assert(io_context_wrapper->running());
    io_context_wrapper->stop();
}
