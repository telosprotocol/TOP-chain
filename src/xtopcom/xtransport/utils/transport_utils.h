// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>

namespace top {

namespace transport {

enum TransportErrorCode {
    kTransportSuccess = 0,
    kTransportFailed = 1,

    kUdpSocketStatusCanceled = 40,
    kUdpSocketStatusNull = 41,
    kUdpSocketStatusConnected = 42,
    kUdpSocketStatusNotConnected = 43,
};

}  // namespace transport

}  // namespace top
