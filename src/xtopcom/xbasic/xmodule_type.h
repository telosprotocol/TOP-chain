// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <unordered_map>

namespace top { namespace chainbase {

#define xsuccess (0)
#define xfailed (1)

enum enum_xmodule_type {
    // xtopcom use 0x00100000, xchain use 0x00200000
    xmodule_type_xcontract_runtime    = 0x00210000,
    xmodule_type_xstore               = 0x00220000,
    xmodule_type_xsync                = 0x00230000,
    xmodule_type_xelect               = 0x00240000,
    xmodule_type_xrouter              = 0x00250000,
    xmodule_type_xvm                  = 0x00260000,
    xmodule_type_xconsensus           = 0x00270000,
    xmodule_type_xunit                = 0x00280000,
    xmodule_type_xtableblock          = 0x00290000,
    xmodule_type_xbookblock           = 0x002A0000,
    xmodule_type_xzoneblock           = 0x002B0000,
    xmodule_type_xdata                = 0x002C0000,
    xmodule_type_xverifier            = 0x002D0000,
    xmodule_type_xtxpool              = 0x002E0000,
    xmodule_type_xtxexecutor          = 0x002F0000,
    xmodule_type_property             = 0x00310000,

};

typedef std::string (*xmodule_log_print_fun_t)(int32_t error_code);

class xmodule_log_mgr {
 public:
    static xmodule_log_mgr & get_instance() {
        static xmodule_log_mgr mgr;
        return mgr;
    }
    void register_error_code_to_str_fun(enum_xmodule_type type, xmodule_log_print_fun_t fun, int32_t min, int32_t max);
    std::string xmodule_error_to_str(int32_t error_code);

 private:
    std::unordered_map<int32_t, xmodule_log_print_fun_t> g_xmodule_log_fun_map;
};

// register xobject
class auto_register_xmodule_log {
 public:
    auto_register_xmodule_log(enum_xmodule_type type, xmodule_log_print_fun_t fun, int32_t min, int32_t max) {
        xmodule_log_mgr::get_instance().register_error_code_to_str_fun(type, fun, min, max);
    }
};

#ifndef REG_XMODULE_LOG
#define REG_XMODULE_LOG(module_type, error_to_string_fun, error_code_min, error_code_max) \
    using chainbase::auto_register_xmodule_log;\
    using chainbase::xmodule_log_print_fun_t;\
    using chainbase::xmodule_log_mgr;\
    static chainbase::auto_register_xmodule_log g_module_log{module_type, error_to_string_fun, error_code_min, error_code_max};
#endif  // REG_XMODULE_LOG


std::string xmodule_error_to_str(int32_t error_code);

}  // namespace chainbase
}  // namespace top
