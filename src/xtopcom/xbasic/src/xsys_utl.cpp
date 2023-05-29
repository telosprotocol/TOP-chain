// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xsys_utl.h"
#include <unistd.h>
#include <assert.h>
#include <chrono>
#include <cstdio>
#include <dirent.h>
#include <iostream>
#include "string.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "xbase/xlog.h"
#ifdef __APPLE__
#include <sys/sysctl.h>
#else
#include <sys/sysinfo.h>
#endif

NS_BEG1(top)

int xsys_utl_t::get_log_directory_available_space_GB(std::string const & log_path) {
    struct statvfs stat;
    if (statvfs(log_path.c_str(), &stat) != 0) {
        fprintf(stderr, "xsys_utl_t::get_log_directory_available_space_GB statvfs failed, path(%s), detail(%d:%s)\n", log_path.c_str(), errno, strerror(errno));
        return -1;
    }

    // the available size is f_bsize * f_bavail
    int64_t available_size = stat.f_bsize * stat.f_bavail;
    return available_size / (1024 * 1024 * 1024);
}

int xsys_utl_t::check_and_remove_old_log_file(std::string const & full_log_file_path, int threshold_timestamp_s) {
    // read file's last modified time
    struct stat file_stat;
    if (stat(full_log_file_path.c_str(), &file_stat) != 0) {
        fprintf(stderr, "xsys_utl_t::check_and_remove_old_log_file stat failed, path(%s), detail(%d:%s)\n", full_log_file_path.c_str(), errno, strerror(errno));
        return -1;
    }

    // remove file before threshold_sec
    if (file_stat.st_mtime < threshold_timestamp_s) {
        if (remove(full_log_file_path.c_str()) != 0) {
            fprintf(stderr, "xsys_utl_t::check_and_remove_old_log_file remove failed, path(%s), detail(%d:%s)\n", full_log_file_path.c_str(), errno, strerror(errno));
            return -1;
        } else {
            xdbg("xsys_utl_t::check_and_remove_old_log_file remove success, path(%s)", full_log_file_path.c_str());
            return 1; // return 1 means file removed
        }
    }

    return 0;
}

int xsys_utl_t::check_and_remove_old_log_files(std::string const & log_path, int log_keep_seconds, int metric_keep_seconds) {
    struct timeval now;
    gettimeofday(&now, nullptr);
    int now_sec = now.tv_sec;
    if (now_sec < log_keep_seconds || now_sec < metric_keep_seconds) {
        fprintf(stderr, "xsys_utl_t::check_and_remove_old_log_files now_sec(%d) < log_keep_seconds(%d) or metric_keep_seconds(%d)", now_sec, log_keep_seconds, metric_keep_seconds);
        return -1;
    }
    int remove_log_timestamp_s = now_sec - log_keep_seconds;
    int remove_metric_timestamp_s = now_sec - metric_keep_seconds;

    xinfo("xsys_utl_t::check_and_remove_old_log_files begin, log_path(%s),log_keep_seconds(%d),metric_keep_seconds(%d),remove_log_timestamp_s(%d),remove_metric_timestamp_s(%d)", 
        log_path.c_str(), log_keep_seconds, metric_keep_seconds, remove_log_timestamp_s, remove_metric_timestamp_s);

    // get all files in log_path
    DIR * dir = opendir(log_path.c_str());
    if (dir == nullptr) {
        fprintf(stderr, "xsys_utl_t::check_and_remove_old_log_files opendir failed, path(%s), detail(%d:%s)\n", log_path.c_str(), errno, strerror(errno));
        return -1;
    }

    int removed_log_file_count = 0;
    int removed_metric_file_count = 0;

    struct dirent * entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (DT_REG == entry->d_type || DT_UNKNOWN == entry->d_type) {
            if (strstr(entry->d_name, ".log") == nullptr) {
                continue;
            }

            if (strncmp(entry->d_name, "xtop.", 5) == 0 && strlen(entry->d_name) > sizeof("xtop.log")) {
                // the log file sample: "xtop.2019-07-04-144108-1-17497.log", should skip xtop.log
                std::string full_file_path = log_path + "/" + entry->d_name;
                if (check_and_remove_old_log_file(full_file_path, remove_log_timestamp_s) > 0)  {
                    removed_log_file_count++;
                } 
            } else if (strncmp(entry->d_name, "xmetric.", 8) == 0 && strlen(entry->d_name) > sizeof("xmetric.log")) {
                // the metrics file sample: "xmetric.2023-04-16-161031-92-7499.log", should skip xmetric.log
                std::string full_file_path = log_path + "/" + entry->d_name;
                if (check_and_remove_old_log_file(full_file_path, remove_metric_timestamp_s) > 0)  {
                    removed_metric_file_count++;
                }
            }
        }
    }

    closedir(dir);

    xinfo("xsys_utl_t::check_and_remove_old_log_files finish.log_path(%s),removed_log_file_count(%d),removed_metric_file_count(%d)", 
        log_path.c_str(), removed_log_file_count, removed_metric_file_count);
    return 0;
}

uint64_t xsys_utl_t::get_total_memory() {
    uint64_t total_ram = 0;
#ifdef __APPLE__
    size_t len = sizeof(total_ram);
    int ret = sysctlbyname("hw.memsize", &total_ram, &len, NULL, 0);
    if (ret != 0)
    {
        xwarn("xdb_impl macos sysctlbyname of hw.memsize failed!");
    }
#else
    struct sysinfo si;
    sysinfo(&si);
    total_ram = si.totalram;
#endif
    return total_ram;
}

NS_END1