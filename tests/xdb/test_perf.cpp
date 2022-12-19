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

class test_perf : public testing::Test {
protected:
    void SetUp() override {
        string db_dir = DB_NAME;
        remove(db_dir.c_str());
    }

    void TearDown() override {
        // xdb::destroy(DB_NAME);
        // remove(DB_NAME.c_str());
    }
};


void test_db_write_read_same_key(const std::shared_ptr<xdb_face_t>& db, int64_t count) {
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

void test_db_write_read_different_key(const std::shared_ptr<xdb_face_t>& db, int64_t count) {
    int64_t begin = top::base::xtime_utl::gmttime_ms();

    string key = "key";
    for (int i=0; i < count; i++) {
        std::string key = to_string(i);
        std::string value;
        if (db->read(key, value)) {
            value = key + to_string(i);
        }
        db->write(key, value);
    }

    int64_t end = top::base::xtime_utl::gmttime_ms();
    std::cout << "test_db_write_read_different_key count:" << count << " ms:" << end-begin << " qps:" << count*1000/(end-begin) << std::endl;
}


TEST_F(test_perf, db_write_read_same_key_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    test_db_write_read_same_key(db, 10000);
    test_db_write_read_same_key(db, 100000);
    test_db_write_read_same_key(db, 1000000);
    // test_db_write_read_same_key(db, 10000000);
}

TEST_F(test_perf, db_write_read_different_key_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    test_db_write_read_different_key(db, 10000);
    test_db_write_read_different_key(db, 100000);
    test_db_write_read_different_key(db, 1000000);
    // test_db_write_read_different_key(db, 10000000);
}



