// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xcrosschain_whitelist.h"

#include "xbase/xcxx_config.h"

namespace top {
namespace data {

#if defined(XBUILD_DEV)

XINLINE_CONSTEXPR char const * g_crosschain_whitelist =
    R"T(
{
    "whitelist": [
        "0xf8a1e199c49c2ae2682ecc5b4a8838b39bab1a38"
    ]
}
)T";

#elif defined(XBUILD_CI)

XINLINE_CONSTEXPR char const * g_crosschain_whitelist =
    R"T(
{
    "whitelist": [
        "0xf8a1e199c49c2ae2682ecc5b4a8838b39bab1a38"
    ]
}
)T";

#elif defined(XBUILD_GALILEO)

XINLINE_CONSTEXPR char const * g_crosschain_whitelist =
    R"T(
{
    "whitelist": [
    ]
}
)T";

#elif defined(XBUILD_BOUNTY)

XINLINE_CONSTEXPR char const * g_crosschain_whitelist =
    R"T(
{
    "whitelist": [
    ]
}
)T";

#else

XINLINE_CONSTEXPR char const * g_crosschain_whitelist =
    R"T(
{
    "whitelist": [
        "0x8b587045c7fcd6faddf022fdf8e09756f2fd6cc6",
        "0x0dbbae16e5afb599e0243b0f339431b9f73c7a75"
    ]
}
)T";

#endif

const char * crosschain_whitelist() {
    return g_crosschain_whitelist;
}

}  // namespace data
}  // namespace top
