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
    enum_xvdb_cf_type_update_most = 'u',  //update most once Put but rarely read; good for meta-data
    enum_xvdb_cf_type_read_most   = 'r',  //read only once first Put.no update; good for block,txs
    enum_xvdb_cf_type_log_only    = 'f',  //store as log(unreliable) and cleared unused & oldest one automatically(like LRU)
    enum_xvdb_cf_type_state       = 's',  //read only once first Put.no update; good for block,txs
    
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
    static void  setup_default_db_options(rocksdb::Options & default_db_options,const int db_kinds);//setup Default Option of whole DB Level
    void         setup_default_cf_options(xColumnFamily & cf_config,const size_t block_size,std::shared_ptr<rocksdb::Cache> & block_cache);
    
    xColumnFamily setup_default_cf();//setup Default ColumnFamily(CF),and for read&write as well
    xColumnFamily setup_universal_style_cf(const std::string & name,uint64_t memtable_memory_budget = 64 * 1024 * 1024,int num_levels = 5);
    xColumnFamily setup_level_style_cf(const std::string & name,uint64_t memtable_memory_budget = 128 * 1024 * 1024,int num_levels = 7);
    xColumnFamily setup_fifo_style_cf(const std::string & name,uint64_t ttl = 14 * 24 * 60 * 60);//setup ColumnFamily(CF) of log only,delete after 14 day as default setting);

 public:
    //db_kinds refer to xdb_kind_t
    explicit xdb_impl(const int db_kinds,const std::string& db_root_dir,std::vector<xdb_path_t> & db_paths);
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
    //compact whole DB if both begin_key and end_key are empty
    //note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
    bool compact_range(const std::string & begin_key,const std::string & end_key);
    bool get_estimate_num_keys(uint64_t & num) const;
    
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
    int                     m_db_kinds = {0};
};

void    xdb::xdb_impl::disable_default_compress_options(rocksdb::ColumnFamilyOptions & default_db_options)
{
    //as default to disable compress since RocksDB may scale-write and read which bring N*Time compression op
    //disable compression as default since consense node may prune data automatically
    default_db_options.compression = rocksdb::kNoCompression;
    default_db_options.compression_opts.enabled = false;
    //,compressiong bring much more CPU/IO,which means,so prefer CPU/IO first
    if(default_db_options.num_levels > 0)
    {
        for (size_t i = 0; i < default_db_options.compression_per_level.size(); ++i)
        {
            default_db_options.compression_per_level[i] = rocksdb::kNoCompression;
        }
    }
}

void xdb::xdb_impl::setup_default_db_options(rocksdb::Options & default_db_options,const int db_kinds)
{
    default_db_options.create_if_missing = true;
    default_db_options.create_missing_column_families = true;
    default_db_options.level_compaction_dynamic_level_bytes = true; //good for slow I/O disk(e.g HDD)
     
    default_db_options.max_background_jobs = 4; //recommend 4 for HDD disk
    default_db_options.max_subcompactions  = 2; //fast compact to clean deleted_range/deleted_keys
    
    if((db_kinds & xdb_kind_high_compress) != 0)
    {
        default_db_options.compression = rocksdb::kLZ4Compression;
        default_db_options.compression_opts.enabled = true;
        default_db_options.bottommost_compression = rocksdb::kZSTD;
        default_db_options.bottommost_compression_opts.enabled = true;
        
        xkinfo("xdb_impl::setup_default_db_options() as xdb_kind_high_compress");
        // printf("xdb_impl::setup_default_db_options() as xdb_kind_high_compress \n");
    }
    else if((db_kinds & xdb_kind_bottom_compress) != 0) //disable level'compression but keep bottom one
    {
        default_db_options.compression = rocksdb::kNoCompression;
        default_db_options.compression_opts.enabled = false;
        default_db_options.bottommost_compression = rocksdb::kLZ4Compression;
        default_db_options.bottommost_compression_opts.enabled = true;
        
        xkinfo("xdb_impl::setup_default_db_options() as xdb_kind_bottom_compress");
        // printf("xdb_impl::setup_default_db_options() as xdb_kind_bottom_compress \n");
    }
    else if((db_kinds & xdb_kind_no_compress) != 0) //disable compress
    {
        default_db_options.compression = rocksdb::kNoCompression;
        default_db_options.compression_opts.enabled = false;
        default_db_options.bottommost_compression = rocksdb::kNoCompression;
        default_db_options.bottommost_compression_opts.enabled = false;
        
        xkinfo("xdb_impl::setup_default_db_options() as xdb_kind_no_compress");
        // printf("xdb_impl::setup_default_db_options() as xdb_kind_no_compress \n");
    }
    else //normal & default compression
    {
        default_db_options.compression = rocksdb::kLZ4Compression;
        default_db_options.compression_opts.enabled = true;
        default_db_options.bottommost_compression = rocksdb::kLZ4Compression;
        default_db_options.bottommost_compression_opts.enabled = true;
        
        xkinfo("xdb_impl::setup_default_db_options() as default_compress");
        // printf("xdb_impl::setup_default_db_options() as default_compress \n");
    }

    return ;
}

