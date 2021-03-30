// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xnode_id.h"

#include <functional>

NS_BEG2(top, network)

using xnetwork_message_ready_callback_t = std::function<void(common::xnode_id_t const &, xbyte_buffer_t const &)>;

NS_END2
