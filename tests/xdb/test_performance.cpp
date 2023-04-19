#include <vector>
#include <algorithm>
#include <stdio.h>

#include "gtest/gtest.h"
#include "xbase/xutl.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"
#include "xdb/xdb_face.h"
#include "xvledger/xvdbkey.h"
#include "xmetrics/xmetrics.h"

using namespace top::db;
using namespace std;
using namespace top::metrics;

static string DB_NAME  = "./test_db_1/";

class test_performance : public testing::Test {
protected:
    void SetUp() override {
        string db_dir = DB_NAME;
        remove(db_dir.c_str());
		//xset_log_level(enum_xlog_level_error);
    }

    void TearDown() override {
        // xdb::destroy(DB_NAME);
        // remove(DB_NAME.c_str());
    }
};


void test_db_performance_write_read_same_key(const std::shared_ptr<xdb_face_t>& db, int64_t count) {
    int64_t begin = top::base::xtime_utl::gmttime_ms();

    string key = "key";
    for (int i=0; i < count; i++) {
        std::string value;
        if (db->read(key, value)) {
            value = key + to_string(i);
        }
        db->write(key, value);
    }

    int64_t end = top::base::xtime_utl::gmttime_ms();
    std::cout << "test_db_write_same_key count:" << count << " ms:" << end-begin << " qps:" << count*1000/(end-begin) << std::endl;
    db->erase(key);
}

void test_db_performance_write_read_different_key(const std::shared_ptr<xdb_face_t>& db, int64_t count) {
    int64_t begin = top::base::xtime_utl::gmttime_ms();

    string key;
    for (int i=0; i < count; i++) {
        key = to_string(i);
        std::string value;
        if (db->read(key, value)) {
            value = key + to_string(i);
        }
        db->write(key, value);
    }

    int64_t end = top::base::xtime_utl::gmttime_ms();
    std::cout << "test_db_write_read_different_key count:" << count << " ms:" << end-begin << " qps:" << count*1000/(end-begin) << std::endl;
}

int parseLine(char *line) {
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char *p = line;
    while (*p < '0' || *p > '9') p++;
    line[i - 3] = '\0';
    i = atoi(p);
    return i;
}
typedef struct {
    uint32_t virtualMem;
    uint32_t physicalMem;
} processMem_t;

processMem_t GetProcessMemory() {
    FILE *file = fopen("/proc/self/status", "r");
    char line[128];
    processMem_t processMem;

    bool found_vmsize = false;
    bool found_vmRSS = false;
    while (fgets(line, 128, file) != NULL) {
        if (!found_vmsize) {
            if (strncmp(line, "VmSize:", 7) == 0) {
                processMem.virtualMem = parseLine(line);
                found_vmsize = true;
            }            
        }

        if (!found_vmRSS) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                processMem.physicalMem = parseLine(line);
                found_vmRSS = true;
            }
        }

        if (found_vmsize && found_vmRSS) {
            break;
        }
    }
    fclose(file);
    return processMem;
}

TEST_F(test_performance, db_write_endless_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db, nullptr);

	int64_t begin = top::base::xtime_utl::gmttime_ms();
	string key;
	std::string value(256, 'A');
    int64_t lasttime = begin;
    uint32_t i = 0;
    while(1) {
		key = "key"+to_string(i);
		db->write(key, value);
        if (i%1000000 == 0) {
            int64_t cur = top::base::xtime_utl::gmttime_ms();
            auto mem = GetProcessMemory();
#ifdef ENABLE_METRICS
            db->GetDBMemStatus();
            uint32_t mem_block_all = XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_rocksdb_block_cache);
            uint32_t mem_reader_memtable_all = XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_rocksdb_table_readers);
            uint32_t all_mem_tables = XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_rocksdb_all_mem_tables);
            uint32_t pinned = XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_rocksdb_cache_pinned);
            uint32_t total = mem_block_all + mem_reader_memtable_all + all_mem_tables + pinned;
            std::cout << "i:" << i << ", timecost(ms):" << (cur - lasttime) << ", time begin-cur(s):" << ((cur - begin) / 1000) << ", block:" << mem_block_all
                      << ", reader_memtable:" << mem_reader_memtable_all << ", all_mem_tables:" << all_mem_tables << ", pinned:" << pinned << ", total:" << total
                      << ", physicalMem:" << mem.physicalMem << ", virtualMem:" << mem.virtualMem << std::endl;
