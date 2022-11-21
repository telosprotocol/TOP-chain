#include "xcrypto/xcrypto_util.h"
#include "xdbstore/xstore_face.h"
#include "xevm_common/trie/xsecure_trie.h"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xevm_common/trie/xtrie_sync.h"
#include "xevm_common/xerror/xerror.h"
#include "xstate_mpt/xerror.h"
#include "xstate_mpt/xstate_sync.h"
#include "xutility/xhash.h"
#include "xvledger/xvdbstore.h"
#include "xvledger/xvledger.h"
#include "xcrypto/xckey.h"

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include <fstream>

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;

using unordered_json = nlohmann::basic_json<my_workaround_fifo_map>;
using json = unordered_json;

#define private public
#include "xstate_mpt/xstate_mpt.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"

#include <gtest/gtest.h>

namespace top {

#define TABLE_ADDRESS common::xaccount_address_t{"Ta0000@0"}

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
        base::xvchain_t::instance().clean_all(true);
    }

    xobject_ptr_t<store::xstore_face_t> m_store{nullptr};
    base::xvdbstore_t * m_db{nullptr};
};

class test_state_mpt_bench_fixture : public testing::Test {
public:
    void SetUp() override {
        base::xvchain_t::instance().clean_all(true);

        int dst_db_kind = top::db::xdb_kind_kvdb;
        std::vector<db::xdb_path_t> db_data_paths {};
        std::shared_ptr<db::xdb_face_t> db = top::db::xdb_factory_t::create_kvdb("/tmp/mpt_db_test");
        raw_db = db;
        m_store = top::store::xstore_factory::create_store_with_static_kvdb(db);
        base::xvchain_t::instance().set_xdbstore(m_store.get());
        m_db = base::xvchain_t::instance().get_xdbstore();
    }
    void TearDown() override {
        base::xvchain_t::instance().clean_all(true);
    }

    xobject_ptr_t<store::xstore_face_t> m_store{nullptr};
    std::shared_ptr<db::xdb_face_t> raw_db{nullptr};
    base::xvdbstore_t * m_db{nullptr};
};

TEST_F(test_state_mpt_fixture, test_db) {
    evm_common::trie::xkv_db_t mpt_db(m_db, TABLE_ADDRESS);

    xbytes_t k1{'k', 'e', 'y', '1'};
    xbytes_t k2{'k', 'e', 'y', '2'};
    xbytes_t k3{'k', 'e', 'y', '3'};
    std::string v1{"value1"};
    std::string v2{"value2"};
    std::string v3{"value3"};

    std::error_code ec;
    EXPECT_FALSE(mpt_db.Has({k1.begin(), k1.end()}, ec));
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k1.begin(), k1.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(evm_common::error::xerrc_t::trie_db_not_found));

    ec.clear();
    mpt_db.Put(k1, {v1.begin(), v1.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    mpt_db.Put(k2, {v2.begin(), v2.end()}, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    mpt_db.Put(k3, {v3.begin(), v3.end()}, ec);
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
    EXPECT_EQ(ec.value(), static_cast<int>(evm_common::error::xerrc_t::trie_db_not_found));

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k2.begin(), k2.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(evm_common::error::xerrc_t::trie_db_not_found));

    ec.clear();
    EXPECT_EQ(mpt_db.Get({k3.begin(), k3.end()}, ec), xbytes_t{});
    EXPECT_EQ(ec.value(), static_cast<int>(evm_common::error::xerrc_t::trie_db_not_found));
}

TEST_F(test_state_mpt_fixture, test_example) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, ec);
    EXPECT_FALSE(ec);

    common::xaccount_address_t k1("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoJ");
    common::xaccount_address_t k2("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoK");
    common::xaccount_address_t k3("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoL");
    common::xaccount_address_t k4("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoM");
    common::xaccount_address_t k5("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoN");
    base::xaccount_index_t index1{1, std::to_string(1), std::to_string(1), 1};
    base::xaccount_index_t index2{2, std::to_string(2), std::to_string(2), 2};
    base::xaccount_index_t index3{3, std::to_string(3), std::to_string(3), 3};
    base::xaccount_index_t index4{4, std::to_string(4), std::to_string(4), 4};
    base::xaccount_index_t index5{5, std::to_string(5), std::to_string(5), 5};

    s->set_account_index(k1, index1, ec);
    EXPECT_FALSE(ec);
    s->set_account_index(k2, index2, ec);
    EXPECT_FALSE(ec);
    s->set_account_index(k3, index3, ec);
    EXPECT_FALSE(ec);
    s->set_account_index(k4, index3, ec);
    EXPECT_FALSE(ec);
    s->set_account_index(k5, index3, ec);
    EXPECT_FALSE(ec);
    auto hash = s->commit(ec);
    hash;
    xdbg("hash: %s", to_hex(hash).c_str());
    EXPECT_FALSE(ec);
}

