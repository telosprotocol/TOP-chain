// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <iostream>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"
#include "rocksdb/convenience.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rocksdb/utilities/optimistic_transaction_db.h"

#include "xbase/xlog.h"
#include "xdb/xdb.h"
#include "xmetrics/xmetrics.h"

using std::string;

namespace top { namespace db {

class xdb_rocksdb_transaction_t : public xdb_transaction_t {
public:
    bool rollback() override;
    bool commit() override;
    xdb_rocksdb_transaction_t(rocksdb::Transaction* txn) : m_txn(txn) { }
    ~xdb_rocksdb_transaction_t();
    bool read(const std::string& key, std::string& value) const override;
    bool write(const std::string& key, const std::string& value) override;
    bool write(const std::string& key, const char* data, size_t size) override;
    bool erase(const std::string& key) override;
private:
    rocksdb::Transaction* m_txn {nullptr};
    void dump_op_status(const rocksdb::Status& status) const;
};

xdb_rocksdb_transaction_t::~xdb_rocksdb_transaction_t() {
    if (m_txn != nullptr) {
        delete m_txn;
    }
}

bool xdb_rocksdb_transaction_t::rollback() {
    if (m_txn != nullptr) {
        rocksdb::Status s = m_txn->Rollback();
        if (!s.ok()) {
            dump_op_status(s);
            return false;
        }
        return true;
    }
    xwarn("xdb_rocksdb_transaction_t::rollback txn is empty when committing");
    return false;
}

void xdb_rocksdb_transaction_t::dump_op_status(const rocksdb::Status& status) const {
    if (status.ok())
        return;
    const string errmsg = "[xdb] rocksDB error: " + status.ToString();
    xwarn("%s", errmsg.c_str());
}

bool xdb_rocksdb_transaction_t::commit() {
    if (m_txn != nullptr) {
        rocksdb::Status s = m_txn->Commit();
        if (!s.ok()) {
            dump_op_status(s);
            return false;
        }
        return true;
    }
    xwarn("xdb_rocksdb_transaction_t::commit txn is empty when committing");
    return false;
}

bool xdb_rocksdb_transaction_t::read(const std::string& key, std::string& value) const {
    if (m_txn == nullptr) {
        return false;
    }
    
    rocksdb::ReadOptions target_opt = rocksdb::ReadOptions();
    target_opt.ignore_range_deletions = true; //ignored deleted_ranges to improve read performance
    target_opt.verify_checksums = false; //application has own checksum
    
    rocksdb::Status s = m_txn->Get(target_opt, rocksdb::Slice(key), &value);
    if (!s.ok()) {
        if (s.IsNotFound()) {
            return true;
        } else {
            dump_op_status(s);
            return false;
        }
    }

    return true;
}

bool xdb_rocksdb_transaction_t::write(const std::string& key, const std::string& value) {
    if (m_txn == nullptr) {
        return false;
    }
    rocksdb::Status s = m_txn->Put(rocksdb::Slice(key), rocksdb::Slice(value));
    if (!s.ok()) {
        dump_op_status(s);
        return false;
    }
    return true;
}

bool xdb_rocksdb_transaction_t::write(const std::string& key, const char* data, size_t size) {
    if (m_txn == nullptr) {
        return false;
    }
    rocksdb::Status s = m_txn->Put(rocksdb::Slice(key), rocksdb::Slice(data, size));
    if (!s.ok()) {
        dump_op_status(s);
        return false;
    }
    return true;
}

bool xdb_rocksdb_transaction_t::erase(const std::string& key) {
    if (m_txn == nullptr) {
        return false;
    }
    rocksdb::Status s = m_txn->Delete(rocksdb::Slice(key));
    if (!s.ok()) {
        if (s.IsNotFound()) {
            return true;
        } else {
            dump_op_status(s);
            return false;
        }
    }
    return true;
}

//new style(defined as character,optimized for i/o,db size etc),stored at dedicated CF(column Family)
enum enum_xvdb_cf_type
{
    //new style(defined as character,optimized for i/o,db size etc)
    enum_xvdb_cf_type_read_write  = 'w',  //default one for key that has Put/Update/Delete together
    enum_xvdb_cf_type_update_most = 'i',  //update most once Put but rarely read; good for meta-data
    enum_xvdb_cf_type_read_only   = 'b',  //read only once first Put.no update; good for block,txs
    enum_xvdb_cf_type_log_only    = 'l',  //store as log(unreliable) and cleared unused & oldest one automatically(like LRU)
    
