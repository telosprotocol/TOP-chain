#pragma once

#include <string>
#include <vector>

namespace top
{
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int32_t ntp_sync(const std::vector<std::string>& ntp_server_list);

void check_log_path(const std::string& log_path);

int daemonize();

int drop_root(const char *username);

void get_groups();

/*
 *       soft  nofile   262144
 *       hard  nofile   262144
 *       soft  core     524288000
 *       soft  sigpending 65535
 *       hard  sigpending 65535
 */

int set_limits();

int get_disk_space(const std::string& compoment_path);

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus
}
