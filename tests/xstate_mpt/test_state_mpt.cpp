// #include "tests/mock/xvchain_creator.hpp"
#include "xcrypto/xcrypto_util.h"
#include "xdbstore/xstore_face.h"
#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_mpt_db.h"
#include "xvledger/xvdbstore.h"
#include "xvledger/xvledger.h"

#define private public
#include "xstate_mpt/xstate_mpt.h"

#include <gtest/gtest.h>

namespace top {

#define TABLE_ADDRESS "Ta0000@0"

class test_state_mpt_fixture : public testing::Test {
public:
    void SetUp() override {
        base::xvchain_t::instance().clean_all(true);
        std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_memdb();
        m_store = store::xstore_factory::create_store_with_static_kvdb(db);
        base::xvchain_t::instance().set_xdbstore(m_store.get());
        m_db = base::xvchain_t::instance().get_xdbstore();
    }
    void TearDown() override {
    }

    xobject_ptr_t<store::xstore_face_t> m_store{nullptr};
    base::xvdbstore_t * m_db{nullptr};
};

TEST_F(test_state_mpt_fixture, test_db) {
    state_mpt::xstate_mpt_db_t mpt_db(m_db, TABLE_ADDRESS);

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
    EXPECT_EQ(ec.value(), static_cast<int>(state_mpt::error::xerrc_t::state_mpt_db_not_found));

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
    EXPECT_EQ(ec.value(), static_cast<int>(state_mpt::error::xerrc_t::state_mpt_db_not_found));

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k2.begin(), k2.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(state_mpt::error::xerrc_t::state_mpt_db_not_found));

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k3.begin(), k3.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(state_mpt::error::xerrc_t::state_mpt_db_not_found));
}

