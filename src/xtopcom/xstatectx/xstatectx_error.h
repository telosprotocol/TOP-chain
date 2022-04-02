// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <assert.h>

#include "xbasic/xmodule_type.h"

namespace top { namespace statectx {

enum enum_statectx_error_type {
    enum_statectx_error_base = chainbase::enum_xmodule_type::xmodule_type_xstatectx,

    enum_statectx_error_max,
};

#define XSTORE_TO_STR(val) #val

inline std::string xstatectx_error_to_string(int32_t code) {
    assert(code > enum_statectx_error_base && code < enum_statectx_error_max);
    static const char* names[] = {

    };
    return names[code - enum_statectx_error_base - 1];
}

}  // namespace statectx
}  // namespace top
