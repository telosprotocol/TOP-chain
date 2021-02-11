#include "global_definition.h"

xChainSDK::user_info g_userinfo;
xChainSDK::user_info copy_g_userinfo;
std::string g_keystore_dir{"/home/topcl/keystore"};
std::string g_data_dir{"/home/topcl"};
std::string g_pw_hint{""};
std::string g_server_host_port;
std::string g_edge_domain{SEED_URL};
std::atomic_bool auto_query{false};
