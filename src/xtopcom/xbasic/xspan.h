// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#if defined(XCXX20)

#include <span>

NS_BEG1(top)

template <typename T, size_t Extent = std::dynamic_extent>
using xspan_t = std::span<T, Extent>;

NS_END1

#else

#include <gsl/span>

NS_BEG1(top)

template <typename T, std::size_t Extent = gsl::dynamic_extent>
using xspan_t = gsl::span<T, Extent>;

NS_END1

#endif
