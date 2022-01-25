// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, error)

template <typename ExceptionT>
void throw_exception(ExceptionT eh) {
    throw std::move(eh);
}

NS_END2