#else
            std::cout << "i:" << i << ", timecost(ms):" << (cur - lasttime) << ", time begin-cur(s):" << ((cur - begin) / 1000) << ", physicalMem:" << mem.physicalMem
                      << ", virtualMem:" << mem.virtualMem << std::endl;
#endif
            lasttime = cur;
        }
        i++;
    }
}

// batch write has better performance.
TEST_F(test_performance, db_batch_write_endless_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db, nullptr);

	int64_t begin = top::base::xtime_utl::gmttime_ms();
	string key;
	std::string value(256, 'A');
    int64_t lasttime = begin;
    uint32_t i = 0;
    std::map<std::string, std::string> batch_kv;
    std::vector<std::string> empty_keys;
    while(1) {
		key = "key"+to_string(i);
        batch_kv[key] = value;
        if (batch_kv.size() >= 100) {
            db->batch_change(batch_kv, empty_keys);
            batch_kv.clear();
        }
        if (i%1000000 == 0) {
            int64_t cur = top::base::xtime_utl::gmttime_ms();
            auto mem = GetProcessMemory();
#ifdef ENABLE_METRICS
            db->GetDBMemStatus();
            uint32_t mem_block_all = XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_rocksdb_block_cache);
            uint32_t mem_reader_memtable_all = XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_rocksdb_table_readers);
            uint32_t all_mem_tables = XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_rocksdb_all_mem_tables);
            uint32_t pinned = XMETRICS_GAUGE_GET_VALUE(xmetrics_tag_t::db_rocksdb_cache_pinned);
            uint32_t total = mem_block_all + mem_reader_memtable_all + all_mem_tables + pinned;
            std::cout << "i:" << i << ", timecost(ms):" << (cur - lasttime) << ", time begin-cur(s):" << ((cur - begin) / 1000) << ", block:" << mem_block_all
                      << ", reader_memtable:" << mem_reader_memtable_all << ", all_mem_tables:" << all_mem_tables << ", pinned:" << pinned << ", total:" << total
                      << ", physicalMem:" << mem.physicalMem << ", virtualMem:" << mem.virtualMem << std::endl;
#else
            std::cout << "i:" << i << ", timecost(ms):" << (cur - lasttime) << ", time begin-cur(s):" << ((cur - begin) / 1000) << ", physicalMem:" << mem.physicalMem
                      << ", virtualMem:" << mem.virtualMem << std::endl;
#endif
            lasttime = cur;
        }
        i++;
    }
}

TEST_F(test_performance, db_write_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);

	int64_t begin = top::base::xtime_utl::gmttime_ms();
	uint32_t count = 2000000;
	string key;
	std::string value(128, 'A');
	for (uint32_t i=0; i < count; i++) {
		key = "key"+to_string(i);
		db->write(key, value);
	}

	int64_t end = top::base::xtime_utl::gmttime_ms();
	std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;

}

TEST_F(test_performance, db_read_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);

	int64_t begin = top::base::xtime_utl::gmttime_ms();
	uint32_t count = 2000000;
	string key = "key";
	std::string value;

    for (uint32_t i=0; i < count; i++) {
        key = "key" + to_string(i);
        if (!db->read(key, value)) {
//            value = key + to_string(i);
			std::cout << "read fail"<<std::endl;
        }
    }
	int64_t end = top::base::xtime_utl::gmttime_ms();
	std::cout << "db_read count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;

}

TEST_F(test_performance, db_write_and_delete_1_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    uint32_t count = 2000000;
    string key;
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read unexist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'A');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->erase(key);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_delete exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read unexist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
}

TEST_F(test_performance, db_write_and_delete_2_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    uint32_t count = 2000000;
    string key;

    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'A');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        uint32_t batch_count = 20;
        uint32_t i_count = count/batch_count;
        for (uint32_t i=0; i < i_count; i++) {
            std::vector<std::string> delete_keys;
            uint32_t begin_j = i*batch_count;
            uint32_t end_j = (i+1)*batch_count;
            for (uint32_t j=begin_j; j<end_j;j++) {
                key = "key"+to_string(j);
                delete_keys.push_back(key);
            }
            db->batch_change({}, delete_keys);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_batch_delete 20 exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read unexist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
}

