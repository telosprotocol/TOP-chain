// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

NS_BEG1(top)

template <typename ExceptionT>
void
throw_exception(ExceptionT const & eh) {
    throw eh;
}

NS_END1