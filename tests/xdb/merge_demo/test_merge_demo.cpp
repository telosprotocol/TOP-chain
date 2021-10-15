
#include "rocksdb/db.h"
#include "rocksdb/merge_operator.h"

#include <assert.h>

#include <gtest/gtest.h>

#include <string>
#include <thread>

using namespace rocksdb;
using std::string;

bool Deserialize(const Slice & existing_value, uint64_t & res) {
    res = std::atoi(std::string{existing_value.data(), existing_value.size()}.c_str());
    // res = res_;
    return true;
}

std::string Serialize(uint64_t value) {
    return std::to_string(value);
}

class UInt64AddOperator : public AssociativeMergeOperator {
public:
    virtual bool Merge(const Slice & key, const Slice * existing_value, const Slice & value, std::string * new_value, Logger * logger) const override {
        // assuming 0 if no existing value
        uint64_t existing = 0;
        if (existing_value) {
            if (!Deserialize(*existing_value, existing)) {
                // if existing_value is corrupted, treat it as 0
                Log(logger, "existing value corruption");
                existing = 0;
            }
        }

        uint64_t oper = 0;
        if (!Deserialize(value, oper)) {
            // if operand is corrupted, treat it as 0
            Log(logger, "operand value corruption");
            oper = 0;
        }

        auto res = existing + oper;
        // std::cout << "merge result: " << res << " existing:" << existing << " oper:" << oper << std::endl;
        *new_value = Serialize(res);
        return true;  // always return true for this, since we treat all errors as "zero".
    }

    virtual const char * Name() const override {
        return "UInt64AddOperator";
    }
};
class Counters {
public:
    virtual ~Counters() {
    }
    // (re)set the value of a named counter
    virtual void Set(const string & key, uint64_t value) = 0;

    // remove the named counter
    virtual void Remove(const string & key) = 0;

    // retrieve the current value of the named counter, return false if not found
    virtual bool Get(const string & key, uint64_t & value) = 0;
};
class RocksCounters : public Counters {
public:
    static const uint64_t kDefaultCount = 0;
    RocksCounters(std::shared_ptr<DB> db) : db_{db} {
    }

    // mapped to a RocksDB Put
    virtual void Set(const string & key, uint64_t value) {
        string serialized = Serialize(value);
        db_->Put(rocksdb::WriteOptions(), key, serialized);
    }

    // mapped to a RocksDB Delete
    virtual void Remove(const string & key) {
        db_->Delete(rocksdb::WriteOptions(), key);
    }

    // mapped to a RocksDB Get
    virtual bool Get(const string & key, uint64_t & value) {
        string str;
        auto s = db_->Get(rocksdb::ReadOptions(), key, &str);
        if (s.ok()) {
            Deserialize(str, value);
            return true;
        } else {
            return false;
        }
    }

    void Add(const string & key, uint64_t value) {
        string serialized = Serialize(value);
        db_->Merge(rocksdb::WriteOptions(), key, serialized);
    }

    std::shared_ptr<DB> db_;
};

void add_thread(RocksCounters * cnt, int num, int expected_max) {
    std::cout << " thread start write" << std::endl;
    for (auto index = 0; index < num; ++index) {
        cnt->Add("a", 1);
        // std::this_thread::sleep_for(std::chrono::milliseconds{10});
        uint64_t v;
        cnt->Get("a", v);
        if (v > (uint64_t)expected_max)
            std::cout << " detected write thread read wrong value" << v << std::endl;
    }
    std::cout << " thread write count " << num << " exit" << std::endl;
}

TEST(test, test_rockdb_merge_multi_thread_safe) {
    DB * dbp;
    Options options;
    options.merge_operator.reset(new UInt64AddOperator);
    options.create_if_missing = true;
    auto status = rocksdb::DB::Open(options, "/tmp/test_merge_demo", &dbp);
    assert(status.ok());
    std::shared_ptr<DB> db(dbp);
    RocksCounters counters(db);

    counters.Set("a", 0);
    uint64_t v;
    counters.Get("a", v);
    std::cout << v << std::endl;

    std::cout << "========== start_write ========== " << std::endl;

    int add_cnt = 10000;

    std::thread t1 = std::thread(&add_thread, &counters, add_cnt, 4 * add_cnt);
    t1.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    std::thread t2 = std::thread(&add_thread, &counters, add_cnt, 4 * add_cnt);
    t2.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    std::thread t3 = std::thread(&add_thread, &counters, add_cnt, 4 * add_cnt);
    t3.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    std::cout << " main thread start write" << std::endl;
    for (auto index = 0; index < add_cnt; ++index) {
        // std::this_thread::sleep_for(std::chrono::milliseconds{10});
        counters.Add("a", 1);
        // std::this_thread::sleep_for(std::chrono::milliseconds{100});
        uint64_t v;
        counters.Get("a", v);
        // std::cout << v << std::endl;

        if (v > (uint64_t)4 * add_cnt)
            std::cout << " detected main thread read wrong value" << v << std::endl;
    }
    std::cout << " main thread end write" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds{10});

    std::cout << "========== end_write ========== " << std::endl;
    counters.Get("a", v);
    std::cout << v << std::endl;
    counters.Get("a", v);
    std::cout << v << std::endl;
    counters.Get("a", v);
    std::cout << v << std::endl;
    // counters.Set("a", 2);
    // counters.Get("a", v);
    // std::cout << v << std::endl;
}