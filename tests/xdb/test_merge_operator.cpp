#define METRICS_UNIT_TEST
#ifndef ENABLE_METRICS
#    define ENABLE_METRICS
#endif
#define private public
#include "gtest/gtest.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"
#include "xmetrics/xmetrics.h"

#include <algorithm>
#include <fstream>
#include <vector>

using namespace top::db;

namespace top {

static std::string MERGE_TEST_DB_NAME = "/tmp/test_merge_operator";

class test_merge_operator : public testing::Test {
public:
    void SetUp() override {
        std::size_t test_dump_interval{2};
        XMETRICS_CONFIG_SET("dump_interval", test_dump_interval);
        bool flag{false};
        XMETRICS_CONFIG_SET("dump_full_unit", flag);
        // XMETRICS_INIT();
        std::string db_dir = MERGE_TEST_DB_NAME;
        m_db = xdb_factory_t::create_kvdb(db_dir);
        top::metrics::e_metrics::get_instance().m_metrics_hub.clear();
        top::metrics::e_metrics::get_instance().start();
    }

    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        top::metrics::e_metrics::get_instance().stop();
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
    data->set("inner_key1", "new_value1");  // overwrite inner_key1
    data->serialize_to_string(data_str);
    ASSERT_TRUE(m_db->merge("key1", data_str));

