// #include "tests/mock/xvchain_creator.hpp"
#include "xcrypto/xcrypto_util.h"
#include "xdbstore/xstore_face.h"
#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_mpt_db.h"
#include "xvledger/xvdbstore.h"
#include "xvledger/xvledger.h"
#include "xstate_mpt/xstate_sync.h"
#include "xevm_common/trie/xtrie_sync.h"

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
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, nullptr, ec);
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
    xdbg("hash: %s", to_hex(hash).c_str());
    EXPECT_FALSE(ec);
}

TEST_F(test_state_mpt_fixture, test_get_unknown) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, nullptr, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    s->get_account_index("unknown", ec);
    EXPECT_EQ(ec.value(), 0);
}

TEST_F(test_state_mpt_fixture, test_create_twice) {
#if 0 // TODO(jimmy)  need fix build
    std::error_code ec;
    xhash256_t root_hash(random_bytes(32));
    std::cout << root_hash.as_hex_str() << std::endl;

    auto s = state_mpt::xstate_mpt_t::create(root_hash, m_db, TABLE_ADDRESS, ec);
    EXPECT_NE(ec.value(), 0);

    ec.clear();
    auto s1 = state_mpt::xstate_mpt_t::create({}, m_db, TABLE_ADDRESS, ec);
    EXPECT_EQ(ec.value(), 0);
#endif
}

TEST_F(test_state_mpt_fixture, test_basic) {
#if 0// TODO(jimmy) should fix
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, state_mpt::xstate_mpt_cache_t::instance(), ec);
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
    EXPECT_EQ(s->m_cache_indexes.size(), 10);
    // commit
    auto hash = s->commit(ec);
    EXPECT_FALSE(ec);
    // cache is not lost
    EXPECT_EQ(s->m_indexes.size(), 10);
    EXPECT_TRUE(s->m_journal.index_changes.empty());
    EXPECT_TRUE(s->m_journal.dirties.empty());

    EXPECT_EQ(state_mpt::xstate_mpt_cache_t::instance()->m_cache.size(), 1);
    EXPECT_TRUE(state_mpt::xstate_mpt_cache_t::instance()->m_cache.count(TABLE_ADDRESS));
    auto & cache = state_mpt::xstate_mpt_cache_t::instance()->m_cache[TABLE_ADDRESS];

    for (auto i = 1; i < 10; i++) {
        std::string str;
        cache->get(data[i].first + "@" + hash.as_hex_str().substr(0, 8), str);
        std::string str2;
        data[i].second.serialize_to(str2);
        EXPECT_EQ(str, str2);
    }
#endif
}

// TODO: nedd to fix double commit
TEST_F(test_state_mpt_fixture, test_create_twice_commit_twice) {
    std::error_code ec;
    xhash256_t root_hash(random_bytes(32));
    std::cout << root_hash.as_hex_str() << std::endl;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, root_hash, m_db, nullptr, ec);
    EXPECT_NE(ec.value(), 0);

    xhash256_t root_hash1;
    std::cout << root_hash1.as_hex_str() << std::endl;
    ec.clear();
    auto s1 = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, root_hash1, m_db, nullptr, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_NE(s1, nullptr);

    s1->set_account_index("testaddr1", "testindex1", ec);
    auto hash1 = s1->commit(ec);
    EXPECT_EQ(ec.value(), 0);
    std::cout << "hash1:" << hash1.as_hex_str() << std::endl;

    // s1->set_account_index("testaddr2", "testindex2", ec);
    // auto hash2 = s1->commit(ec);
    // EXPECT_EQ(ec.value(), 0);
    // std::cout << "hash2:" << hash2.as_hex_str() << std::endl;
}

