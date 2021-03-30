// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xbasic_socket.h"

#include <cassert>

NS_BEG2(top, network)

void
xtop_basic_socket::start() {
    assert(!running());
    running(true);
    assert(running());
}

void
xtop_basic_socket::stop() {
    assert(running());
    running(false);
    assert(!running());
}

void
xtop_basic_socket::register_data_ready_notify(xsocket_data_ready_callback_t callback) {
    assert(!running());

    std::lock_guard<std::mutex> lock{ m_data_ready_callback_mutex };
    assert(m_data_ready_callback == nullptr);
    m_data_ready_callback = std::move(callback);
}

void
xtop_basic_socket::unregister_data_ready_notify() {
    assert(!running());

    std::lock_guard<std::mutex> lock{ m_data_ready_callback_mutex };
    assert(m_data_ready_callback != nullptr);
    m_data_ready_callback = nullptr;
}

NS_END2