void xdb::xdb_impl::setup_default_cf_options(xColumnFamily & cf_config,const size_t block_size,std::shared_ptr<rocksdb::Cache> & block_cache)
{
    rocksdb::BlockBasedTableOptions table_options;
    if((m_db_kinds & xdb_kind_no_compress) != 0)//performance most
        table_options.enable_index_compression = false;
    
    if(block_size > 0)
        table_options.block_size = block_size;
    if(block_cache != nullptr)
        table_options.block_cache = block_cache;
    
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
 
    cf_config.cf_option.level_compaction_dynamic_level_bytes = m_options.level_compaction_dynamic_level_bytes;
    return;
}

//setup ColumnFamily(CF) of read & write,as default CF
xColumnFamily xdb::xdb_impl::setup_default_cf()
{
    xColumnFamily  cf_config;
    cf_config.cf_name = rocksdb::kDefaultColumnFamilyName;
    cf_config.cf_option.num_levels = m_options.num_levels;
    cf_config.cf_option.compression_per_level = m_options.compression_per_level;
    
    const size_t block_size = 0; //use default one(4 * 1024)
    std::shared_ptr<rocksdb::Cache> block_cache = nullptr; //use default one(8M)
    setup_default_cf_options(cf_config,block_size,block_cache);
    
    return cf_config;
}

//setup ColumnFamily(CF) based on OptimizeUniversalStyleCompaction
xColumnFamily xdb::xdb_impl::setup_universal_style_cf(const std::string & name,uint64_t memtable_memory_budget,int num_levels)
{
    xColumnFamily  cf_config;
    cf_config.cf_name = name;
    cf_config.cf_option = rocksdb::ColumnFamilyOptions();
    if(num_levels <= 0)
        cf_config.cf_option.num_levels = m_options.num_levels;
    else
        cf_config.cf_option.num_levels = num_levels;
    cf_config.cf_option.OptimizeUniversalStyleCompaction(memtable_memory_budget);
    
    const size_t block_size = 0; //use default one(4 * 1024)
    std::shared_ptr<rocksdb::Cache> block_cache = nullptr; //use default one(8M)
    setup_default_cf_options(cf_config,block_size,block_cache);
    
    if(false == cf_config.cf_option.compression_opts.enabled)//force turn off for each level
    {
        xdb::xdb_impl::disable_default_compress_options(cf_config.cf_option);
    }
    //XTODO,upgrade to RocksDB 6.x version
    //cf_config.cf_option.periodic_compaction_seconds = 12 * 60 * 60; //12 hour
    return cf_config;
}