    //old style(defined by object) and stored at default CF(column Family)
};

struct xColumnFamily
{
public:
    std::string                  cf_name;
    rocksdb::ColumnFamilyOptions cf_option{};
    rocksdb::ColumnFamilyHandle* cf_handle{nullptr};
};

class xdb::xdb_impl final
{
public:
    static void  disable_default_compress_options(rocksdb::ColumnFamilyOptions & default_cf_options);
    static void  setup_default_db_options(rocksdb::Options & default_db_options);//setup Default Option of whole DB Level
    
    xColumnFamily    setup_default_cf(uint64_t memtable_memory_budget = 448 * 1024 * 1024,int num_levels = 7);//setup Default ColumnFamily(CF),and for read&write as well
    xColumnFamily setup_update_most_cf(uint64_t memtable_memory_budget = 128 * 1024 * 1024,int num_levels = 5);//setup ColumnFamily(CF) of update_most; good for meta-data
    xColumnFamily setup_read_only_cf(uint64_t memtable_memory_budget = 256 * 1024 * 1024,int num_levels = 7);//setup ColumnFamily(CF) of read_only; good for block
    xColumnFamily setup_log_only_cf(uint64_t ttl = 14 * 24 * 60 * 60);//setup ColumnFamily(CF) of log only,delete after 14 day as default setting);

 public:
    explicit xdb_impl(const std::string& db_root_dir,std::vector<xdb_path_t> & db_paths);
    ~xdb_impl();
    bool open();
    bool close();
    bool read(const std::string& key, std::string& value) const;
    bool exists(const std::string& key) const;
    bool write(const std::string& key, const std::string& value);
    bool write(const std::string& key, const char* data, size_t size);
    bool write(const std::map<std::string, std::string>& batches);
    bool erase(const std::string& key);
    bool erase(const std::vector<std::string>& keys);
    bool batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys);
    bool read_range(const std::string& prefix, std::vector<std::string>& values);

    bool single_delete(const std::string& key);
    bool delete_range(const std::string& begin_key,const std::string& end_key);
    
    //iterator each key of prefix.note: go throuh whole db if prefix is empty
    bool read_range(const std::string& prefix,xdb_iterator_callback callback,void * cookie);
    
    static void destroy(const std::string& m_db_name);

