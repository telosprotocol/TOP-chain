#include <sys/stat.h>
#include <errno.h>
#include <features.h>
#include <rocksdb/db.h>
#include <rocksdb/env.h>
#include <rocksdb/options.h>
#include <rocksdb/status.h>
#include <rocksdb/utilities/backupable_db.h>
#include <rocksdb/write_batch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstring>

#include "db_tool.h"


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

extern "C"
{
#include "contrib/minizip/unzip.h"
}
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

using namespace std;

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
    const string errmsg = "[xdb] rocksDB error: " + status.ToString();
    printf("%s\n", errmsg.c_str());
}

int backup(const std::string& db_name, const std::string& backup_dir,std::string& errormsg){
	rocksdb::DB* m_db{nullptr};
    rocksdb::Options m_options{};
	m_options.create_if_missing = true;
    rocksdb::WriteBatch m_batch{};

    rocksdb::Status s = rocksdb::DB::OpenForReadOnly(m_options, db_name, &m_db);
	if(!s.ok()){
        errormsg = s.getState();
        return -1;
    }

	rocksdb::BackupEngine* backup_engine;
	s = rocksdb::BackupEngine::Open(rocksdb::Env::Default(),
		rocksdb::BackupableDBOptions(backup_dir), &backup_engine);
	// assert(s.ok());
    if (!s.ok()){
        // printf("Get error: %s when open db dir\n",s.getState());
        errormsg = s.getState();
        delete backup_engine;
        delete m_db;
        m_db = nullptr;
        return -1;
    }
	s = backup_engine->CreateNewBackup(m_db);

    if(!s.ok()){
        // printf("Get error: %s when create new backup\n",s.getState());
        errormsg = s.getState();
    }
	// assert(s.ok());

    delete backup_engine;
    delete m_db;
	m_db = nullptr;
	return 0;
}

int restore(const uint32_t backup_id,const std::string& backup_dir, const std::string& restore_dir,std::string& errormsg){
    rocksdb::BackupEngineReadOnly* backup_engine;
    rocksdb::Status s = rocksdb::BackupEngineReadOnly::Open(
        rocksdb::Env::Default(), rocksdb::BackupableDBOptions(backup_dir), &backup_engine);

    if (!s.ok()){
        // printf("Error: %s\n",s.getState());
        errormsg = s.getState();
        delete backup_engine;
        return -1;
    }

    s = backup_engine->RestoreDBFromBackup((rocksdb::BackupID)backup_id,restore_dir, restore_dir);
    if (!s.ok()){
        // printf("Error: Id: %d, %s\n",backup_id,s.getState());
        errormsg = s.getState();
        delete backup_engine;
        return -1;
    }
    delete backup_engine;

	// rocksdb::DB* m_db{nullptr};
    // rocksdb::Options m_options{};
    // rocksdb::WriteBatch m_batch{};
    // s = rocksdb::DB::Open(m_options, restore_dir, &m_db);
	// handle_error(s);

    // delete m_db;
	// m_db = nullptr;

	return 0;
}

std::vector<rocksdb::BackupInfo> db_backup_list_info(const std::string& backup_dir) {
    rocksdb::BackupEngineReadOnly* backup_engine;
    rocksdb::Status s = rocksdb::BackupEngineReadOnly::Open(
        rocksdb::Env::Default(), rocksdb::BackupableDBOptions(backup_dir), &backup_engine);
    // assert(s.ok());
    if (!s.ok()){
        return std::vector<rocksdb::BackupInfo>{};
    }
    std::vector<rocksdb::BackupInfo> backup_info;
    backup_engine->GetBackupInfo(&backup_info);
    delete backup_engine;


	return backup_info;
}
