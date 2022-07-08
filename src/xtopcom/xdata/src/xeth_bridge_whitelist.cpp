// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xeth_bridge_whitelist.h"

#include "xbase/xcxx_config.h"

namespace top {
namespace data {

#if defined(XBUILD_DEV)

XINLINE_CONSTEXPR char const * g_eth_bridge_whitelist =
    R"T(
{
    "whitelist": [
        "0xf8a1e199c49c2ae2682ecc5b4a8838b39bab1a38"
    ]
}
)T";

#elif defined(XBUILD_CI)

XINLINE_CONSTEXPR char const * g_eth_bridge_whitelist =
    R"T(
{
    "whitelist": [
        "0xf8a1e199c49c2ae2682ecc5b4a8838b39bab1a38"
    ]
}
)T";

#elif defined(XBUILD_GALILEO)

XINLINE_CONSTEXPR char const * g_eth_bridge_whitelist =
    R"T(
{
    "whitelist": [
    ]
}
)T";

#elif defined(XBUILD_BOUNTY)

XINLINE_CONSTEXPR char const * g_eth_bridge_whitelist =
    R"T(
{
    "whitelist": [
    ]
}
)T";

#else

XINLINE_CONSTEXPR char const * g_eth_bridge_whitelist =
    R"T(
{
    "whitelist": [
    ]
}
)T";

#endif

const char * eth_bridge_whitelist() {
    return g_eth_bridge_whitelist;
}

}  // namespace data
}  // namespace top
