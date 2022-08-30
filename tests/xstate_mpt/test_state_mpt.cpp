#include "tests/mock/xvchain_creator.hpp"
#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xstate_mpt/xstate_mpt_db.h"
#include "xvledger/xvdbstore.h"

#include <gtest/gtest.h>

namespace top {


class test_state_mpt_fixture : public testing::Test {
public:
    void SetUp() override {
        static mock::xvchain_creator creator(false);
        m_db = base::xvchain_t::instance().get_xdbstore();
    }
    void TearDown() override {
    }

    base::xvdbstore_t * m_db{nullptr};
};

TEST_F(test_state_mpt_fixture, test_db) {
    state_mpt::xstate_mpt_db_t mpt_db(m_db);

    std::string k1{"key1"};
    std::string k2{"key2"};
    std::string k3{"key3"};
    std::string v1{"value1"};
    std::string v2{"value2"};
    std::string v3{"value3"};

    std::error_code ec;
    EXPECT_FALSE(mpt_db.Has({k1.begin(), k1.end()}, ec));
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k1.begin(), k1.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(state_mpt::error::xerrc_t::mpt_not_found));

    ec.clear();
    mpt_db.Put({k1.begin(), k1.end()}, {v1.begin(), v1.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    mpt_db.Put({k2.begin(), k2.end()}, {v2.begin(), v2.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    mpt_db.Put({k3.begin(), k3.end()}, {v3.begin(), v3.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k1.begin(), k1.end()}, ec), top::to_bytes(v1));
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k2.begin(), k2.end()}, ec), top::to_bytes(v2));
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k3.begin(), k3.end()}, ec), top::to_bytes(v3));
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    mpt_db.Delete({k1.begin(), k1.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    mpt_db.Delete({k2.begin(), k2.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    mpt_db.Delete({k3.begin(), k3.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    mpt_db.Delete({k3.begin(), k3.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k1.begin(), k1.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(state_mpt::error::xerrc_t::mpt_not_found));

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k2.begin(), k2.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(state_mpt::error::xerrc_t::mpt_not_found));

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k3.begin(), k3.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(state_mpt::error::xerrc_t::mpt_not_found));
}

}