    ASSERT_TRUE(m_db->read("key1", read_value));
    print(read_value);
}

std::string rand_str(std::size_t sz) {
    static std::string v = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::string res = "";
    while (res.size() < sz) {
        res.push_back(v[rand() % 60]);
    }
    return res;
}

void checkhashlist(std::size_t num) {
    std::ifstream read_file;
    read_file.open("./txhashlist.log");
    read_file.seekg(0, std::ios_base::end);
    auto len = read_file.tellg();
    std::cout << len << std::endl;
    if (len != 3500000) {
        std::cout << "rewrite hash file" << std::endl;
        read_file.close();
        std::ofstream write_file;
        write_file.open("./txhashlist.log");
        for (std::size_t index = 0; index < num; ++index) {
            std::string rand_tx_hash = rand_str(34);
            write_file << rand_tx_hash << std::endl;
        }
        write_file.close();
    } else {
        int cnt = 0;
        read_file.seekg(0, std::ios_base::beg);
        for (std::size_t index = 0; index < 10; ++index) {
            std::string _;
            read_file >> _;
            std::cout << index << " : " << _ << std::endl;
        }
        read_file.close();
    }
}

std::map<std::string, std::string> generate_rand_put_data(std::size_t num) {
    static std::string perfix[3] = {"send", "recv", "confirm"};
    std::map<std::string, std::string> res;
    std::ifstream read_file;
    read_file.open("./txhashlist.log");
    read_file.seekg(0, std::ios_base::beg);
    for (std::size_t index = 0; index < num; ++index) {
        std::string _;
        read_file >> _;
        for (auto _i = 0; _i < 3; _i++) {
            res.insert({perfix[_i] + _, rand_str(114)});
        }
    }
    read_file.close();
    return res;
}

std::vector<std::pair<std::string, std::string>> generate_rand_merge_data(std::size_t num) {
    std::vector<std::pair<std::string, std::string>> res;
    std::ifstream read_file;
    read_file.open("./txhashlist.log");
    read_file.seekg(0, std::ios_base::beg);
    std::string key = "";
    for (std::size_t index = 0; index < num; ++index) {
        static std::string perfix[3] = {"send", "recv", "confirm"};
        static int value_size[3] = {80, 80, 40};
        read_file >> key;
        for (auto _i = 0; _i < 3; _i++) {
            base::xauto_ptr<base::xstrmap_t> data = new base::xstrmap_t();
            data->set(perfix[_i], rand_str(value_size[_i]));
            std::string data_str;
            data->serialize_to_string(data_str);
            res.push_back({key, data_str});
        }
        key = "";
    }
    read_file.close();
    assert(res.size() == num * 3);
    return res;
}

std::vector<std::string> get_txhash_list(std::size_t num) {
    std::vector<std::string> res;
    std::ifstream read_file;
    read_file.open("./txhashlist.log");
    read_file.seekg(0, std::ios_base::beg);
    for (std::size_t index = 0; index < num; ++index) {
        std::string key = "";
        read_file >> key;
        res.push_back(key);
    }
    read_file.close();
    assert(num == res.size());
    return res;
}

TEST_F(test_merge_operator, set_BENCH) {
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    auto st1 = std::chrono::system_clock::now().time_since_epoch().count();

    std::size_t cnt = 100000;
    checkhashlist(cnt);

    auto data = generate_rand_put_data(cnt);
    auto st2 = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "generate_rand_data finished use: " << st2 - st1 << std::endl;
    for (auto const & _p : data) {
        m_db->write(_p.first, _p.second);
    }
    auto st3 = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "write data(size: " << data.size() << ") finished use: " << st3 - st2 << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto st4 = std::chrono::system_clock::now().time_since_epoch().count();
    for (auto const & _p : data) {
        std::string _;
        m_db->read(_p.first, _);
    }
    auto st5 = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "read data(size: " << data.size() << ") finished use: " << st5 - st4 << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
}


TEST_F(test_merge_operator, read_after_set_BENCH) {
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    auto st1 = std::chrono::system_clock::now().time_since_epoch().count();

    std::size_t cnt = 100000;
    auto data = generate_rand_put_data(cnt);
    
    auto st4 = std::chrono::system_clock::now().time_since_epoch().count();
    for (auto const & _p : data) {
        std::string _;
        m_db->read(_p.first, _);
    }
    auto st5 = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "read data(size: " << data.size() << ") finished use: " << st5 - st4 << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

TEST_F(test_merge_operator, merge_BENCH) {
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    auto st1 = std::chrono::system_clock::now().time_since_epoch().count();

    std::size_t cnt = 100000;
    checkhashlist(cnt);
    auto data = generate_rand_merge_data(cnt);

    auto st2 = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "generate_rand_data finished use: " << st2 - st1 << std::endl;
    for (auto const & _p : data) {
        m_db->merge(_p.first, _p.second);
    }
    auto st3 = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "merge data(size: " << data.size() << ") finished use: " << st3 - st2 << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto hashlist = get_txhash_list(cnt);

    auto st4 = std::chrono::system_clock::now().time_since_epoch().count();
    for (std::size_t index = 0; index < hashlist.size(); index++) {
        std::string _;
        m_db->read(hashlist[index], _);
    }
    auto st5 = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "read data(size: " << hashlist.size() << ") finished use: " << st5 - st4 << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

TEST_F(test_merge_operator, read_after_merge_BENCH) {
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    auto st1 = std::chrono::system_clock::now().time_since_epoch().count();

    std::size_t cnt = 100000;
    auto hashlist = get_txhash_list(cnt);

    auto st4 = std::chrono::system_clock::now().time_since_epoch().count();
    for (std::size_t index = 0; index < hashlist.size(); ++index) {
        std::string _;
        m_db->read(hashlist[index], _);
        // xinfo("read_key: %s value: %s", hashlist[index].c_str(), _.c_str());
    }
    auto st5 = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "read data(size: " << hashlist.size() << ") finished use: " << st5 - st4 << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (std::size_t index = 0; index < 100; ++index) {
        XMETRICS_COUNTER_INCREMENT("test_count_metrics1", static_cast<uint64_t>((100 + rand()) % 10000));
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

TEST_F(test_merge_operator, only_set_data_for_merge_BENCH) {
    std::size_t cnt = 1000000;
    for (std::size_t index = 0; index < cnt; index++) {
        m_db->write(rand_str(34),rand_str(100));
    }
}

}  // namespace top