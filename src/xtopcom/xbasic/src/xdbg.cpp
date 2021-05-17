// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xdbg.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#include "xbase/xbase.h"

NS_BEG1(top)

#define STACK_LEN 5

std::string xdbg_helper::demangle(const char * symbol) {
    size_t size;
    int status;
    char temp[128] = {0};
    char * demangled;

    // first, try to demangle a c++ name
    if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
        if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
            std::string result(demangled);
            free(demangled);
            return result;
        }
    }

    // if that didn't work, try to get a regular c symbol
    if (1 == sscanf(symbol, "%127s", temp)) {
        return temp;
    }

    // if all else fails, just return the symbol
    return symbol;
}

void xdbg_helper::print_trace(void * p, const char * op) {
    void * array[STACK_LEN];
    size_t size;
    char ** func_name_cache;
    size = backtrace(array, STACK_LEN);
    func_name_cache = backtrace_symbols(array, size);
    for (size_t i = 1; i < size; i++) {
        xinfo("%p-%s[%d]-%s", p, op, i, demangle(func_name_cache[i]).c_str());
    }
    free(func_name_cache);
}

std::vector<std::string> xdbg_helper::get_trace() {
    std::vector<std::string> traces;
    void * array[STACK_LEN];
    size_t size;
    char ** func_name_cache;
    size = backtrace(array, STACK_LEN);
    func_name_cache = backtrace_symbols(array, size);
    for (size_t i = 1; i < size; i++) {
        //xinfo("%p-%s[%d]-%s", p, op, i, demangle(func_name_cache[i]).c_str());
        traces.push_back(demangle(func_name_cache[i]));
    }
    free(func_name_cache);
    return traces;
}

NS_END1
