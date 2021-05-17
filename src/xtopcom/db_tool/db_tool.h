// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <stdint.h>
#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"
#include "rocksdb/convenience.h"
#include "rocksdb/utilities/backupable_db.h"

namespace rocksdb {
struct BackupInfo;
}  // namespace rocksdb

int backup(const std::string& db_name, const std::string& backup_dir,std::string& errormsg);
int restore(const uint32_t backup_id, const std::string& backup_dir, const std::string& restore_dir,std::string& errormsg);

std::vector<rocksdb::BackupInfo> db_backup_list_info(const std::string& backup_dir);
int makedir(const char *newdir);