TEST_F(test_performance, db_write_and_delete_3_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    uint32_t count = 2000000;
    string key;

    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'A');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }    
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        for (uint32_t i=0; i < count; i++) {
            key = "unkey"+to_string(i);
            db->erase(key);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_delete unexist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }    
}

TEST_F(test_performance, db_write_and_delete_4_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    uint32_t count = 2000000;
    string key;

    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'A');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        uint32_t batch_count = 100;
        uint32_t i_count = count/batch_count;
        for (uint32_t i=0; i < i_count; i++) {
            std::vector<std::string> delete_keys;
            uint32_t begin_j = i*batch_count;
            uint32_t end_j = (i+1)*batch_count;
            for (uint32_t j=begin_j; j<end_j;j++) {
                key = "key"+to_string(j);
                delete_keys.push_back(key);
            }
            db->batch_change({}, delete_keys);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_batch_delete 100 exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read unexist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
}

TEST_F(test_performance, db_write_and_delete_5_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    uint32_t count = 2000000;
    string key;

    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'A');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        uint32_t batch_count = 5;
        uint32_t i_count = count/batch_count;
        for (uint32_t i=0; i < i_count; i++) {
            std::vector<std::string> delete_keys;
            uint32_t begin_j = i*batch_count;
            uint32_t end_j = (i+1)*batch_count;
            for (uint32_t j=begin_j; j<end_j;j++) {
                key = "key"+to_string(j);
                delete_keys.push_back(key);
            }
            db->batch_change({}, delete_keys);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_batch_delete 5 exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read unexist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
}

