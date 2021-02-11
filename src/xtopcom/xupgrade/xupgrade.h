// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef _XUPGRADE_H
#define _XUPGRADE_H
#include <string>
struct upgrade_config_t {
    uint16_t enable_upgrade;
    uint16_t upgrade_mode;  // 0 for xnode, 1 for xchain
    std::string https_addr;
    uint32_t check_interval;
    uint16_t cmd_port;
    uint16_t xchain_port;
    std::string xchain_dir;
    std::string local_ip;
};
#endif
