// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xns_macro.h"
#include <string>
#include <vector>
NS_BEG1(top)

class xdbg_helper {
    static std::string demangle(const char * symbol);

public:
    static void print_trace(void * p, const char * op);
    static std::vector<std::string> get_trace();
};

#if defined XENABLE_PSTACK
# define P_STACK(p, tag) xdbg_helper::print_trace(p, tag);
#else
# define P_STACK(p, tag)
#endif  // DEBUG

NS_END1