TEST_F(test_state_mpt_fixture, test_trie_sync) {
    auto k4 = "6bf0c8abe6bc49f558c591d09cd8639459f93aa70a9da15a0f1a14ee86f63d9c";
    auto v4 = "e5808080808080cb358902003040020132013280808080808080808089010030400101310131";
    auto k3 = "e7f27dd05b413bfa74d15bb326398e109780e284e5e7e30722a69c1bedea5a34";
    auto v3 = "e583006f67a06bf0c8abe6bc49f558c591d09cd8639459f93aa70a9da15a0f1a14ee86f63d9c";
    auto k2 = "e87f9bf2c49634104763d8674fc0dda3d6267d142ea963c76c04351c9c3d32ec";
    auto v2 = "f83f808080ce8320617489030030400301330133a0e7f27dd05b413bfa74d15bb326398e109780e284e5e7e30722a69c1bedea5a34808080808080808080808080";
    auto k1 = "6290c634f8ae9c9ea8dd5e60a563941d0ae70cc5f3bf87d3e101a481a431757e";
    auto v1 = "e216a0e87f9bf2c49634104763d8674fc0dda3d6267d142ea963c76c04351c9c3d32ec";

    std::error_code ec;
    auto sched = state_mpt::new_state_sync(TABLE_ADDRESS, xhash256_t(from_hex(k1, ec)), m_db);
    auto mpt_db = std::make_shared<state_mpt::xstate_mpt_db_t>(m_db, TABLE_ADDRESS);
    EXPECT_FALSE(ec);
    size_t fill = 128;
    // step 1
    {
        auto res = sched->Missing(fill);
        auto nodes = std::get<0>(res);
        auto paths = std::get<1>(res);
        auto units = std::get<2>(res);
        EXPECT_EQ(nodes.size(), 1);
        EXPECT_TRUE(nodes[0]== xhash256_t(from_hex(k1, ec)));
        EXPECT_FALSE(ec);
        EXPECT_EQ(paths.size(), 1);
        EXPECT_TRUE(units.empty());
        evm_common::trie::SyncResult data;
        data.Hash = xhash256_t(from_hex(k1, ec));
        EXPECT_FALSE(ec);
        data.Data = from_hex(v1, ec);
        EXPECT_FALSE(ec);
        sched->Process(data, ec);
        EXPECT_FALSE(ec);
    }
    // step 2
    {
        auto res = sched->Missing(fill);
        auto nodes = std::get<0>(res);
        auto paths = std::get<1>(res);
        auto units = std::get<2>(res);
        EXPECT_EQ(nodes.size(), 1);
        EXPECT_TRUE(nodes[0]== xhash256_t(from_hex(k2, ec)));
        EXPECT_FALSE(ec);
        EXPECT_EQ(paths.size(), 1);
        EXPECT_TRUE(units.empty());
        evm_common::trie::SyncResult data;
        data.Hash = xhash256_t(from_hex(k2, ec));
        EXPECT_FALSE(ec);
        data.Data = from_hex(v2, ec);
        EXPECT_FALSE(ec);
        sched->Process(data, ec);
        EXPECT_FALSE(ec);
    }
    // step 3
    {
        auto res = sched->Missing(fill);
        auto nodes = std::get<0>(res);
        auto paths = std::get<1>(res);
        auto units = std::get<2>(res);
        EXPECT_EQ(nodes.size(), 1);
        EXPECT_TRUE(nodes[0]== xhash256_t(from_hex(k3, ec)));
        EXPECT_FALSE(ec);
        EXPECT_EQ(paths.size(), 1);
        EXPECT_TRUE(units.empty());
        evm_common::trie::SyncResult data;
        data.Hash = xhash256_t(from_hex(k3, ec));
        EXPECT_FALSE(ec);
        data.Data = from_hex(v3, ec);
        EXPECT_FALSE(ec);
        sched->Process(data, ec);
        EXPECT_FALSE(ec);
    }
    // step 4
    {
        auto res = sched->Missing(fill);
        auto nodes = std::get<0>(res);
        auto paths = std::get<1>(res);
        auto units = std::get<2>(res);
        EXPECT_EQ(nodes.size(), 1);
        EXPECT_TRUE(nodes[0]== xhash256_t(from_hex(k4, ec)));
        EXPECT_FALSE(ec);
        EXPECT_EQ(paths.size(), 1);
        EXPECT_TRUE(units.empty());
        evm_common::trie::SyncResult data;
        data.Hash = xhash256_t(from_hex(k4, ec));
        EXPECT_FALSE(ec);
        data.Data = from_hex(v4, ec);
        EXPECT_FALSE(ec);
        sched->Process(data, ec);
    }
    sched->Commit(mpt_db);
    EXPECT_EQ(sched->Pending(), 0);

}

}  // namespace top