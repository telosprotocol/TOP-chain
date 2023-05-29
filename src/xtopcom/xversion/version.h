#pragma once

#include <string>

namespace top {

std::string get_md5();
void print_version();
std::string dump_version();
std::string get_program_version();
std::string get_git_log_latest();
std::string get_git_submodule();
std::string get_build_date_time();
std::string get_build_options();

}
