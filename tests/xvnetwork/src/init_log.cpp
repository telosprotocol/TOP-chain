// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xbase/xlog.h"

class init_logger final
{
public:
    init_logger() {
        xinit_log("/tmp/xtop.log", true, true);
        xset_log_level(enum_xlog_level::enum_xlog_level_debug);
    }
};

//init_logger init;