TEST_F(test_state_mpt_fixture, test_get_unknown) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    s->get_account_index(common::xaccount_address_t{"T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoM"}, ec);
    EXPECT_EQ(ec.value(), 0);
}

// TODO: should fix cache
TEST_F(test_state_mpt_fixture, test_basic) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, ec);
    EXPECT_EQ(ec.value(), 0);

    auto origin_hash = s->m_trie->hash();

    std::vector<std::pair<common::xaccount_address_t, base::xaccount_index_t>> data;
    std::set<common::xaccount_address_t> acc_set;
    while (acc_set.size() != 10) {
        data.clear();
        acc_set.clear();
        for (auto i = 0; i < 10; i++) {
            auto acc = common::xaccount_address_t{top::utl::xcrypto_util::make_address_by_random_key(base::enum_vaccount_addr_type_secp256k1_eth_user_account, 0)};
            std::string state_str{"state_str" + std::to_string(i)};
            auto hash = base::xcontext_t::instance().hash(state_str, enum_xhash_type_sha2_256);
            base::xaccount_index_t index{rand(), hash, hash, rand()};
            data.emplace_back(std::make_pair(acc, index));
            acc_set.insert(acc);
        }
    }

    for (auto i = 0; i < 2; i++) {
        ec.clear();
        s->set_account_index(data[i].first, data[i].second, ec);
        EXPECT_FALSE(ec);
    }
    for (auto i = 2; i < 5; i++) {
        ec.clear();
        std::string state_str{"state_str" + std::to_string(i)};
        s->set_account_index_with_unit(data[i].first, data[i].second, {state_str.begin(), state_str.end()}, ec);
        EXPECT_FALSE(ec);
    }
    EXPECT_EQ(s->m_state_objects.size(), 5);
    for (auto i = 0; i < 5; i++) {
        // write in cache
        EXPECT_TRUE(s->m_state_objects.count(data[i].first));
        if (i >= 2) {
            EXPECT_TRUE(s->m_state_objects[data[i].first]->dirty_unit);
        }
        // not commit in trie
        auto index_bytes = s->m_trie->try_get(top::to_bytes(data[i].first), ec);
        EXPECT_FALSE(ec);
        EXPECT_TRUE(index_bytes.empty());
    }

    // hash not change
    EXPECT_EQ(origin_hash, s->m_trie->hash());

    EXPECT_EQ(origin_hash, s->m_trie->hash());
    EXPECT_EQ(s->m_state_objects.size(), 5);
    EXPECT_EQ(s->m_state_objects_pending.size(), 5);
    for (auto i = 0; i < 5; i++) {
        EXPECT_TRUE(s->m_state_objects_pending.count(data[i].first));
        // not commit in trie
        auto index_bytes = s->m_trie->try_get(top::to_bytes(data[i].first), ec);
        EXPECT_FALSE(ec);
        EXPECT_TRUE(index_bytes.empty());
    }
    // update
    auto prev_hash = s->get_root_hash(ec);
    EXPECT_FALSE(ec);
    EXPECT_NE(origin_hash, s->m_trie->hash());
    EXPECT_EQ(prev_hash, s->m_trie->hash());
    EXPECT_TRUE(s->m_state_objects_pending.empty());
    for (auto i = 0; i < 5; i++) {
        // commit in db
        auto index_bytes = s->m_trie->try_get(to_bytes(data[i].first), ec);
        EXPECT_FALSE(ec);
        state_mpt::xaccount_info_t info;
        info.m_account = data[i].first;
        info.m_index = data[i].second;
        auto str = info.encode();
        xbytes_t str_bytes{str.begin(), str.end()};
        EXPECT_EQ(str_bytes, index_bytes);
    }
    // get
    auto index0 = s->get_account_index(data[0].first, ec);
    EXPECT_FALSE(ec);
    auto index1 = s->get_account_index(data[1].first, ec);
    EXPECT_FALSE(ec);
    EXPECT_EQ(s->m_state_objects.size(), 5);
    EXPECT_EQ(s->m_state_objects[data[0].first]->index, index0);
    EXPECT_EQ(s->m_state_objects[data[1].first]->index, index1);
    // set
    // auto new_index0 = index0_str;
    base::xaccount_index_t new_index0 = index0;
    new_index0.m_latest_unit_height += 1;
    s->set_account_index(data[0].first, new_index0, ec);
    EXPECT_FALSE(ec);
    for (auto i = 5; i < 10; i++) {
        s->set_account_index(data[i].first, data[i].second, ec);
        EXPECT_FALSE(ec);
    }
    EXPECT_EQ(s->m_state_objects.size(), 10);
    EXPECT_EQ(s->m_state_objects[data[0].first]->index, new_index0);
    EXPECT_EQ(s->m_state_objects[data[1].first]->index, index1);
    // error set
    s->set_account_index(data[2].first, new_index0, ec);
    EXPECT_EQ(ec, state_mpt::error::make_error_code(state_mpt::error::xerrc_t::state_mpt_unit_hash_mismatch));
    ec.clear();
    // EXPECT_EQ(s->m_cache_indexes.size(), 10);
    // commit
    s->commit(ec);
    EXPECT_FALSE(ec);
    // cache is lost
    EXPECT_EQ(s->m_state_objects.size(), 0);
    for (auto obj : s->m_state_objects) {
        EXPECT_FALSE(obj.second->dirty_unit);
    }
    // // check code
    // for (auto i = 2; i < 5; i++) {
    //     std::string state_str{"state_str" + std::to_string(i)};
    //     auto hash = base::xcontext_t::instance().hash(state_str, enum_xhash_type_sha2_256);
    //     xbytes_t b{hash.begin(), hash.end()};
    //     auto state_key = base::xvdbkey_t::create_prunable_unit_state_key(
    //             base::xvaccount_t{info.m_account.value()}, info.m_index.get_latest_unit_height(), info.m_index.get_latest_unit_hash());
    //     auto v = ReadUnitWithPrefix(s->m_db->DiskDB(), b);
    //     EXPECT_EQ(to_string(v), state_str);
    // }

    // EXPECT_EQ(state_mpt::xstate_mpt_cache_t::instance()->m_cache.size(), 1);
    // EXPECT_TRUE(state_mpt::xstate_mpt_cache_t::instance()->m_cache.count(TABLE_ADDRESS));
    // auto & cache = state_mpt::xstate_mpt_cache_t::instance()->m_cache[TABLE_ADDRESS];

    // for (auto i = 1; i < 10; i++) {
    //     std::string str;
    //     cache->get(data[i].first + "@" + hash.as_hex_str().substr(0, 8), str);
    //     std::string str2;
    //     data[i].second.serialize_to(str2);
    //     EXPECT_EQ(str, str2);
    // }
}

