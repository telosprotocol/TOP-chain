// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "topcl.h"
#include "user_info.h"
#include "xconfig/xpredefined_configurations.h"

#include <string>

#if defined XBUILD_GALILEO
#    define SEED_URL ""
#    define SERVER_HOST_PORT_HTTP ""
#    define SERVER_HOST_PORT_WS ""
#else
#    define SEED_URL ""
#    define SERVER_HOST_PORT_HTTP ""
#    define SERVER_HOST_PORT_WS ""
#endif

#define SDK_VERSION "1.0"
#define SERVER_ERROR "Server Connection Error"

constexpr uint64_t MinDeposit = 1;
constexpr uint64_t MinTxDeposit = 100000;
//constexpr uint64_t TOP_UNIT = 1e6;  // 1TOP = 1e6 uTOP
//#define ASSET_TOP(num) ((uint64_t)((num)*TOP_UNIT))
const std::string TOP_ACCOUNT_PREFIX = "T00000";

extern xChainSDK::user_info g_userinfo;
extern xChainSDK::user_info copy_g_userinfo;
extern std::string g_keystore_dir;
extern std::string g_data_dir;
extern std::string g_pw_hint;
extern std::string g_server_host_port;
extern std::string g_edge_domain;
extern std::atomic_bool auto_query;
