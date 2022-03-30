#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <iostream>
#include <random>
#include <functional>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "gtest/gtest.h"
#include "xdb/xdb_factory.h"

#include "generator.h"

using namespace top::db;
using namespace std;

static string DB_NAME  = "./test_db_perf";
constexpr int key_size = 50;
constexpr int value_size = 100;

class test_transaction_perf : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    static void TearDownTestCase() {
        std::this_thread::sleep_for(chrono::seconds(1));
    }
    static void SetUpTestCase() {
    }
};

void test_transaction_set(std::shared_ptr<xdb_face_t> db, const std::vector<key_value_t>& pairs, int repetitions)
{
    auto begin = std::chrono::steady_clock::now();

    for (int i = 0; i < repetitions; i++) {
        int idx = 0;
        string value = get_random_string(value_size);
        db->write(pairs[idx].m_key, value);
    }
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

    std::stringstream ss;
    ss << std::this_thread::get_id();
    ss << " test_transaction_set ";
    ss << " count: " << repetitions;
    ss << ", time: " << duration << "ms";
    ss  << ", qps: " << repetitions * 1000 / duration << std::endl;

    std::cout << ss.str();
}

TEST_F(test_transaction_perf, db_transaction_write) {
    unsigned n = std::thread::hardware_concurrency();
    int count = 1;

    char db_name[256];
    memset(db_name, 0, sizeof(db_name));
    snprintf(db_name, sizeof(db_name), "%s", DB_NAME.c_str());

    std::shared_ptr<xdb_face_t> db = xdb_factory_t::create_kvdb(db_name);
    std::vector<key_value_t> pairs;
    pairs.reserve(count);
    key_value_pairs_generate(pairs, count, key_size, value_size);
    std::cout << db_name << " begin." << std::endl;
    for (int i = 0; i < count; i++) {
        db->write(pairs[i].m_key, pairs[i].m_value);
    }

    std::vector<std::thread> db_thread(n);
    int repetitions = 20000;

    for (unsigned i = 0; i < n; ++i) {
        db_thread[i] = std::move(std::thread(std::bind(test_transaction_set, db, std::ref(pairs), repetitions)));
    }

    for (unsigned i = 0; i < n; ++i) {
        db_thread[i].join();
    }
    std::cout << db_name << " done." << std::endl;
    char db_cleanup[512];
    memset(db_cleanup, 0, sizeof(db_cleanup));
    snprintf(db_cleanup, sizeof(db_cleanup), "rm -rf %s", db_name);
    if (system(db_cleanup)) {
        std::cout << db_cleanup << " failed: " << strerror(errno) << std::endl;
    }
}