// TODO: nedd to fix double commit
TEST_F(test_state_mpt_fixture, test_create_twice_commit_twice) {
    std::error_code ec;
    evm_common::xh256_t root_hash(random_bytes(32));
    std::cout << root_hash.hex() << std::endl;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, root_hash, m_db, ec);
    EXPECT_NE(ec.value(), 0);

    evm_common::xh256_t root_hash1;
    std::cout << root_hash1.hex() << std::endl;
    ec.clear();
    auto s1 = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, root_hash1, m_db, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_NE(s1, nullptr);

    s1->set_account_index(common::xaccount_address_t("T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"), base::xaccount_index_t(), ec);
    std::cout << "first commit" << std::endl;
    auto hash1 = s1->commit(ec);
    EXPECT_EQ(ec.value(), 0);
    // hash1;
    std::cout << "hash1:" << hash1.hex() << std::endl;

    // s1->set_account_index(common::xaccount_address_t("T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"), base::xaccount_index_t(), ec);
    std::cout << "second commit" << std::endl;
    auto hash2 = s1->commit(ec);
    EXPECT_EQ(ec.value(), 0);
    std::cout << "hash2:" << hash2.hex() << std::endl;
    ASSERT_EQ(hash1, hash2);
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
    auto sched = state_mpt::new_state_sync(TABLE_ADDRESS, evm_common::xh256_t(from_hex(k1, ec)), m_db, false);
    EXPECT_FALSE(ec);
    size_t fill = 128;
    // step 1
    {
        auto res = sched->Missing(fill);
        auto nodes = std::get<0>(res);
        auto paths = std::get<1>(res);
        auto units = std::get<2>(res);
        EXPECT_EQ(nodes.size(), 1);
        EXPECT_TRUE(nodes[0]== evm_common::xh256_t(from_hex(k1, ec)));
        EXPECT_FALSE(ec);
        EXPECT_TRUE(units.empty());
        evm_common::trie::SyncResult data;
        data.Hash = evm_common::xh256_t(from_hex(k1, ec));
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
        EXPECT_TRUE(nodes[0] == evm_common::xh256_t(from_hex(k2, ec)));
        EXPECT_FALSE(ec);
        EXPECT_TRUE(units.empty());
        evm_common::trie::SyncResult data;
        data.Hash = evm_common::xh256_t(from_hex(k2, ec));
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
        EXPECT_TRUE(nodes[0] == evm_common::xh256_t(from_hex(k3, ec)));
        EXPECT_FALSE(ec);
        EXPECT_TRUE(units.empty());
        evm_common::trie::SyncResult data;
        data.Hash = evm_common::xh256_t(from_hex(k3, ec));
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
        EXPECT_TRUE(nodes[0] == evm_common::xh256_t(from_hex(k4, ec)));
        EXPECT_FALSE(ec);
        EXPECT_TRUE(units.empty());
        evm_common::trie::SyncResult data;
        data.Hash = evm_common::xh256_t(from_hex(k4, ec));
        EXPECT_FALSE(ec);
        data.Data = from_hex(v4, ec);
        EXPECT_FALSE(ec);
        sched->Process(data, ec);
    }
    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, TABLE_ADDRESS);
    sched->Commit(kv_db);
    EXPECT_EQ(sched->Pending(), 0);
}

