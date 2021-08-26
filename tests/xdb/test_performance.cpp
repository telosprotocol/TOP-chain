#include <vector>
#include <algorithm>
#include <stdio.h>

#include "gtest/gtest.h"
#include "xbase/xutl.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"

using namespace top::db;
using namespace std;

static string DB_NAME  = "./test_db_1/";

class test_performance : public testing::Test {
protected:
    void SetUp() override {
        string db_dir = DB_NAME;
        remove(db_dir.c_str());
		xset_log_level(enum_xlog_level_error);
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


TEST_F(test_performance, db_write_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db, nullptr);

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
    ASSERT_NE(db, nullptr);

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
    ASSERT_NE(db, nullptr);
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
    ASSERT_NE(db, nullptr);
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
    ASSERT_NE(db, nullptr);
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
    ASSERT_NE(db, nullptr);
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
    ASSERT_NE(db, nullptr);
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
    ASSERT_NE(db, nullptr);
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

