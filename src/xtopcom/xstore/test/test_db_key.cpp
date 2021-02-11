#include <vector>

#include <sys/time.h>

#include "gtest/gtest.h"
#include "xbase/xdata.h"
#include "xbase/xutl.h"
#include "xdb/xdb_factory.h"

#include "xutility/xhash.h"

using namespace top;
using namespace top::base;
using namespace std;

class test_db_key : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    static void TearDownTestCase() {
    }
    static void SetUpTestCase() {
    }
};

std::string to_db_key_plus(int32_t type, int32_t subtype, const std::string& owner, const std::string& subowner, const std::string& id) {
    return (std::to_string(type) + ":" + std::to_string(subtype) + ":" + owner + ":" + subowner + ":" + id);
}

std::string to_db_key_append(int32_t type, int32_t subtype, const std::string& owner, const std::string& subowner, const std::string& id) {
    std::string db_key;
    db_key.reserve(256);
    db_key.append(std::to_string(type));
    db_key.append(":");
    db_key.append(std::to_string(subtype));
    db_key.append(":");
    db_key.append(owner);
    db_key.append(":");
    db_key.append(subowner);
    db_key.append(":");
    db_key.append(id);

    return std::move(db_key);
}

std::string to_db_key_stringstream(int32_t type, int32_t subtype, const std::string& owner, const std::string& subowner, const std::string& id) {
    std::stringstream ss;
    ss << type << ":" << subtype << ":";
    ss << owner << ":" << subowner << ":" << id;

    return std::move(ss.str());
}

std::string to_db_key_snprintf(int32_t type, int32_t subtype, const std::string& owner, const std::string& subowner, const std::string& id) {
    char buf[256];
    memset(buf, 0, sizeof(buf));

    int n = snprintf(buf, sizeof(buf), "%d:%d:%s:%s:%s", type, subtype, owner.c_str(), subowner.c_str(), id.c_str());
    return std::string(buf, n);
}


void key_test(int32_t type, int32_t subtype, const std::string& owner,
                const std::string& subowner, const std::string& id,
                std::function<std::string(int32_t,int32_t,const std::string&, const std::string&, const std::string&)> f) {
    size_t count = 10000000;

    int opt_stat = 0;

    struct timeval begin, end;
    gettimeofday(&begin, nullptr);

    for (size_t i = 0; i < count; ++i) {
        std::string db_key = f(type, subtype, owner, subowner, id);
        if (!db_key.empty()) {
            ++opt_stat;
        }
    }
    gettimeofday(&end, nullptr);
    uint64_t elapse = (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
    std::cout << count << " keys cost " << elapse << " micro second" << std::endl;
}

// perf test
TEST_F(test_db_key, db_key_stat1_BENCH) {

    int32_t         type = 1;
    int32_t         subtype = 0;
    std::string     owner("T-s-oedRLvZ3eM5y6Xsgo4t137An61uoPiM9vS");
    std::string     subowner{"1123"};
    std::string     id{};

    std::cout << "in to_db_key_snprintf ";
    key_test(type, subtype, owner, subowner, id, to_db_key_snprintf);

    std::cout << "in to_db_key_append ";
    key_test(type, subtype, owner, subowner, id, to_db_key_append);

    std::cout << "in to_db_key_plus ";
    key_test(type, subtype, owner, subowner, id, to_db_key_plus);

    std::cout << "in to_db_key_stringstream ";
    key_test(type, subtype, owner, subowner, id, to_db_key_stringstream);
}
