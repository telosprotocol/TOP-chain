// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <set>
#include "xbase/xns_macro.h"

NS_BEG1(top)

class xsys_utl_t {
 public:
    static int    get_log_directory_available_space_GB(std::string const & log_path);
    static int    check_and_remove_old_log_file(std::string const & full_log_file_path, int threshold_timestamp_s);
    static int    check_and_remove_old_log_files(std::string const & log_path, int log_keep_seconds, int metric_keep_seconds);

    static uint64_t get_total_memory();
};


NS_END1