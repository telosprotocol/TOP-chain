// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xsocket_data_ready_callback.h"
#include "xnetwork/xsocket_face.h"

#include <atomic>
#include <mutex>

NS_BEG2(top, network)

class xtop_basic_socket : public xsocket_face_t
{
protected:
    mutable std::mutex m_data_ready_callback_mutex{};
    xsocket_data_ready_callback_t m_data_ready_callback{};

public:
    xtop_basic_socket()                                      = default;
    xtop_basic_socket(xtop_basic_socket const &)             = delete;
    xtop_basic_socket & operator=(xtop_basic_socket const &) = delete;
    xtop_basic_socket(xtop_basic_socket &&)                  = default;
    xtop_basic_socket & operator=(xtop_basic_socket &&)      = default;
    ~xtop_basic_socket() override                            = default;

    void
    start() override;

    void
    stop() override;

    void
    register_data_ready_notify(xsocket_data_ready_callback_t callback) override final;

    void
    unregister_data_ready_notify() override final;
};

using xbasic_socket_t = xtop_basic_socket;

NS_END2