TEST_F(test_state_mpt_fixture, test_example) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create({}, m_db, TABLE_ADDRESS, ec);
    EXPECT_FALSE(ec);

    std::string k1("dog");
    std::string k2("doge");
    std::string k3("cat");
    base::xaccount_index_t index1{1, std::to_string(1), std::to_string(1), 1, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
    base::xaccount_index_t index2{2, std::to_string(2), std::to_string(2), 2, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
    base::xaccount_index_t index3{3, std::to_string(3), std::to_string(3), 3, base::enum_xvblock_class_light, base::enum_xvblock_type_general};

    s->set_account_index(k1, index1, ec);
    EXPECT_FALSE(ec);
    s->set_account_index(k2, index2, ec);
    EXPECT_FALSE(ec);
    s->set_account_index(k3, index3, ec);
    EXPECT_FALSE(ec);
    auto hash = s->commit(ec);
    hash;
    EXPECT_FALSE(ec);
}

TEST_F(test_state_mpt_fixture, test_get_unknown) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create({}, m_db, TABLE_ADDRESS, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    s->get_account_index("unknown", ec);
    EXPECT_EQ(ec.value(), 0);
}

TEST_F(test_state_mpt_fixture, test_basic) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create({}, m_db, TABLE_ADDRESS, ec);
    EXPECT_EQ(ec.value(), 0);

    auto origin_hash = s->m_trie->Hash();

    std::vector<std::pair<std::string, base::xaccount_index_t>> data;
    std::set<std::string> acc_set;
    while (acc_set.size() != 10) {
        data.clear();
        acc_set.clear();
        for (auto i = 0; i < 10; i++) {
            std::string acc = top::utl::xcrypto_util::make_address_by_random_key(base::enum_vaccount_addr_type_secp256k1_eth_user_account);
            base::xaccount_index_t index{rand(), std::to_string(rand()), std::to_string(rand()), rand(), base::enum_xvblock_class_light, base::enum_xvblock_type_general};
            data.emplace_back(std::make_pair(acc, index));
            acc_set.insert(acc);
        }
    }

    for (auto i = 0; i < 5; i++) {
        ec.clear();
        s->set_account_index(data[i].first, data[i].second, ec);
        EXPECT_FALSE(ec);
    }
    EXPECT_EQ(s->m_indexes.size(), 5);
    for (auto i = 0; i < 5; i++) {
        // write in cache
        EXPECT_TRUE(s->m_indexes.count(data[i].first));
        EXPECT_TRUE(s->m_journal.dirties.count(data[i].first));
        EXPECT_EQ(s->m_journal.index_changes[i].account, data[i].first);
        // not commit in trie
        auto index_bytes = s->m_trie->TryGet({data[i].first.begin(), data[i].first.end()}, ec);
        EXPECT_FALSE(ec);
        EXPECT_TRUE(index_bytes.empty());
    }

    // hash not change
    EXPECT_EQ(origin_hash, s->m_trie->Hash());

    // finalize
    s->finalize();
    EXPECT_EQ(origin_hash, s->m_trie->Hash());
    EXPECT_TRUE(s->m_journal.dirties.empty());
    EXPECT_TRUE(s->m_journal.index_changes.empty());
    EXPECT_EQ(s->m_indexes.size(), 5);
    EXPECT_EQ(s->m_pending_indexes.size(), 5);
    for (auto i = 0; i < 5; i++) {
        EXPECT_TRUE(s->m_pending_indexes.count(data[i].first));
        // not commit in trie
        auto index_bytes = s->m_trie->TryGet({data[i].first.begin(), data[i].first.end()}, ec);
        EXPECT_FALSE(ec);
        EXPECT_TRUE(index_bytes.empty());
    }
    // update
    auto prev_hash = s->get_root_hash(ec);
    EXPECT_FALSE(ec);
    EXPECT_NE(origin_hash, s->m_trie->Hash());
    EXPECT_EQ(prev_hash, s->m_trie->Hash());
    EXPECT_TRUE(s->m_pending_indexes.empty());
    for (auto i = 0; i < 5; i++) {
        // commit in db
        auto index_bytes = s->m_trie->TryGet({data[i].first.begin(), data[i].first.end()}, ec);
        EXPECT_FALSE(ec);
        std::string str;
        data[i].second.serialize_to(str);
        xbytes_t str_bytes{str.begin(), str.end()};
        EXPECT_EQ(str_bytes, index_bytes);
    }
    // get
    auto index0_str = s->get_account_index_str(data[0].first, ec);
    EXPECT_FALSE(ec);
    auto index1_str = s->get_account_index_str(data[1].first, ec);
    EXPECT_FALSE(ec);
    EXPECT_EQ(s->m_indexes.size(), 5);
    xbytes_t index0_bytes{index0_str.begin(), index0_str.end()};
    xbytes_t index1_bytes{index1_str.begin(), index1_str.end()};
    EXPECT_EQ(s->m_indexes[data[0].first], index0_bytes);
    EXPECT_EQ(s->m_indexes[data[1].first], index1_bytes);
    EXPECT_TRUE(s->m_journal.index_changes.empty());
    EXPECT_TRUE(s->m_journal.dirties.empty());
    // set
    // auto new_index0 = index0_str;
    base::xaccount_index_t new_index0;
    new_index0.serialize_from({index0_str.begin(), index0_str.end()});
    new_index0.m_latest_unit_height += 1;
    s->set_account_index(data[0].first, new_index0, ec);
    EXPECT_FALSE(ec);
    for (auto i = 5; i < 10; i++) {
        s->set_account_index(data[i].first, data[i].second, ec);
        EXPECT_FALSE(ec);
    }
    EXPECT_EQ(s->m_indexes.size(), 10);
    EXPECT_EQ(s->m_journal.index_changes.size(), 6);
    EXPECT_EQ(s->m_journal.dirties.size(), 6);
    std::string new_index0_str;
    new_index0.serialize_to(new_index0_str);
    xbytes_t new_index0_bytes{new_index0_str.begin(), new_index0_str.end()};
    EXPECT_EQ(s->m_indexes[data[0].first], new_index0_bytes);
    EXPECT_EQ(s->m_indexes[data[1].first], index1_bytes);
    EXPECT_EQ(s->m_journal.index_changes.size(), 6);
    EXPECT_EQ(s->m_journal.index_changes[0].account, data[0].first);
    EXPECT_EQ(s->m_journal.index_changes[0].prev_index, index0_str);
    for (auto i = 5; i < 10; i++) {
        std::string str;
        data[i].second.serialize_to(str);
        xbytes_t b{str.begin(), str.end()};
        EXPECT_EQ(s->m_indexes[data[i].first], b);
        EXPECT_EQ(s->m_journal.index_changes[i - 4].account, data[i].first);
        EXPECT_EQ(s->m_journal.index_changes[i - 4].prev_index, std::string());
        EXPECT_TRUE(s->m_journal.dirties.count(data[i].first));
    }
    // commit
    s->commit(ec);
    EXPECT_FALSE(ec);
    // cache is not lost
    EXPECT_EQ(s->m_indexes.size(), 10);
    EXPECT_TRUE(s->m_journal.index_changes.empty());
    EXPECT_TRUE(s->m_journal.dirties.empty());
}

TEST_F(test_state_mpt_fixture, test_double_commit) {

}

}  // namespace top