TEST_F(test_state_mpt_fixture, test_trie_callback) {
    std::error_code ec;
    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, TABLE_ADDRESS);
    auto trie_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
    auto trie = evm_common::trie::xsecure_trie_t::build_from({},trie_db,ec);
    EXPECT_FALSE(ec);

    std::vector<std::string> accounts = {
        "T8000044b9d8bdf16fd0fbc6804e0aabec1f83b88bc7fb",
        "T80000546f7bd4cfc01be68253ec115818c831a6e3fb9b",
        "T800002ffa6445c3b5f27b1e6ac6c7fa89ff2c7123fd46",
        "T80000d9dd3799932c5da9803d91d53a8ccc35d8b523d0",
        "T800001c2400b66cc6c7c5141f13ef41e98af92afdd2aa",
    };

    // table
    auto table_bstate = make_object_ptr<base::xvbstate_t>(TABLE_ADDRESS.to_string(), 100, 100, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    std::string table_state_str;
    table_bstate->serialize_to_string(table_state_str);
    auto table_state = std::make_shared<data::xtable_bstate_t>(table_bstate.get());
    auto table_snapshot = table_state->take_snapshot();
    auto table_state_hash_str = base::xcontext_t::instance().hash(table_snapshot, enum_xhash_type_sha2_256);
    auto table_block_hash = utl::xkeccak256_t::digest(TABLE_ADDRESS.to_string());
    std::string table_block_hash_str((char *)table_block_hash.data(), table_block_hash.size());
    printf("table, account: %s, block_hash: %s, state_hash: %s, state: %s\n",
           TABLE_ADDRESS.to_string().c_str(),
           to_hex(table_block_hash_str).c_str(),
           to_hex(table_state_hash_str).c_str(),
           to_hex(table_state_str).c_str());
    // unit
    for (auto i = 0; i < 5; i++) {
        auto bstate = make_object_ptr<base::xvbstate_t>(accounts[i], i + 1, i + 1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_var(to_string(i), canvas.get());
        auto obj = bstate->load_string_var(to_string(i));
        obj->reset(to_string(i), canvas.get());
        std::string unit_state_str;
        bstate->serialize_to_string(unit_state_str);
        auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
        auto snapshot = unit_state->take_snapshot();
        auto unit_state_hash_str = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
        auto unit_block_hash = utl::xkeccak256_t::digest(std::to_string(i));
        std::string unit_block_hash_str((char *)unit_block_hash.data(), unit_block_hash.size());
        base::xaccount_index_t index{i + 1, unit_block_hash_str, unit_state_hash_str, i + 1};
        state_mpt::xaccount_info_t info;
        info.m_account = common::xaccount_address_t(accounts[i]);
        info.m_index = index;
        auto info_str = info.encode();
        trie->update(to_bytes(accounts[i]), to_bytes(info_str));
        printf("unit, account: %s, value: %s, block_hash: %s, state_hash: %s, state: %s\n",
               accounts[i].c_str(),
               to_hex(info_str).c_str(),
               to_hex(unit_block_hash_str).c_str(),
               to_hex(unit_state_hash_str).c_str(),
               to_hex(unit_state_str).c_str());
    }
    auto trie_hash = trie->commit(ec);
    EXPECT_FALSE(ec);
    printf("hash: %s\n", to_hex(trie_hash.first).c_str());

    auto callback = [&](std::vector<xbytes_t> const & path, xbytes_t const & key, xbytes_t const & value, evm_common::xh256_t const & req_hash, std::error_code & ec) {
        printf("on account key: %s, value: %s, req: %s\n", to_hex(key).c_str(), to_hex(value).c_str(), req_hash.hex().c_str());
    };
    auto sched = evm_common::trie::Sync::NewSync(kv_db);
    sched->Init(trie_hash.first, callback);

    std::vector<evm_common::xh256_t> queue;
    auto res = sched->Missing(1);
    auto nodes = std::get<0>(res);
    queue.insert(queue.end(), nodes.begin(), nodes.end());
    while (queue.size() > 0) {
        for (auto q : queue) {
            auto v = trie_db->Node(q, ec);
            EXPECT_FALSE(ec);
            evm_common::trie::SyncResult result;
            result.Hash = evm_common::xh256_t(q);
            result.Data = v;
            printf("node hash: %s, node value: %s\n", to_hex(result.Hash).c_str(), to_hex(result.Data).c_str());
            sched->Process(result, ec);
            EXPECT_FALSE(ec);
        }
        EXPECT_FALSE(ec);
        auto miss = sched->Missing(1);
        queue.clear();
        auto n = std::get<0>(miss);
        queue.insert(queue.end(), n.begin(), n.end());
    }
    sched->Commit(kv_db);
}

std::map<evm_common::xh256_t, xbytes_t> create_node_hash_data(size_t count) {
    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::base::enum_vaccount_addr_type account_address_type{top::base::enum_vaccount_addr_type_secp256k1_user_account};
    top::base::enum_xchain_zone_index zone_index{top::base::enum_chain_zone_consensus_index};

    std::map<evm_common::xh256_t, xbytes_t> data;
    uint16_t ledger_id = top::base::xvaccount_t::make_ledger_id(static_cast<top::base::enum_xchain_id>(network_id.value()), zone_index);
    for (size_t i = 0; i < count; i++) {
        top::utl::xecprikey_t private_key;
        auto public_key = private_key.get_public_key();
        std::string account_address = private_key.to_account_address(account_address_type, ledger_id);
        state_mpt::xaccount_info_t info;
        info.m_account = common::xaccount_address_t{account_address};
        std::string state_str{"state_str" + std::to_string(i)};
        auto hash = base::xcontext_t::instance().hash(state_str, enum_xhash_type_sha2_256);
        base::xaccount_index_t index{rand(), hash, hash, rand()};
        info.m_index = index;
        auto str = info.encode();
        auto hashvalue = utl::xkeccak256_t::digest(std::to_string(i));
        evm_common::xh256_t key{to_bytes(hashvalue)};
        data[key] = {str.begin(), str.end()};
    }
    return data;
}

std::map<xbytes_t, xbytes_t> create_node_bytes_data(size_t count) {
    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::base::enum_vaccount_addr_type account_address_type{top::base::enum_vaccount_addr_type_secp256k1_user_account};
    top::base::enum_xchain_zone_index zone_index{top::base::enum_chain_zone_consensus_index};

    std::map<xbytes_t, xbytes_t> data;
    uint16_t ledger_id = top::base::xvaccount_t::make_ledger_id(static_cast<top::base::enum_xchain_id>(network_id.value()), zone_index);
    for (size_t i = 0; i < count; i++) {
        top::utl::xecprikey_t private_key;
        auto public_key = private_key.get_public_key();
        std::string account_address = private_key.to_account_address(account_address_type, ledger_id);
        state_mpt::xaccount_info_t info;
        info.m_account = common::xaccount_address_t{account_address};
        std::string state_str{"state_str" + std::to_string(i)};
        auto hash = base::xcontext_t::instance().hash(state_str, enum_xhash_type_sha2_256);
        base::xaccount_index_t index{rand(), hash, hash, rand()};
        info.m_index = index;
        auto str = info.encode();
        auto hashvalue = utl::xkeccak256_t::digest(std::to_string(i));
        // xhash256_t key{to_bytes(hashvalue)};
        data[to_bytes(hashvalue)] = {str.begin(), str.end()};
    }
    return data;
}

std::map<common::xaccount_address_t, std::pair<base::xaccount_index_t, std::string>> create_mpt_data(size_t count) {
    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::base::enum_vaccount_addr_type account_address_type{top::base::enum_vaccount_addr_type_secp256k1_eth_user_account};
    top::base::enum_xchain_zone_index zone_index{top::base::enum_chain_zone_consensus_index};

    std::map<common::xaccount_address_t, std::pair<base::xaccount_index_t, std::string>> data;
    uint16_t ledger_id = top::base::xvaccount_t::make_ledger_id(static_cast<top::base::enum_xchain_id>(network_id.value()), zone_index);
    for (size_t i = 0; i < count; i++) {
        top::utl::xecprikey_t private_key;
        auto public_key = private_key.get_public_key();
        std::string account_address = private_key.to_account_address(account_address_type, ledger_id);
        std::string state_str{"state_str" + std::to_string(i)};
        auto hash = base::xcontext_t::instance().hash(state_str, enum_xhash_type_sha2_256);
        base::xaccount_index_t index{rand(), hash, hash, rand()};
        auto bstate = make_object_ptr<base::xvbstate_t>(account_address, i + 1, i + 1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_var(to_string(i), canvas.get());
        auto obj = bstate->load_string_var(to_string(i));
        obj->reset(to_string(i), canvas.get());
        std::string unit_state_str;
        bstate->serialize_to_string(unit_state_str);
        auto p = std::make_pair(index, unit_state_str);
        data.emplace(std::make_pair(common::xaccount_address_t{account_address}, p));
    }
    return data;
}

void generate_state_mpt_data_file() {
    auto data = create_mpt_data(1000000);
    json j;
    int i{0};
    for (auto & d : data) {
        std::string str;
        d.second.first.serialize_to(str);
        j[d.first.to_string()]["index"] = to_hex(str);
        j[d.first.to_string()]["state"] = to_hex(d.second.second);
    }
    std::ofstream f("state_mpt_data.json");
    f << std::setw(4) << j;
}

TEST_F(test_state_mpt_bench_fixture, test_cache_node_key_BENCH) {
    auto data = create_node_hash_data(1000000);

    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, TABLE_ADDRESS);
    auto t1 = base::xtime_utl::time_now_ms();
    for (auto & d : data) {
        evm_common::trie::WriteTrieNode(kv_db, d.first, d.second);
    }
    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time: " << t2 - t1 << " ms" << std::endl;
}

TEST_F(test_state_mpt_bench_fixture, test_batch_node_BENCH) {
    std::vector<std::map<xbytes_t, xbytes_t>> data;
    for (auto i = 0; i < 1000000 / 1000; i++) {
        data.emplace_back(create_node_bytes_data(1000));
    }

    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, TABLE_ADDRESS);
    auto t1 = base::xtime_utl::time_now_ms();
    for (auto & d : data) {
        evm_common::trie::WriteTrieNodeBatch(kv_db, d);
    }
    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time: " << t2 - t1 << " ms" << std::endl;
}

