#include "gtest/gtest.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"

#include <algorithm>
#include <vector>

using namespace top::db;

namespace top {

static std::string MERGE_TEST_DB_NAME = "/tmp/test_merge_operator";

class test_merge_operator : public testing::Test {
public:
    void SetUp() override {
        std::string db_dir = MERGE_TEST_DB_NAME;
        m_db = xdb_factory_t::create_kvdb(db_dir);
    }

    void TearDown() override {
    }

    std::shared_ptr<xdb_face_t> m_db;
};

void print(std::string const & value) {
    base::xauto_ptr<base::xstrmap_t> data_r = new base::xstrmap_t();
    data_r->serialize_from_string(value);
    std::cout << "==========" << std::endl;
    for (auto _p : data_r->get_map()) {
        std::cout << _p.first << " " << _p.second << std::endl;
    }
    std::cout << "==========" << std::endl;
}

TEST_F(test_merge_operator, normal_use) {
    base::xauto_ptr<base::xstrmap_t> data = new base::xstrmap_t();
    data->set("inner_key1", "value1");
    std::string data_str;
    data->serialize_to_string(data_str);

    m_db->erase("key1");
    std::string read_value;

    // ===
    ASSERT_TRUE(m_db->write("key1", data_str));

    ASSERT_TRUE(m_db->read("key1", read_value));
    print(read_value);

    // ===
    data->set("inner_key2", "value2");
    data->serialize_to_string(data_str);
    ASSERT_TRUE(m_db->merge("key1", data_str));

    ASSERT_TRUE(m_db->read("key1", read_value));
    print(read_value);

    // ===
    data->clear();
    data->set("inner_key3", "value3");
    data->serialize_to_string(data_str);
    ASSERT_TRUE(m_db->merge("key1", data_str));

    ASSERT_TRUE(m_db->read("key1", read_value));
    print(read_value);
}

TEST_F(test_merge_operator, merge_into_empty_is_ok) {
    base::xauto_ptr<base::xstrmap_t> data = new base::xstrmap_t();
    data->set("inner_key1", "value1");
    std::string data_str;
    data->serialize_to_string(data_str);

    m_db->erase("key1");
    std::string read_value;

    // ===
    ASSERT_TRUE(m_db->merge("key1", data_str));  // use merge first time

    ASSERT_TRUE(m_db->read("key1", read_value));
    print(read_value);

    // ===
    data->set("inner_key2", "value2");
    data->serialize_to_string(data_str);
    ASSERT_TRUE(m_db->merge("key1", data_str));

    ASSERT_TRUE(m_db->read("key1", read_value));
    print(read_value);
}

TEST_F(test_merge_operator, merge_will_overwrite_same_inner_key) {
    base::xauto_ptr<base::xstrmap_t> data = new base::xstrmap_t();
    data->set("inner_key1", "value1");
    std::string data_str;
    data->serialize_to_string(data_str);

    m_db->erase("key1");
    std::string read_value;

    // ===
    ASSERT_TRUE(m_db->merge("key1", data_str));  // use merge first time

    ASSERT_TRUE(m_db->read("key1", read_value));
    print(read_value);

    // ===
    data->set("inner_key2", "value2");
    data->set("inner_key1", "new_value1"); // overwrite inner_key1
    data->serialize_to_string(data_str);
    ASSERT_TRUE(m_db->merge("key1", data_str));

    ASSERT_TRUE(m_db->read("key1", read_value));
    print(read_value);
}

}  // namespace top