#include "xbase/xcxx_config.h"

#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>

#include <string>
#include <vector>
#include <fstream>

#include "db_tool.h"
// #include "rocksdb/slice.h"
// #include "rocksdb/options.h"
// #include "rocksdb/table.h"
// #include "rocksdb/convenience.h"
#include "rocksdb/utilities/backupable_db.h"
#include "xbase/xbase.h"
#include <cstring>


#ifndef __USE_FILE_OFFSET64
        #define __USE_FILE_OFFSET64
#endif
#ifndef __USE_LARGEFILE64
        #define __USE_LARGEFILE64
#endif
#ifndef _LARGEFILE64_SOURCE
        #define _LARGEFILE64_SOURCE
#endif
#ifndef _FILE_OFFSET_BIT
        #define _FILE_OFFSET_BIT 64
#endif

#if defined(XCXX20)
#include <zlib.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>
#else
#include "zlib.h"
#include "contrib/minizip/zip.h"
#include "contrib/minizip/unzip.h"
#endif

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

#if defined(__MAC_PLATFORM__)
#define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#define FTELLO_FUNC(stream) ftell(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseek(stream, offset, origin)
#else
#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#endif

// using namespace std;

int mymkdir(
    const char* dirname)
{
    int ret=0;
    ret = mkdir (dirname,0775);
    return ret;
}
int makedir(const char *newdir)
{
	char *buffer ;
	char *p;
	int  len = (int)strlen(newdir);

	if (len <= 0)
	return 0;

	buffer = (char*)malloc(len+1);
    if (buffer==NULL)
    {
        printf("Error allocating memory\n");
        return UNZ_INTERNALERROR;
    }
	strcpy(buffer,newdir);

	if (buffer[len-1] == '/') {
		buffer[len-1] = '\0';
	}
	if (mymkdir(buffer) == 0)
	{
		free(buffer);
		return 1;
	}

	p = buffer+1;
	while (1)
	{
		char hold;

		while(*p && *p != '\\' && *p != '/')
			p++;

		hold = *p;
		*p = 0;
		if ((mymkdir(buffer) == -1) && (errno != EEXIST))
		// if ((mymkdir(buffer) == -1))
		{
			printf("couldn't create directory %s,errno:%d\n",buffer,errno);
			free(buffer);
			return 0;
		}
		if (hold == 0)
			break;
		*p++ = hold;
	}
	free(buffer);
	return 1;
}

void handle_error(const rocksdb::Status& status) {
    if (status.ok())
        return;
    const std::string errmsg = "[xdb] rocksDB error: " + status.ToString();
    printf("%s\n", errmsg.c_str());
}

