// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xsys_utl.h"
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
#include <gtest/gtest.h>

using namespace top;

class test_sys_utl : public top::xsys_utl_t {
 public:

};

void create_a_file(const std::string & file_path) {
    FILE * fp = fopen(file_path.c_str(), "w");
    if (fp == nullptr) {
        fprintf(stderr, "create file failed, path(%s), detail(%d:%s)\n", file_path.c_str(), errno, strerror(errno));
        return;
    }
    fclose(fp);
}

void ceate_a_directory(const std::string & dir_path) {
    if (mkdir(dir_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
        fprintf(stderr, "create directory failed, path(%s), detail(%d:%s)\n", dir_path.c_str(), errno, strerror(errno));
        return;
    }
}

void get_log_file_num(const std::string & log_path, int & log_file_num, int & metric_file_num) {
    log_file_num = 0;
    metric_file_num = 0;
    DIR * dir = opendir(log_path.c_str());
    if (dir == nullptr) {
        fprintf(stderr, "open directory failed, path(%s), detail(%d:%s)\n", log_path.c_str(), errno, strerror(errno));
        return;
    }
    struct dirent * ptr;
    while ((ptr = readdir(dir)) != nullptr) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        }
        if (strstr(ptr->d_name, "xtop.") != nullptr) {
            log_file_num++;
        } else if (strstr(ptr->d_name, "xmetric.") != nullptr) {
            metric_file_num++;
        }
    }
    closedir(dir);
}

TEST(xbasic, sort_log_file_names) {
    std::set<std::string> log_file_names;
    log_file_names.insert("xtop.2023-04-18-061642-1-97612.log");
    log_file_names.insert("xtop.2023-04-18-061708-2-97612.log");
    log_file_names.insert("xtop.2023-04-18-061805-3-97612.log");
    log_file_names.insert("xtop.2023-04-18-061909-4-97612.log");
    log_file_names.insert("xtop.2023-04-20-061909-4-97612.log");
    log_file_names.insert("xtop.2024-04-20-061909-4-97612.log");
    log_file_names.insert("xtop.2023-02-20-061909-4-97612.log");

    assert(log_file_names.size() == 7);
    std::cout << "before sort:" << std::endl;
    for (auto & log_file_name : log_file_names) {
        std::cout << log_file_name << std::endl;
    }   
}


// create a log directory and create some log and metrics files
void create_log_directory_and_log_metrics_files(std::string const& log_path, int log_file_num, int metrics_file_num) {
    ceate_a_directory(log_path);

    {
        int year = 2023;
        int month = 4;
        int day = 18;
        int hour = 6;
        int minute = 16;
        int second = 42;
        int num1 = 1;
        int num2 = 97612;
        char filename[50];

        for (int i = 1; i <= log_file_num; ++i) {
            if (i % 60 == 0) year++;
            if (i % 50 == 0) month++;
            if (i % 40 == 0) day++;
            if (i % 30 == 0) hour++;
            if (i % 20 == 0) minute++;
            if (i % 10 == 0) second++;
            num1++;
            
            sprintf(filename, "xtop.%d-%02d-%02d-%02d%02d%02d-%d-%d.log", year, month, day, hour, minute, second, num1, num2);
            std::string log_file_name{filename};        
            create_a_file(log_path + "/" + log_file_name);
        }
    }

    {
        int year = 2023;
        int month = 4;
        int day = 18;
        int hour = 6;
        int minute = 16;
        int second = 42;
        int num1 = 1;
        int num2 = 97612;
        char filename[50];        
        for (int i = 1; i <= metrics_file_num; ++i) {
            if (i % 60 == 0) year++;
            if (i % 50 == 0) month++;
            if (i % 40 == 0) day++;
            if (i % 30 == 0) hour++;
            if (i % 20 == 0) minute++;
            if (i % 10 == 0) second++;
            num1++;
            
            sprintf(filename, "xmetric.%d-%02d-%02d-%02d%02d%02d-%d-%d.log", year, month, day, hour, minute, second, num1, num2);
            std::string log_file_name{filename};        
            create_a_file(log_path + "/" + log_file_name);
        }        
    }

    create_a_file(log_path + "/" + "xmetric.log");
    create_a_file(log_path + "/" + "xtop.log");
    create_a_file(log_path + "/" + "temp.log");
    create_a_file(log_path + "/" + "other.log");
}

TEST(xbasic, sys_utl_space_BENCH) {
    std::string log_path = "test_sys_utl_space";
    create_log_directory_and_log_metrics_files(log_path, 100, 100);
    
    int available_space = test_sys_utl::get_log_directory_available_space_GB(log_path);
    std::cout << "available_space:" << available_space << std::endl;
    ASSERT_GT(available_space, 0);
}
TEST(xbasic, remove_old_logs_metrics_BENCH) {
    std::string log_path = "test_sys_utl_remove_old_logs_metrics";
    create_log_directory_and_log_metrics_files(log_path, 100, 100);

    int log_file_num = 0;
    int metric_file_num = 0;
    get_log_file_num(log_path, log_file_num, metric_file_num);
    ASSERT_EQ(log_file_num, 100+1);
    ASSERT_EQ(metric_file_num, 100+1);
    sleep(100+1);
    xsys_utl_t::check_and_remove_old_log_files(log_path, 200, 200);
    get_log_file_num(log_path, log_file_num, metric_file_num);
    ASSERT_EQ(log_file_num, 100+1);
    ASSERT_EQ(metric_file_num, 100+1);

    xsys_utl_t::check_and_remove_old_log_files(log_path, 100, 100);
    get_log_file_num(log_path, log_file_num, metric_file_num);
    ASSERT_EQ(log_file_num, 1);
    ASSERT_EQ(metric_file_num, 1);
}
