// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbasic/uint128_t.h"

int32_t operator<<(top::base::xbuffer_t & stream, const uint128_t & value);
int32_t operator>>(top::base::xbuffer_t & stream, uint128_t & value);
