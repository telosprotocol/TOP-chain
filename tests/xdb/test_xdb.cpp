#include <vector>
#include <algorithm>
#include <stdio.h>

#include "gtest/gtest.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"
#include "xdb/xdb_face.h"

using namespace top::db;
using namespace std;

static string DB_NAME  = "./test_db_10/";

class test_xdb : public testing::Test {
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

TEST_F(test_xdb, db_open_close_1) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    db->write("1", "aa");
    db = nullptr;
    db = xdb_factory_t::create_kvdb(db_dir);
    std::string value;
    auto ret = db->read("1", value);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(value, "aa");
}

TEST_F(test_xdb, db_open_close_2) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    size_t count = 50;
    for(size_t i = 0; i < count; i++)
    {
        db->close();
        db->open();
        db->write("1", to_string(i));
    }

    std::string value;
    auto ret = db->read("1", value);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(value, to_string(count-1));
}

TEST_F(test_xdb, db_create_destroy) {
    string db_dir = DB_NAME;

    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);
    db->write("1", "aa");
    db = nullptr;

    xdb::destroy(db_dir);
    db = xdb_factory_t::create_kvdb(db_dir);
    std::string value;
    auto ret = db->read("1", value);
    ASSERT_EQ(ret, false);
    db = nullptr;
    //ASSERT_THROW(xdb db3("/xxx/mydb2"), xdb_error);
}

TEST_F(test_xdb, db_write_read_same_key_100000_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);

    int count = 100000;
    string key = "key";
    string value = "value";
    for(int i=0; i<count; i++)
    {
        db->write(key, value + to_string(i));
    }
    db->write(key, value + to_string(count));

    std::string value1;
    auto ret = db->read(key, value1);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(value1, value + to_string(count));

    ret = db->exists(key);
    ASSERT_EQ(ret, true);
    db->erase(key);
    ret = db->exists(key);
    ASSERT_EQ(ret, false);
}

TEST_F(test_xdb, db_write_read_different_key_100000_BENCH) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);

    int count = 100000;
    string key = "key";
    string value = "value";
    for(int i=0; i<count; i++)
    {
        db->write(key + to_string(i), value + to_string(i));
    }

    std::string value1;
    for(int i=0; i<count; i++)
    {
        auto ret = db->read(key + to_string(i), value1);
        ASSERT_EQ(ret, true);
        ASSERT_EQ(value1, value + to_string(i));
    }

    for(int i=0; i<count; i++)
    {
        db->erase(key + to_string(i));
    }

    for(int i=0; i<count; i++)
    {
        auto ret = db->read(key + to_string(i), value1);
        ASSERT_EQ(ret, false);
    }
}

const char test_kHexLookup[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7,  8,  9,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };

std::string test_HexDecode(const std::string& str) {
    auto size(str.size());
    if (size % 2) return "";

    std::string non_hex_output(size / 2, 0);
    for (size_t i(0), j(0); i != size / 2; ++i) {
        non_hex_output[i] = (test_kHexLookup[static_cast<int>(str[j++])] << 4);
        non_hex_output[i] |= test_kHexLookup[static_cast<int>(str[j++])];
    }
    return non_hex_output;
}

TEST_F(test_xdb, db_write_with_binary_key) {
    string db_dir = DB_NAME;
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db.get(), nullptr);

    string key = test_HexDecode("1a021a020ffe1abd1a08215353c233d6e009613e95eec4253832a761af28ff37ac5a150c");
    string value = "22222";
    db->write(key, value);

    std::string value1;
    auto ret = db->read(key, value1);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(value1, value);
}

TEST_F(test_xdb, db_erase_one_key) {
    std::vector<xdb_path_t> db_paths;
    xdb db1(xdb_kind_t::xdb_kind_kvdb, DB_NAME,db_paths);
    {
        string value;
        db1.write("key3","value3");
        bool ret = db1.read("key3", value);
        ASSERT_TRUE(ret);
        ASSERT_EQ(value, "value3");
    }
    {
        string value;
        db1.erase("key3");
        bool ret = db1.read("key3", value);
        ASSERT_FALSE(ret);
        ASSERT_TRUE(value.empty());
    }
    {
        string value;
        db1.write("key3","value3");
        bool ret = db1.read("key3", value);
        ASSERT_TRUE(ret);
        ASSERT_EQ(value, "value3");
    }
}