 private:
    rocksdb::ColumnFamilyHandle* get_cf_handle(const std::string& key) const;
    void handle_error(const rocksdb::Status& status) const;
    std::string             m_db_name{};
    rocksdb::DB*            m_db{nullptr};
    rocksdb::Options        m_options{};
    rocksdb::WriteBatch     m_batch{};
    std::vector<xColumnFamily>                m_cf_configs;
    std::vector<rocksdb::ColumnFamilyHandle*> m_cf_handles;
};

void    xdb::xdb_impl::disable_default_compress_options(rocksdb::ColumnFamilyOptions & default_db_options)
{
    //as default to disable compress since RocksDB may scale-write and read which bring N*Time compression op
    //disable compression as default since consense node may prune data automatically
    default_db_options.compression = rocksdb::kNoCompression;
    default_db_options.compression_opts.enabled = false;
    default_db_options.bottommost_compression = rocksdb::kNoCompression;
    default_db_options.bottommost_compression_opts.enabled = false;
    //,compressiong bring much more CPU/IO,which means,so prefer CPU/IO first
    if(default_db_options.num_levels > 0)
    {
        for (size_t i = 0; i < default_db_options.compression_per_level.size(); ++i)
        {
            default_db_options.compression_per_level[i] = rocksdb::kNoCompression;
        }
    }
}

void xdb::xdb_impl::setup_default_db_options(rocksdb::Options & default_db_options)
{
    default_db_options.create_if_missing = true;
    default_db_options.create_missing_column_families = true;
    default_db_options.level_compaction_dynamic_level_bytes = true; //good for slow I/O disk(e.g HDD)
     
    default_db_options.max_background_jobs = 4; //recommend 4 for HDD disk
    default_db_options.max_subcompactions  = 2; //fast compact to clean deleted_range/deleted_keys
    
    #ifdef __ENABLE_ROCKSDB_COMPRESSTION__
    //do nothing to keep defaut
    #else //disable compress
    default_db_options.compression = rocksdb::kNoCompression;
    default_db_options.compression_opts.enabled = false;
    default_db_options.bottommost_compression = rocksdb::kNoCompression;
    default_db_options.bottommost_compression_opts.enabled = false;
    #endif
    return ;
}

//setup ColumnFamily(CF) of read & write,as default CF
xColumnFamily xdb::xdb_impl::setup_default_cf(uint64_t memtable_memory_budget,int num_levels)
{
    xColumnFamily  cf_config;
    cf_config.cf_name   = rocksdb::kDefaultColumnFamilyName;
 
    rocksdb::BlockBasedTableOptions table_options;
    table_options.enable_index_compression = false;
    table_options.block_size = 16 * 1024; //default is 4K -> 16K
    table_options.block_cache = rocksdb::NewLRUCache(32 << 20);//default 8M -> 32M
    
#ifdef DB_CACHE
    table_options.cache_index_and_filter_blocks = true;
    table_options.cache_index_and_filter_blocks_with_high_priority = true;
    table_options.pin_l0_filter_and_index_blocks_in_cache = true;
#endif
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
    
    cf_config.cf_option.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
    
    cf_config.cf_option.compression_opts.enabled = m_options.compression_opts.enabled;
    cf_config.cf_option.compression = m_options.compression;
    
    cf_config.cf_option.bottommost_compression_opts.enabled = m_options.bottommost_compression_opts.enabled;
    cf_config.cf_option.bottommost_compression = m_options.bottommost_compression;
    
    cf_config.cf_option.num_levels = m_options.num_levels;
    cf_config.cf_option.compression_per_level = m_options.compression_per_level;
    
    cf_config.cf_option.level_compaction_dynamic_level_bytes = m_options.level_compaction_dynamic_level_bytes;
    
    return cf_config;
}

//setup ColumnFamily(CF) of update_only; good for meta-data,span-data
//update only once first Put,no delete and rarely read op
xColumnFamily xdb::xdb_impl::setup_update_most_cf(uint64_t memtable_memory_budget,int num_levels)
{
    xColumnFamily  cf_config;
    cf_config.cf_name.append(1,(const char)enum_xvdb_cf_type_update_most);
    cf_config.cf_option = rocksdb::ColumnFamilyOptions();
    cf_config.cf_option.num_levels = num_levels;
    cf_config.cf_option.OptimizeUniversalStyleCompaction(memtable_memory_budget);
    cf_config.cf_option.level_compaction_dynamic_level_bytes = m_options.level_compaction_dynamic_level_bytes;
    //XTODO,upgrade to RocksDB 6.x version
    //cf_config.cf_option.periodic_compaction_seconds = 12 * 60 * 60; //12 hour
    
    #ifndef __ENABLE_ROCKSDB_COMPRESSTION__
    xdb::xdb_impl::disable_default_compress_options(cf_config.cf_option);
    #endif
    return cf_config;
}

//setup ColumnFamily(CF) of read_only; good for block,txs
//delete only once first Put, no update and many read
xColumnFamily xdb::xdb_impl::setup_read_only_cf(uint64_t memtable_memory_budget,int num_levels)
{
    xColumnFamily  cf_config;
    cf_config.cf_name.append(1,(const char)enum_xvdb_cf_type_read_only);
    cf_config.cf_option = rocksdb::ColumnFamilyOptions();
    cf_config.cf_option.num_levels = num_levels;
    cf_config.cf_option.OptimizeLevelStyleCompaction(memtable_memory_budget);
    cf_config.cf_option.level_compaction_dynamic_level_bytes = m_options.level_compaction_dynamic_level_bytes;
    //XTODO,upgrade to RocksDB 6.x version
    //cf_config.cf_option.periodic_compaction_seconds = 1 * 24 * 60 * 60; //1 day
 
    #ifndef __ENABLE_ROCKSDB_COMPRESSTION__
    xdb::xdb_impl::disable_default_compress_options(cf_config.cf_option);
    #endif
    return cf_config;
}

//setup ColumnFamily(CF) of log_only; good for cached txs
//store as log(unreliable) and cleared unused & oldest one automatically(like LRU)
xColumnFamily xdb::xdb_impl::setup_log_only_cf(uint64_t ttl)
{
    xColumnFamily  cf_config;
    cf_config.cf_name.append(1,(const char)enum_xvdb_cf_type_log_only);
    cf_config.cf_option = rocksdb::ColumnFamilyOptions();
    cf_config.cf_option.OptimizeForSmallDb();
    cf_config.cf_option.compaction_style = rocksdb::kCompactionStyleFIFO;
    //cf_config.cf_option.compaction_options_fifo.max_table_files_size; //Default: 1GB
    cf_config.cf_option.compaction_options_fifo.ttl = ttl; //unit: seconds
    cf_config.cf_option.level_compaction_dynamic_level_bytes = false;
    
    xdb::xdb_impl::disable_default_compress_options(cf_config.cf_option);
    return cf_config;
}

bool xdb::xdb_impl::open()
{
    if (m_db == nullptr)
    {
        std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
        for(auto it : m_cf_configs)
        {
            column_families.push_back(rocksdb::ColumnFamilyDescriptor(it.cf_name,it.cf_option));
        }
 
        //const std::string rocksdb_version = rocksdb::GetRocksVersionAsString();
        //xkinfo("xrocksdb_t::get_raw_ptr(),rocksdb::version=%s",version.c_str());
        //printf("xrocksdb_t::get_raw_ptr(),rocksdb::version=%s \n",version.c_str());
        
        std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
        rocksdb::Status s = rocksdb::DB::Open(m_options, m_db_name, column_families, &cf_handles, &m_db);
        handle_error(s);
        if(s.ok())
        {
            //keep handle ptr to config together
            for(size_t i = 0; i < cf_handles.size(); ++i)
            {
                m_cf_configs[i].cf_handle = cf_handles[i];
            }
            //setup fast-mapping
            for(auto cf : m_cf_configs)
            {
                if(cf.cf_name == rocksdb::kDefaultColumnFamilyName)
                    m_cf_handles[0] = cf.cf_handle; //always put default one to slot of 0
                else
                    m_cf_handles[cf.cf_name.at(0)] = cf.cf_handle;
            }
            
            rocksdb::Options working_options = m_db->GetOptions();
            if(working_options.compression_per_level.empty())
            {
                xkinfo("xdb_impl::open(),Options.compression=%d,bottommost_compression=%d",working_options.compression,working_options.bottommost_compression);
                printf("xdb_impl::open(),Options.compression=%d,bottommost_compression=%d \n",working_options.compression,working_options.bottommost_compression);
            }
            else
            {
                for(size_t it = 0; it < working_options.compression_per_level.size(); ++it)
                {
                    xkinfo("xdb_impl::open(),Options.compression[%d] = %d \n",(int)it,(int)working_options.compression_per_level[it]);
                    printf("xdb_impl::open(),Options.compression[%d] = %d \n",(int)it,(int)working_options.compression_per_level[it]);
                }
            }
        }
        else
        {
            const string errmsg = "[xdb] rocksDB error: " + s.ToString() + " ,db name " + m_db_name;
            xerror("%s", errmsg.c_str());
            printf("%s\n",errmsg.c_str());
        }
        return s.ok();
    }
    return true;
}
bool xdb::xdb_impl::close()
{
    if (m_db)
    {
        rocksdb::DB* old_db_ptr = m_db;
        //delete handle ptr
        for(auto & cf : m_cf_configs)
        {
            delete cf.cf_handle;
            cf.cf_handle = NULL;
        }
        m_db->Close();
        m_db = NULL;
        
        //clear ptr
        for(size_t i = 0; i < m_cf_handles.size(); ++i)
        {
            m_cf_handles[i] = NULL;
        }
        
        delete old_db_ptr;
        old_db_ptr = NULL;
    }
    return true;
}

xdb::xdb_impl::xdb_impl(const std::string& db_root_dir,std::vector<xdb_path_t> & db_paths)
{
    m_cf_handles.clear();
    m_cf_handles.resize(256); //static mapping each 'char' -> handles
    for(size_t i = 0; i < m_cf_handles.size(); ++i)
    {
        m_cf_handles[i] = NULL; //force to reset
    }
    
    xkinfo("xdb_impl::init,db_root_dir=%s",db_root_dir.c_str());
    printf("xdb_impl::init,db_root_dir=%s \n",db_root_dir.c_str());
    
    m_db_name = db_root_dir;
    xdb::xdb_impl::setup_default_db_options(m_options);//setup base options first
    if(db_paths.empty() == false)
    {
        for(auto it : db_paths)
        {
            rocksdb::DbPath rocksdb_path;
            rocksdb_path.path        = it.path;
            rocksdb_path.target_size = it.target_size;
            m_options.db_paths.push_back(rocksdb_path);
            
            xkinfo("xdb_impl::init,the customized data_path=%s and target_size=%lld",it.path.c_str(),(long long int)it.target_size);
            printf("xdb_impl::init,the customized data_path=%s and target_size=%lld \n",it.path.c_str(),(long long int)it.target_size);
        }
    }
    
    std::vector<xColumnFamily> cf_list;
    cf_list.push_back(setup_default_cf()); //default is always first one
    //XTODO,add other CF here
    
    m_cf_configs = cf_list;
    
    open();
}

xdb::xdb_impl::~xdb_impl() {
    close();
}


rocksdb::ColumnFamilyHandle* xdb::xdb_impl::get_cf_handle(const std::string& key) const
{
    if(key.size() >= 2)//hit most case
    {
        if(key.at(1) != '/') //all customized cf must format as "x/..."
            return m_cf_handles[0];
        
        rocksdb::ColumnFamilyHandle* target_cf = m_cf_handles[(uint8_t)key.at(0)];
        if(NULL == target_cf) //using default for any other case
            target_cf = m_cf_handles[0];
        
        return target_cf;
    }
    else //default one for any other cases
    {
        return m_cf_handles[0];
    }
}

void xdb::xdb_impl::handle_error(const rocksdb::Status& status) const {
    if (status.ok())
        return;
    const string errmsg = "[xdb] rocksDB error: " + status.ToString() + " ,db name " + m_db_name;
    xwarn("%s", errmsg.c_str());
    // throw xdb_error(errmsg);
}

bool xdb::xdb_impl::read(const std::string& key, std::string& value) const {
    rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(key);
    
    rocksdb::ReadOptions target_opt = rocksdb::ReadOptions();
    target_opt.ignore_range_deletions = true; //ignored deleted_ranges to improve read performance
    target_opt.verify_checksums = false; //application has own checksum
    
    rocksdb::Status s = m_db->Get(target_opt, target_cf, rocksdb::Slice(key), &value);
    if (!s.ok()) {
        if (s.IsNotFound()) {
            return false;
        }
        handle_error(s);
        return false;
    }
    return true;
}

bool xdb::xdb_impl::exists(const std::string& key) const {
    std::string value;
    return read(key, value);
}

bool xdb::xdb_impl::write(const std::string& key, const std::string& value) {
    rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(key);
    
    rocksdb::Status s = m_db->Put(rocksdb::WriteOptions(), target_cf, rocksdb::Slice(key), rocksdb::Slice(value));
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::write(const std::string& key, const char* data, size_t size) {
    rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(key);
    
    rocksdb::Status s = m_db->Put(rocksdb::WriteOptions(), target_cf, rocksdb::Slice(key), rocksdb::Slice(data, size));
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::write(const std::map<std::string, std::string>& batches) {
    rocksdb::WriteBatch batch;
    for (const auto& entry: batches) {
        rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(entry.first);
        batch.Put(target_cf, entry.first, entry.second);
    }
    rocksdb::Status s = m_db->Write(rocksdb::WriteOptions(), &batch);
    handle_error(s);
    return s.ok();
}


bool xdb::xdb_impl::erase(const std::string& key) {
    rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(key);
    
    rocksdb::Status s = m_db->Delete(rocksdb::WriteOptions(), target_cf, rocksdb::Slice(key));
    if (!s.ok()) {
        if (s.IsNotFound()) //possible case
            return true;
        
        handle_error(s);
        return false;
    }
    return true;
}

bool xdb::xdb_impl::erase(const std::vector<std::string>& keys) {
    rocksdb::WriteBatch batch;
    for (const auto& key: keys) {
        rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(key);
        batch.Delete(target_cf, key);
    }
    rocksdb::Status s = m_db->Write(rocksdb::WriteOptions(), &batch);
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) {
    rocksdb::WriteBatch batch;
    for (const auto& entry: objs) {
        rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(entry.first);
        batch.Put(target_cf, entry.first, entry.second);
    }
    for (const auto& key: delete_keys) {
        rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(key);
        batch.Delete(target_cf, key);
    }
    rocksdb::Status s = m_db->Write(rocksdb::WriteOptions(), &batch);
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::single_delete(const std::string& key)
{
    rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(key);
 
    rocksdb::Status res = m_db->SingleDelete(rocksdb::WriteOptions(), target_cf, rocksdb::Slice(key));
    if (!res.ok())
    {
        if (res.IsNotFound()) //possible case
            return true;
        
        handle_error(res);
        return false;
    }
    return true;
}

bool xdb::xdb_impl::delete_range(const std::string& begin_key,const std::string& end_key)
{
    rocksdb::ColumnFamilyHandle* begin_cf = get_cf_handle(begin_key);
    rocksdb::ColumnFamilyHandle* end_cf   = get_cf_handle(end_key);
    if(end_cf != begin_cf)
    {
        xerror("xdb_impl::delete_range,keys are at different CFs,beginCF(%s) != endCF(%s)",begin_cf->GetName().c_str(),end_cf->GetName().c_str());
        return false;
    }
    
    rocksdb::Status res = m_db->DeleteRange(rocksdb::WriteOptions(),begin_cf,rocksdb::Slice(begin_key), rocksdb::Slice(end_key));
    if (!res.ok())
    {
        if (res.IsNotFound()) //possible case
            return true;
        
        handle_error(res);
        return false;
    }
    return true;
}

void xdb::xdb_impl::destroy(const std::string& m_db_name) {
    rocksdb::DestroyDB(m_db_name, rocksdb::Options());
}

bool xdb::xdb_impl::read_range(const std::string& prefix, std::vector<std::string>& values) {
 
    rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(prefix);
    
    rocksdb::ReadOptions target_opt = rocksdb::ReadOptions();
    target_opt.ignore_range_deletions = true; //ignored deleted_ranges to improve read performance
    target_opt.verify_checksums = false; //application has own checksum
    
    bool ret = false;
    auto iter = m_db->NewIterator(target_opt, target_cf);

    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        values.push_back(iter->value().ToString());
        ret = true;
    }
    delete iter;
    return ret;
}

//iterator each key of prefix.note: go throuh whole db if prefix is empty
bool xdb::xdb_impl::read_range(const std::string& prefix,xdb_iterator_callback callback_fuc,void * cookie)
{
    bool ret = false;
    rocksdb::ColumnFamilyHandle* target_cf = get_cf_handle(prefix);
    
    rocksdb::ReadOptions target_opt = rocksdb::ReadOptions();
    target_opt.ignore_range_deletions = true; //ignored deleted_ranges to improve read performance
    target_opt.verify_checksums = false; //application has own checksum
    
    auto iter = m_db->NewIterator(target_opt, target_cf);
    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next())
    {
        const std::string std_key(iter->key().ToString());
        const std::string std_value(iter->value().ToString());
        if((*callback_fuc)(std_key,std_value,cookie) == false)
        {
            ret = false;
            break;
        }
        ret = true;
    }
    delete iter;
    return ret;
}


xdb::xdb(const std::string& db_root_dir,std::vector<xdb_path_t> & db_paths)
: m_db_impl(new xdb_impl(db_root_dir,db_paths)) {
}

xdb::~xdb() noexcept = default;

bool xdb::open() {
    return m_db_impl->open();
}
bool xdb::close() {
    return m_db_impl->close();
}

bool xdb::read(const std::string& key, std::string& value) const {
    XMETRICS_TIMER(metrics::db_read_tick);
    auto ret = m_db_impl->read(key, value);
    XMETRICS_GAUGE(metrics::db_read_size, value.size());
    XMETRICS_GAUGE(metrics::db_read, ret ? 1 : 0);
    return ret;
}

bool xdb::exists(const std::string& key) const {
    return m_db_impl->exists(key);
}

bool xdb::write(const std::string& key, const std::string& value) {
    XMETRICS_TIMER(metrics::db_write_tick);
    XMETRICS_GAUGE(metrics::db_write_size, value.size());
    auto ret = m_db_impl->write(key, value);
    XMETRICS_GAUGE(metrics::db_write, ret ? 1 : 0);
    return ret;
}

bool xdb::write(const std::string& key, const char* data, size_t size) {
    XMETRICS_TIMER(metrics::db_write_tick);
    XMETRICS_GAUGE(metrics::db_write_size, size);
    auto ret = m_db_impl->write(key, data, size);
    XMETRICS_GAUGE(metrics::db_write, ret ? 1 : 0);
    return ret;
}

bool xdb::write(const std::map<std::string, std::string>& batches) {
    XMETRICS_TIMER(metrics::db_write_tick);
    auto ret = m_db_impl->write(batches);
    XMETRICS_GAUGE(metrics::db_write, ret ? 1 : 0);
    return ret;
}

bool xdb::erase(const std::string& key) {
    XMETRICS_TIMER(metrics::db_delete_tick);
    auto ret = m_db_impl->erase(key);
    XMETRICS_GAUGE(metrics::db_delete, ret ? 1 : 0);
    return ret;
}

bool xdb::erase(const std::vector<std::string>& keys) {
    XMETRICS_TIMER(metrics::db_delete_tick);
    auto ret = m_db_impl->erase(keys);
    XMETRICS_GAUGE(metrics::db_delete, ret ? 1 : 0);
    return ret;
}

bool xdb::batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) {
    return m_db_impl->batch_change(objs, delete_keys);
}

void xdb::destroy(const std::string& m_db_name) {
    rocksdb::DestroyDB(m_db_name, rocksdb::Options());
}

bool xdb::read_range(const std::string& prefix, std::vector<std::string>& values)
{
    XMETRICS_TIMER(metrics::db_read_tick);
    auto ret =  m_db_impl->read_range(prefix, values);
    XMETRICS_GAUGE(metrics::db_read, ret ? 1 : 0);
    return ret;
}

bool xdb::delete_range(const std::string& begin_key,const std::string& end_key)
{
    XMETRICS_TIMER(metrics::db_delete_tick);
    auto ret = m_db_impl->delete_range(begin_key, end_key);
    XMETRICS_GAUGE(metrics::db_delete, ret ? 1 : 0);
    return ret;
}

bool xdb::single_delete(const std::string& key)
{
    XMETRICS_TIMER(metrics::db_delete_tick);
    auto ret =  m_db_impl->single_delete(key);
    XMETRICS_GAUGE(metrics::db_delete, ret ? 1 : 0);
    return ret;
}
 
//iterator each key of prefix.note: go throuh whole db if prefix is empty
bool xdb::read_range(const std::string& prefix,xdb_iterator_callback callback,void * cookie)
{
    return m_db_impl->read_range(prefix, callback,cookie);
}

}  // namespace ledger
}  // namespace top
