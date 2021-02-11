// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xendpoint.h"

#include <type_traits>

NS_BEG3(top, network, tests)

template <typename T>
xendpoint_t
loopback_endpoint(T const n,
                  std::uint16_t const port) {
    XSTATIC_ASSERT(std::is_integral<T>::value);
    return { u8"127.0.0." + std::to_string(n), port };
}

template <typename T>
xendpoint_t
loopback_endpoint(T const n1,
                  T const n2,
                  std::uint16_t const port) {
    XSTATIC_ASSERT(std::is_integral<T>::value);
    return { u8"127.0." + std::to_string(n1) + u8"." + std::to_string(n2), port };
}

template <typename T>
xendpoint_t
loopback_endpoint(T const n1,
                  T const n2,
                  T const n3,
                  std::uint16_t const port) {
    XSTATIC_ASSERT(std::is_integral<T>::value);
    return { u8"127." + std::to_string(n1) + u8"." + std::to_string(n2) + u8"." + std::to_string(n3), port };
}

NS_END3