TEST_F(test_xdb, db_erase_multi_key) {
    std::vector<xdb_path_t> db_paths;
    xdb db1(xdb_kind_kvdb,DB_NAME,db_paths);
    {
        string value;
        std::string key("multikey1");
        db1.write(key, key);
        bool ret = db1.read(key, value);
        ASSERT_TRUE(ret);
        ASSERT_EQ(value, key);
    }
    {
        string value;
        std::string key("multikey2");
        db1.write(key, key);
        bool ret = db1.read(key, value);
        ASSERT_TRUE(ret);
        ASSERT_EQ(value, key);
    }
    {
        string value;
        std::string key("multikey3");
        db1.write(key, key);
        bool ret = db1.read(key, value);
        ASSERT_TRUE(ret);
        ASSERT_EQ(value, key);
    }
    {
        string value;
        std::vector<std::string> keys{"multikey1", "multikey2", "multikey3"};
        db1.erase(keys);
        bool ret = db1.read(keys[0], value);
        ASSERT_FALSE(ret);
        ASSERT_TRUE(value.empty());
    }
    {
        string value;
        db1.erase("non_exist_key");
    }
    {
        std::vector<std::string> keys{"multikey1", "multikey2", "multikey3"};
        db1.erase(keys);
    }
}

TEST_F(test_xdb, db_read_range) {
    std::vector<xdb_path_t> db_paths;
    xdb db1(xdb_kind_kvdb,DB_NAME,db_paths);

    db1.write("key11", "value11");
    db1.write("key21", "value21");
    db1.write("key12", "value12");
    db1.write("key22", "value22");
    db1.write("key13", "value13");
    db1.write("key23", "value23");
    db1.write("key14", "value14");

    {
        vector<string> values;
        auto ret = db1.read_range("key1", values);
        ASSERT_TRUE(ret);
        ASSERT_EQ(values.size(), 4);
        vector<string>::iterator iter;
        iter = std::find(values.begin(), values.end(), "value11");
        ASSERT_TRUE(iter != values.end());
        iter = std::find(values.begin(), values.end(), "value12");
        ASSERT_TRUE(iter != values.end());
        iter = std::find(values.begin(), values.end(), "value13");
        ASSERT_TRUE(iter != values.end());
        iter = std::find(values.begin(), values.end(), "value14");
        ASSERT_TRUE(iter != values.end());
        iter = std::find(values.begin(), values.end(), "value21");
        ASSERT_TRUE(iter == values.end());
        iter = std::find(values.begin(), values.end(), "value22");
        ASSERT_TRUE(iter == values.end());
        iter = std::find(values.begin(), values.end(), "value23");
        ASSERT_TRUE(iter == values.end());
    }

    {
        vector<string> values;
        auto ret = db1.read_range("key2", values);
        ASSERT_TRUE(ret);
        ASSERT_EQ(values.size(), 3);
        vector<string>::iterator iter;
        iter = std::find(values.begin(), values.end(), "value11");
        ASSERT_TRUE(iter == values.end());
        iter = std::find(values.begin(), values.end(), "value12");
        ASSERT_TRUE(iter == values.end());
        iter = std::find(values.begin(), values.end(), "value13");
        ASSERT_TRUE(iter == values.end());
        iter = std::find(values.begin(), values.end(), "value14");
        ASSERT_TRUE(iter == values.end());
        iter = std::find(values.begin(), values.end(), "value21");
        ASSERT_TRUE(iter != values.end());
        iter = std::find(values.begin(), values.end(), "value22");
        ASSERT_TRUE(iter != values.end());
        iter = std::find(values.begin(), values.end(), "value23");
        ASSERT_TRUE(iter != values.end());
    }
}
/*TEST_F(test_xdb, db_backup) {
    string db_dir = DB_NAME;

    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_dir);
    ASSERT_NE(db, nullptr);
    db->write("1", "aa");

    db->backup("./test_db_backup/");
}
TEST_F(test_xdb, db_restore) {
    string db_dir = DB_NAME;

    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb("./test_db_backup/");
    ASSERT_NE(db, nullptr);

    db->restore("./test_db_backup/", "./restore_db1/");
}
TEST_F(test_xdb, db_restore_check) {
    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb("./restore_db1/");
	std::string value;
	auto ret = db->read("1", value);
	ASSERT_EQ(ret, true);
	ASSERT_EQ(value, "aa");
}*/
