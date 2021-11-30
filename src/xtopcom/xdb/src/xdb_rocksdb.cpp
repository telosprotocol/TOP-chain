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
    rocksdb::Status s = m_txn->Get(rocksdb::ReadOptions(), rocksdb::Slice(key), &value);
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

class xdb::xdb_impl final{
 public:
    explicit xdb_impl(const std::string& name);
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
    
    static void destroy(const std::string& m_db_name);

 private:
    void handle_error(const rocksdb::Status& status) const;
    std::string m_db_name{};
    rocksdb::OptimisticTransactionDB* m_db{nullptr};
    rocksdb::Options m_options{};
    rocksdb::WriteBatch m_batch{};
    rocksdb::ColumnFamilyOptions m_cf_opt{};
    std::vector<rocksdb::ColumnFamilyHandle*> m_handles;
};

bool xdb::xdb_impl::open() {
    if (m_db == nullptr) {
        std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
        // have to open default column family
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
                rocksdb::kDefaultColumnFamilyName, m_cf_opt));

        rocksdb::Status s = rocksdb::OptimisticTransactionDB::Open(m_options, m_db_name, column_families, &m_handles, &m_db);
        handle_error(s);
        return s.ok();
    }
    return true;
}
bool xdb::xdb_impl::close() {
    if (m_db) {
        for (auto handle : m_handles) {
            delete handle;
        }
        m_handles.clear();

        delete m_db;
        m_db = nullptr;
    }
    return true;
}

xdb::xdb_impl::xdb_impl(const std::string& name) {
    m_db_name = name;
    m_options.create_if_missing = true;

    m_options.compression = rocksdb::kLZ4Compression;
    m_options.level_compaction_dynamic_level_bytes = true;
    // default number of levels is 7
    // explicit set here to match each level's compression algorithm
    m_options.num_levels = 7;
    m_options.compression_per_level.resize(m_options.num_levels);

    m_options.compression_per_level = { };

    m_options.compression_opts.enabled = true;
    m_options.bottommost_compression = rocksdb::kZSTD;

    rocksdb::BlockBasedTableOptions table_options;
    table_options.enable_index_compression = false;
#ifdef DB_CACHE
    table_options.cache_index_and_filter_blocks = true;
    table_options.cache_index_and_filter_blocks_with_high_priority = true;
    table_options.pin_l0_filter_and_index_blocks_in_cache = true;
#endif
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));

    m_cf_opt.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

    m_cf_opt.compression = m_options.compression;
    m_cf_opt.bottommost_compression = m_options.bottommost_compression;

    m_cf_opt.compression_opts.enabled = m_options.compression_opts.enabled;
    m_cf_opt.bottommost_compression = m_options.bottommost_compression;

    m_cf_opt.num_levels = m_options.num_levels;
    m_cf_opt.compression_per_level = m_options.compression_per_level;

    m_cf_opt.level_compaction_dynamic_level_bytes = m_options.level_compaction_dynamic_level_bytes;

    open();
}

xdb::xdb_impl::~xdb_impl() {
    close();
}

void xdb::xdb_impl::handle_error(const rocksdb::Status& status) const {
    if (status.ok())
        return;
    const string errmsg = "[xdb] rocksDB error: " + status.ToString() + " ,db name " + m_db_name;
    xwarn("%s", errmsg.c_str());
    // throw xdb_error(errmsg);
}

bool xdb::xdb_impl::read(const std::string& key, std::string& value) const {
    rocksdb::Status s = m_db->Get(rocksdb::ReadOptions(), m_handles[0], rocksdb::Slice(key), &value);
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
    rocksdb::Status s = m_db->Put(rocksdb::WriteOptions(), m_handles[0], rocksdb::Slice(key), rocksdb::Slice(value));
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::write(const std::string& key, const char* data, size_t size) {
    rocksdb::Status s = m_db->Put(rocksdb::WriteOptions(), m_handles[0], rocksdb::Slice(key), rocksdb::Slice(data, size));
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::write(const std::map<std::string, std::string>& batches) {
    rocksdb::WriteBatch batch;
    for (const auto& entry: batches) {
        batch.Put(m_handles[0], entry.first, entry.second);
    }
    rocksdb::Status s = m_db->Write(rocksdb::WriteOptions(), &batch);
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::erase(const std::string& key) {
    rocksdb::Status s = m_db->Delete(rocksdb::WriteOptions(), m_handles[0], rocksdb::Slice(key));
    if (!s.ok()) {
        if (!s.IsNotFound()) {
            handle_error(s);
        }
        return false;
    }
    return true;
}

bool xdb::xdb_impl::erase(const std::vector<std::string>& keys) {
    rocksdb::WriteBatch batch;
    for (const auto& key: keys) {
        batch.Delete(m_handles[0], key);
    }
    rocksdb::Status s = m_db->Write(rocksdb::WriteOptions(), &batch);
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) {
    rocksdb::WriteBatch batch;
    for (const auto& entry: objs) {
        batch.Put(m_handles[0], entry.first, entry.second);
    }
    for (const auto& key: delete_keys) {
        batch.Delete(m_handles[0], key);
    }
    rocksdb::Status s = m_db->Write(rocksdb::WriteOptions(), &batch);
    handle_error(s);
    return s.ok();
}

bool xdb::xdb_impl::single_delete(const std::string& key)
{
    if(key.empty() || (NULL == m_db))
        return false;
 
    rocksdb::Status res = m_db->SingleDelete(rocksdb::WriteOptions(), m_handles[0], rocksdb::Slice(key));
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
    if(begin_key.empty() || end_key.empty() || (NULL == m_db))
        return false;
    
    rocksdb::Status res = m_db->DeleteRange(rocksdb::WriteOptions(),m_handles[0],rocksdb::Slice(begin_key), rocksdb::Slice(end_key));
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
    bool ret = false;
    auto iter = m_db->NewIterator(rocksdb::ReadOptions(), m_handles[0]);

    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        values.push_back(iter->value().ToString());
        ret = true;
    }
    delete iter;
    return ret;
}

xdb::xdb(const std::string& name)
: m_db_impl(new xdb_impl(name)) {
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

}  // namespace ledger
}  // namespace top
