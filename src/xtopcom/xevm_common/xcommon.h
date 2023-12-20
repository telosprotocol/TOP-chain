// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xfixed_bytes.h"

#include <cstddef>

NS_BEG3(top, evm, common)

XINLINE_CONSTEXPR size_t BLS_PUBLIC_KEY_LEN{48};
XINLINE_CONSTEXPR size_t BLS_SIGNATURE_LEN{96};

using xbls_publick_key_t = xfixed_bytes_t<BLS_PUBLIC_KEY_LEN>;
using xbls_signature_t = xfixed_bytes_t<BLS_SIGNATURE_LEN>;


NS_END3