TEST_F(test_performance, db_write_and_delete_6_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    uint32_t count = 2000000;
    string key;
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read unexist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'A');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'B');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'C');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }    
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'D');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }        
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string value(128, 'E');
        for (uint32_t i=0; i < count; i++) {
            key = "key"+to_string(i);
            db->write(key, value);
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_write count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }
    {
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        std::string rvalue;
        for (uint32_t i=0; i < count; i++) {
            key = "key" + to_string(i);
            db->read(key, rvalue);
            // xassert(!rvalue.empty());
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << "db_read exist count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    }       
}

TEST_F(test_performance, db_write_single_path) {
    xinfo("db_write_single_path start:"); 
    string db_dir[2] = {"./db_single/", "./db_single_2/"};
    for (int j = 0; j < 2; j++) {   
        std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir[j]);
        ASSERT_NE(db.get(), nullptr);
        uint32_t count = 20001;
        std::string value(2048, 'A');
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        for (uint32_t i=0; i < count; i++) {
            std::string keyStr = std::to_string(i);
            db->write(keyStr, value);
            if ((i % 1000000) == 0) {
               db->GetDBMemStatus();
            }
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << db_dir[j] << " db_write_single_path count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
        xinfo("db_dir %s db_write_single_path count: %ld time(ms): %ld qps: %lf ." ,db_dir[j].c_str(), count, end-begin ,   1.0*count/(end-begin)*1000);
    }
    
}

TEST_F(test_performance, db_read_single_path) {
    string db_dir[2] = {"./db_single/",  "./db_single_2/"};
    for (int j = 0; j < 2; j++) {   
        std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir[j]);
        ASSERT_NE(db.get(), nullptr);
       
        uint32_t count = 20001;
        std::string value;
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        for (uint32_t i=0; i < count; i++) {
            std::string keyStr = std::to_string(i);
            if (!db->read(keyStr, value)) {
			    std::cout << "read fail"<<std::endl;
                 break;
            }
            if ((i % 1000000) == 0) {
               db->GetDBMemStatus();
            }
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << db_dir[j] << " db_read_single_path count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
        xinfo("db_dir %s db_read_single_path count: %ld time(ms): %ld qps: %lf ." ,db_dir[j].c_str() , count, end-begin ,   1.0*count/(end-begin)*1000);
    }
 
}


TEST_F(test_performance, db_write_mult_path) {
    std::string db_dir = "./db_mult";
    std::string key_db_path[2] = { "./db_mult_db_1", "./db_mult_db_2"};
    int path_num = 2;
    std::vector<top::db::xdb_path_t> db_data_paths;
    for (int i = 0; i < path_num; i++) {
        uint64_t key_db_size = 0;
        key_db_size = 10L << 30 << i;
        db_data_paths.emplace_back(key_db_path[i], key_db_size); 
    }
  
    std::shared_ptr< top::db::xdb_face_t> db = top::db::xdb_factory_t::instance(db_dir, db_data_paths);
    ASSERT_NE(db.get(), nullptr);

	int64_t begin = top::base::xtime_utl::gmttime_ms();
	uint32_t count = 20001;
	string key;
	std::string value(2048, 'A');
	for (uint32_t i=0; i < count; i++) {
        std::string keyStr = std::to_string(i);
        db->write(keyStr, value);
        if ((i % 1000000) == 0) {
            db->GetDBMemStatus();
        }
	}
    int64_t end = top::base::xtime_utl::gmttime_ms();
    std::cout << "db_write_mult_path count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    xinfo("db_dir %s db_write_mult_path count: %ld time(ms): %ld qps: %lf ." ,db_dir.c_str(), count, end-begin ,  1.0*count/(end-begin)*1000);
}

TEST_F(test_performance, db_read_mult_path) {
    std::string db_dir = "./db_mult";
    std::string key_db_path[2] = { "./db_mult_db_1", "./db_mult_db_2"};
    int path_num = 2;
    std::vector<top::db::xdb_path_t> db_data_paths;
    for (int i = 0; i < path_num; i++) {
        uint64_t key_db_size = 0;
        key_db_size = 10L << 30 << i;
        db_data_paths.emplace_back(key_db_path[i], key_db_size); 
    }
  
    std::shared_ptr< top::db::xdb_face_t> db = top::db::xdb_factory_t::instance(db_dir, db_data_paths);
    ASSERT_NE(db.get(), nullptr);

	int64_t begin = top::base::xtime_utl::gmttime_ms();
	uint32_t count = 20001;
    std::string result;
	for (uint32_t i=0; i < count; i++) {
        std::string keyStr = std::to_string(i);
        if (!db->read(keyStr, result)) {
            std::cout << "read fail"<<std::endl;
            break;
        }
        if ((i % 1000000) == 0) {
            db->GetDBMemStatus();
        }
	}
	int64_t end = top::base::xtime_utl::gmttime_ms();
	std::cout << "db_read_mult_path count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
    xinfo("db_dir %s db_read_mult_path count: %ld time(ms): %ld qps: %lf ." ,db_dir.c_str(), count, end-begin ,  1.0*count/(end-begin)*1000);
}


TEST_F(test_performance, db_test_db_compactions) {
    string db_dir[4] = {"./db_single_default/", "./db_single_high_compress/", "./db_single_bottom_compress/", "./db_single_no_compress/"};
    for (size_t i = 0; i < 4; i++) {
        remove(db_dir[i].c_str());
    }

    for (int j = 0; j < 4; j++) {   
         int db_kinds = xdb_kind_kvdb;
        if(j > 0){
            db_kinds |= (xdb_kind_high_compress << (j-1));
        }
        std::shared_ptr<xdb_face_t> db = xdb_factory_t::create(db_kinds, db_dir[j]);
        ASSERT_NE(db.get(), nullptr);
       
        uint32_t count = 2000;
        std::string value(2048, 'A');
        int64_t begin = top::base::xtime_utl::gmttime_ms();
        for (uint32_t i=0; i < count; i++) {
            std::string keyStr = top::base::xvdbkey_t::create_prunable_tx_key(std::to_string(i+1000000000));
            if (!db->write(keyStr, value)) {
			    std::cout << "read fail"<<std::endl;
                 break;
            }
        }
        int64_t end = top::base::xtime_utl::gmttime_ms();
        std::cout << db_dir[j] << " db_read_single_path count:" << count << " time(ms):" << end-begin << " qps:" << 1.0*count/(end-begin)*1000 << std::endl;
        xinfo("db_dir %s db_read_single_path count: %ld time(ms): %ld qps: %lf ." ,db_dir[j].c_str() , count, end-begin ,   1.0*count/(end-begin)*1000);
    }
}