int backup(const std::string& db_name, const std::string& backup_dir,std::string& errormsg){
    
    rocksdb::Status s;
    rocksdb::DB* m_db{nullptr};
    rocksdb::Options m_options;
    m_options.create_if_missing = true;
    std::shared_ptr<rocksdb::Logger> backup_log;
    rocksdb::BackupEngine* backup_engine{nullptr};
 
    do
    {
        rocksdb::Env::Default()->NewLogger("./backup_.log",  &backup_log);
        backup_log.get()->SetInfoLogLevel(rocksdb::InfoLogLevel::DEBUG_LEVEL);

        std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
        std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor("1",rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor("2",rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor("3",rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor("4",rocksdb::ColumnFamilyOptions()));
        s = rocksdb::DB::OpenForReadOnly(m_options, db_name, column_families, &cf_handles, &m_db); 
        if (!s.ok()) {
            break;
        }

        rocksdb::BackupableDBOptions backup_options = rocksdb::BackupableDBOptions(backup_dir, nullptr, true, backup_log.get());
        backup_options.max_background_operations = 4;
        s = rocksdb::BackupEngine::Open(rocksdb::Env::Default(), backup_options, &backup_engine);
        if (!s.ok()) {
            break;
        }

        rocksdb::CreateBackupOptions back_options;
        s = backup_engine->CreateNewBackup(back_options, m_db);
        if(!s.ok()) {
            break;
        }
        //check db
        std::vector<rocksdb::BackupInfo> backup_info;
        backup_engine->GetBackupInfo(&backup_info);

        //for check db 3 times
        for(int n = 1; n < 3; n++){
            s = backup_engine->VerifyBackup(backup_info.back().backup_id);
            if (!s.ok()) {
                sleep(5*60*n*n);
                std::cout << "VerifyBackup rocksDB failed: " << backup_info.back().backup_id <<std::endl;
            } else {
                std::cout << "VerifyBackup rocksDB sucess: " << backup_info.back().backup_id <<std::endl;
                //only keep 2 backup
                backup_engine->PurgeOldBackups(2);
                break;
            }
        }

        if (!s.ok()) {
            backup_engine->DeleteBackup(backup_info.back().backup_id);
            std::cout << "VerifyBackup rocksDB failed, delete backup id  " << backup_info.back().backup_id <<std::endl;
        }
    } while (0);
    
    delete backup_engine;
    delete m_db;
    if (!s.ok()) {
        errormsg = s.getState();
        rocksdb::Log(rocksdb::InfoLogLevel::ERROR_LEVEL, backup_log, "%s", s.getState());
        return -1;
    }    
    return 0;
}

int restore(const uint32_t backup_id,const std::string& backup_dir, const std::string& restore_dir,std::string& errormsg){
    rocksdb::Status s;
    rocksdb::BackupEngineReadOnly* backup_engine;
    std::shared_ptr<rocksdb::Logger> backup_log;
    
    do {
        rocksdb::Env::Default()->NewLogger("./restore_.log",  &backup_log);
        backup_log.get()->SetInfoLogLevel(rocksdb::InfoLogLevel::DEBUG_LEVEL);

        rocksdb::BackupableDBOptions backup_options = rocksdb::BackupableDBOptions(backup_dir, nullptr, true, backup_log.get());
        backup_options.max_background_operations = 4;
        s = rocksdb::BackupEngineReadOnly::Open(rocksdb::Env::Default(), backup_options, &backup_engine);
        if (!s.ok()) {
            break;
        }
        s = backup_engine->RestoreDBFromBackup((rocksdb::BackupID)backup_id,restore_dir, restore_dir);
        if (!s.ok()) {
            break;
        }
    } while (0);
    
    delete backup_engine;
    if (!s.ok()) {
        errormsg = s.getState();
        rocksdb::Log(rocksdb::InfoLogLevel::ERROR_LEVEL, backup_log, "%s", s.getState());
        return -1;
    }   
	return 0;
}

backup_list_info db_backup_list_info(const std::string & backup_dir) {
    auto r = backup_list_info{backup_dir};
    return r;
}

class backup_list_info_impl {
public:
    std::vector<rocksdb::BackupInfo> m_data;

public:
    backup_list_info_impl(std::vector<rocksdb::BackupInfo> info) : m_data{info} {
    }
};

backup_list_info::backup_list_info(std::string const & backup_dir) {
    rocksdb::BackupEngineReadOnly * backup_engine;
    rocksdb::Status s = rocksdb::BackupEngineReadOnly::Open(rocksdb::Env::Default(), rocksdb::BackupableDBOptions(backup_dir), &backup_engine);
    // assert(s.ok());
    std::vector<rocksdb::BackupInfo> backup_info;
    if (s.ok()) {
        backup_engine->GetBackupInfo(&backup_info);
    }
    m_info = std::make_shared<backup_list_info_impl>(backup_info);
    delete backup_engine;
}

uint32_t backup_list_info::latest_backup_id() {
    if (empty()) {
        return 0;
    }
    return m_info->m_data.back().backup_id;
}

bool backup_list_info::empty() const noexcept {
    return m_info->m_data.empty();
}

std::vector<std::pair<uint32_t, int64_t>> backup_list_info::backup_ids_and_timestamps() {
    std::vector<std::pair<uint32_t, int64_t>> result;
    for (auto iter : m_info->m_data) {
        result.push_back(std::make_pair(iter.backup_id, iter.timestamp));
    }
    return result;
}