//setup ColumnFamily(CF) based by OptimizeLevelStyleCompaction
xColumnFamily xdb::xdb_impl::setup_level_style_cf(const std::string & name,uint64_t memtable_memory_budget,int num_levels)
{
    xColumnFamily  cf_config;
    cf_config.cf_name = name;
    cf_config.cf_option = rocksdb::ColumnFamilyOptions();
    if(num_levels <= 0)
        cf_config.cf_option.num_levels = m_options.num_levels;
    else
        cf_config.cf_option.num_levels = num_levels;
    cf_config.cf_option.OptimizeLevelStyleCompaction(memtable_memory_budget);

    const size_t block_size = 8 * 1024; //default is 4K -> 8K
    std::shared_ptr<rocksdb::Cache> block_cache = rocksdb::NewLRUCache(32 << 20);//default 8M -> 32M
    //4 Block CF -> 4 * 32 = 128M block caches
    setup_default_cf_options(cf_config,block_size,block_cache);
    if(false == cf_config.cf_option.compression_opts.enabled)//force turn off for each level
    {
        xdb::xdb_impl::disable_default_compress_options(cf_config.cf_option);
    }
    else
    {
        for(size_t it = 0; it < cf_config.cf_option.compression_per_level.size(); ++it)
        {
            if(it < 3) //disable compression of level-0,1,2
            {
                cf_config.cf_option.compression_per_level[it] = rocksdb::kNoCompression;
            }
            xkinfo("xdb_impl::setup_level_style_cf,Options.compression[%d] = %d \n",(int)it,(int)cf_config.cf_option.compression_per_level[it]);
            // printf("xdb_impl::setup_level_style_cf,Options.compression[%d] = %d \n",(int)it,(int)cf_config.cf_option.compression_per_level[it]);
        }
    }
    
    //XTODO,upgrade to RocksDB 6.x version
    //cf_config.cf_option.periodic_compaction_seconds = 12 * 60 * 60; //12 hour
    return cf_config;
}

//setup ColumnFamily(CF) based on kCompactionStyleFIFO
//store as log(unreliable) and cleared unused & oldest one automatically(like LRU)
xColumnFamily xdb::xdb_impl::setup_fifo_style_cf(const std::string & name,uint64_t ttl)
{
    xColumnFamily  cf_config;
    cf_config.cf_name = name;
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
        
        rocksdb::Status s;
        if((m_db_kinds & xdb_kind_readonly) != 0)
        {
            xkinfo("xdb_impl::open() as readonly cf=%zu", column_families.size());
            // printf("xdb_impl::open() as readonly cf=%zu\n", column_families.size());
            s = rocksdb::DB::OpenForReadOnly(m_options, m_db_name, column_families, &cf_handles, &m_db);
        }
        else
        {
            xkinfo("xdb_impl::open() as read_write cf=%zu", column_families.size());
            // printf("xdb_impl::open() as read_write cf=%zu\n", column_families.size());
            s = rocksdb::DB::Open(m_options, m_db_name, column_families, &cf_handles, &m_db);
        }
        
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

        xkinfo("xdb_impl::close,db_root_dir=%s",m_db_name.c_str());
        printf("xdb_impl::close,db_root_dir=%s \n",m_db_name.c_str());
    }
    return true;
}

