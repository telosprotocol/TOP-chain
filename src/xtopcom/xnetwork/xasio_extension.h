// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_config.h"

#include <asio/socket_base.hpp>

#include <sys/socket.h>

NS_BEG2(top, network)

class socket_base final : public asio::socket_base
{
public:
    using reuse_port = asio::detail::socket_option::boolean<ASIO_OS_DEF(SOL_SOCKET), SO_REUSEPORT>;
};

NS_END2
