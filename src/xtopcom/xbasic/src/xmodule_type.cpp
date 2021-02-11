// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <unordered_map>
#include <assert.h>
#include <iostream>

#include "xbasic/xmodule_type.h"
#include "xbase/xlog.h"

namespace top { namespace chainbase {

void xmodule_log_mgr::register_error_code_to_str_fun(enum_xmodule_type type, xmodule_log_print_fun_t fun, int32_t min, int32_t max) {
    xkinfo("module type register error str fun. type:0x%x", type);
    int32_t map_type = (int32_t)type;
    auto iter = g_xmodule_log_fun_map.find(map_type);
    if (iter != g_xmodule_log_fun_map.end()) {
        assert(0);
    }
    g_xmodule_log_fun_map[map_type] = fun;

    // test all error code
    for (int32_t code = min; code < max; code++) {
        std::string str = fun(code);
        assert(!str.empty());
    }
}

inline enum_xmodule_type get_module_type(int32_t error_code) {
    return (enum_xmodule_type)(error_code & 0xFFFF0000);
}

std::string xmodule_log_mgr::xmodule_error_to_str(int32_t error_code) {
    if (error_code == 0) {
        return "success";
    }

    enum_xmodule_type type = get_module_type(error_code);
    auto iter = xmodule_log_mgr::g_xmodule_log_fun_map.find(type);
    if (iter == xmodule_log_mgr::g_xmodule_log_fun_map.end()) {
        return std::to_string(error_code);
    }

    xmodule_log_print_fun_t fun = iter->second;
    return fun(error_code);
}

std::string xmodule_error_to_str(int32_t error_code) {
    return xmodule_log_mgr::get_instance().xmodule_error_to_str(error_code);
}



}  // namespace chainbase
}  // namespace top
