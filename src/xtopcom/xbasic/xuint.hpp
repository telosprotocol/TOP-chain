// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/uint128_t.h"
#include "xbasic/xbyte.h"
#include "xbasic/xhash.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#pragma GCC diagnostic pop


NS_BEG1(top)



NS_END1

#include "xbase/xmem.h"

int32_t operator<<(top::base::xbuffer_t & stream, const uint128_t & value);
int32_t operator>>(top::base::xbuffer_t & stream, uint128_t & value);