TEST_F(test_state_mpt_bench_fixture, test_state_mpt_BENCH) {

    std::ifstream stream("state_mpt_data.json");
    if (stream.bad()) {
        return;
    }
    json j;
    stream >> j;

    std::error_code ec;
    std::map<common::xaccount_address_t, std::pair<base::xaccount_index_t, xbytes_t>> data;

    for (auto it = j.begin(); it != j.end(); it++) {
        auto addr = it.key();
        auto index_str = from_hex(it->at("index").get<std::string>(), ec);
        auto state_str = from_hex(it->at("state").get<std::string>(), ec);
        EXPECT_FALSE(ec);
        base::xaccount_index_t index;
        index.serialize_from({index_str.begin(), index_str.end()});
        auto p = std::make_pair(index, xbytes_t{state_str.begin(), state_str.end()});
        data.emplace(std::make_pair(common::xaccount_address_t{addr}, p));

    }
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, ec);
    EXPECT_FALSE(ec);

    auto t1 = base::xtime_utl::time_now_ms();
    for (auto & d : data) {
        s->set_account_index_with_unit(d.first, d.second.first, d.second.second, ec);
    }
    auto t2 = base::xtime_utl::time_now_ms();
    s->commit(ec);
    auto t3 = base::xtime_utl::time_now_ms();
    EXPECT_FALSE(ec);
    std::cout << "total num: " << data.size() << ", update time: " << t2 - t1 << " ms" << ", commit time: " << t3 - t2 << std::endl;
}

TEST_F(test_state_mpt_bench_fixture, test_state_mpt_not_commit_memory_leak_BENCH) {
    {
        auto data = create_mpt_data(1000000);
        std::error_code ec;
        auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, ec);
        EXPECT_FALSE(ec);

        for (auto const & d : data) {
            s->set_account_index_with_unit(d.first, d.second.first, to_bytes(d.second.second), ec);
        }
    }
    m_db->close();
}

TEST_F(test_state_mpt_bench_fixture, test_state_mpt_commit_memory_leak_BENCH) {
    {
        auto data = create_mpt_data(1000000);
        std::error_code ec;
        auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, ec);
        EXPECT_FALSE(ec);

        for (auto const & d : data) {
            s->set_account_index_with_unit(d.first, d.second.first, to_bytes(d.second.second), ec);
        }
        s->commit(ec);
        EXPECT_FALSE(ec);
    }

    m_db->close();
    while (!m_db->is_close());
    raw_db->close();
}

}  // namespace top