//db_kinds refer to xdb_kind_t
xdb::xdb_impl::xdb_impl(const int db_kinds,const std::string& db_root_dir,std::vector<xdb_path_t> & db_paths)
{
    m_db_kinds = db_kinds;
    m_cf_handles.clear();
    m_cf_handles.resize(256); //static mapping each 'char' -> handles
    for(size_t i = 0; i < m_cf_handles.size(); ++i)
    {
        m_cf_handles[i] = NULL; //force to reset
    }
    
    xkinfo("xdb_impl::init,db_root_dir=%s",db_root_dir.c_str());
    printf("xdb_impl::init,db_root_dir=%s \n",db_root_dir.c_str());
    
    m_db_name = db_root_dir;
    xdb::xdb_impl::setup_default_db_options(m_options,m_db_kinds);//setup base options first
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

    if ((m_db_kinds & xdb_kind_no_multi_cf) == 0)
    {
        cf_list.push_back(setup_level_style_cf("1")); //block 'cf[1]
        cf_list.push_back(setup_level_style_cf("2")); //block 'cf[2]
        cf_list.push_back(setup_level_style_cf("3")); //block 'cf[3]
        cf_list.push_back(setup_level_style_cf("4")); //block 'cf[4]
        //cf_list.push_back(setup_fifo_style_cf("f"));  //fifo
        //XTODO,add other CF here
    }
    
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
        const char* key_data = key.data();
        if(key_data[1] != '/') //all customized cf must format as "x/..."
            return m_cf_handles[0];
    
        /* key of "r" class for block & index
            r/full_ledger(24bit)/...
            //full_ledger = (ledger_id & 0xFFFF) + (subaddr_of_ledger & 0xFF)
                //ledger_id= [chain_id:12bit][zone_index:4bit]
                //[8bit:subaddr_of_ledger] = [5 bit:book-index]-[3 bit:table-index]
        */
        rocksdb::ColumnFamilyHandle* target_cf  = nullptr;
        if(  (key_data[0] == enum_xvdb_cf_type_read_most)
           ||(key_data[0] == enum_xvdb_cf_type_state) )
        {
            if(key.size() >= 8)
            {
                //const int subaddr_of_ledger = (((int)key_data[6]) << 4) | key_data[7];
                //const int maped_cf = (subaddr_of_ledger % 4) + 1; //mapping to cf[1],cf[2],cf[3].cf[4]
                const int maped_cf = (key_data[7] % 4) + 1; //mapping to cf[1],cf[2],cf[3].cf[4]
                if(maped_cf == 1)
                    target_cf = m_cf_handles['1'];
                else if(maped_cf == 2)
                    target_cf = m_cf_handles['2'];
                else if(maped_cf == 3)
                    target_cf = m_cf_handles['3'];
                else if(maped_cf == 4)
                    target_cf = m_cf_handles['4'];
                
                #ifdef DEBUG
                //xdbg("xdb_get_cf_handle,access CF(%d) <- key(%s)",maped_cf,key.c_str());
                #endif
            }
            else
            {
                target_cf = m_cf_handles[key_data[0]];
            }
        }
        else
        {
            target_cf = m_cf_handles[key_data[0]];
        }
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

//compact whole DB if both begin_key and end_key are empty
//note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
bool xdb::xdb_impl::compact_range(const std::string & begin_key,const std::string & end_key)
{
    rocksdb::Slice  begin_slice(begin_key);
    rocksdb::Slice  end_slice(end_key);
    
    if( (begin_key.empty() == false) && (end_key.empty() == false) ) //specified range of begin and end
    {
        rocksdb::ColumnFamilyHandle* begin_cf = get_cf_handle(begin_key);
        rocksdb::ColumnFamilyHandle* end_cf   = get_cf_handle(end_key);
        if(end_cf == begin_cf) //most case
        {
            printf("xdb_impl::compact_range,one cf \n");
            rocksdb::Status res = m_db->CompactRange(rocksdb::CompactRangeOptions(),begin_cf,&begin_slice,&end_slice);
            if (!res.ok())
            {
                if (res.IsNotFound()) //possible case
                    return true;
                
                handle_error(res);
                return false;
            }
            return true;
        }
    }
    
    printf("xdb_impl::compact_range,all cfs \n");
    for(size_t i = 0; i < m_cf_handles.size(); ++i)
    {
        rocksdb::ColumnFamilyHandle* cf_handle = m_cf_handles[i];
        if(cf_handle != nullptr)//try every CF
        {
            m_db->CompactRange(rocksdb::CompactRangeOptions(),cf_handle,&begin_slice,&end_slice);
        }
    }
    return true;
}

bool xdb::xdb_impl::get_estimate_num_keys(uint64_t & num) const
{
    return m_db->GetIntProperty("rocksdb.estimate-num-keys", &num);
}

xdb::xdb(const int db_kinds,const std::string& db_root_dir,std::vector<xdb_path_t> & db_paths)
: m_db_impl(new xdb_impl(db_kinds,db_root_dir,db_paths)) {
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

//compact whole DB if both begin_key and end_key are empty
//note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
bool xdb::compact_range(const std::string & begin_key,const std::string & end_key)
{
    return m_db_impl->compact_range(begin_key, end_key);
}
bool xdb::get_estimate_num_keys(uint64_t & num) const
{
    return m_db_impl->get_estimate_num_keys(num);
}

}  // namespace ledger
}  // namespace top
