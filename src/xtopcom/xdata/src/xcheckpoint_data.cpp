// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xcheckpoint_state.h"

#include "xbase/xcxx_config.h"

namespace top {
namespace data {

#if defined(CHECKPOINT_TEST)

XINLINE_CONSTEXPR char const * g_checkpoint_state =
    R"T(
{
}
)T";

#elif defined(XBUILD_CI) || defined(XBUILD_DEV)

XINLINE_CONSTEXPR char const * g_checkpoint_state =
    R"T(
{
}
)T";

#elif defined(XBUILD_GALILEO)

XINLINE_CONSTEXPR char const * g_checkpoint_state =
    R"T(
{
}
)T";

#else

XINLINE_CONSTEXPR char const * g_checkpoint_state =
    R"T(
{
}
)T";

#endif

const char * checkpoint_state() {
    return g_checkpoint_state;
}

}  // namespace data
}  // namespace top