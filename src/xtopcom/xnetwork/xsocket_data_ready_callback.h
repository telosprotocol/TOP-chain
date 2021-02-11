// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xnetwork/xendpoint.h"

#include <functional>

NS_BEG2(top, network)

using xsocket_data_ready_callback_t = std::function<void(xendpoint_t const &, xbyte_buffer_t const &)>;

NS_END2
