// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <memory>
#include <string>

class backup_list_info_impl;
class backup_list_info {
private:
    std::shared_ptr<backup_list_info_impl> m_info;

public:
    backup_list_info(std::string const & backup_dir);

    uint32_t latest_backup_id();
    bool empty() const noexcept;
    std::vector<std::pair<uint32_t, int64_t>> backup_ids_and_timestamps();
};

int backup(const std::string & db_name, const std::string & backup_dir, std::string & errormsg);
int restore(const uint32_t backup_id, const std::string & backup_dir, const std::string & restore_dir, std::string & errormsg);

backup_list_info db_backup_list_info(const std::string & backup_dir);
int makedir(const char * newdir);
