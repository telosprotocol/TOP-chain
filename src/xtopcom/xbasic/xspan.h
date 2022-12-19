// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xcxx_config.h"

#if defined(XCXX20_OR_ABOVE)

#include <span>

template <typename T, std::size_t Extent = std::dynamic_extent>
using xspan_t = std::span<T, Extent>;

#else

#include <gsl/span>

template <typename T, std::size_t Extent = gsl::dynamic_extent>
using xspan_t = gsl::span<T, Extent>;

#endif
