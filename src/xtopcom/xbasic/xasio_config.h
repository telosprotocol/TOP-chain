// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#if !defined ASIO_STANDALONE
# define ASIO_STANDALONE
#endif

#if !defined ASIO_HAS_STD_CHRONO
# define ASIO_HAS_STD_CHRONO
#endif

#if !defined ASIO_HAS_STD_SYSTEM_ERROR
# define ASIO_HAS_STD_SYSTEM_ERROR
#endif
