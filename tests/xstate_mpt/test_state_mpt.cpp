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
    }

    xobject_ptr_t<store::xstore_face_t> m_store{nullptr};
    base::xvdbstore_t * m_db{nullptr};
};

TEST_F(test_state_mpt_fixture, test_db) {
    evm_common::trie::xkv_db_t mpt_db(m_db, TABLE_ADDRESS);

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
    EXPECT_EQ(ec.value(), static_cast<int>(evm_common::error::xerrc_t::trie_db_not_found));

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
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, nullptr, ec);
    EXPECT_FALSE(ec);

    common::xaccount_address_t k1("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoJ");
    common::xaccount_address_t k2("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoK");
    common::xaccount_address_t k3("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoL");
    common::xaccount_address_t k4("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoM");
    common::xaccount_address_t k5("T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoN");
    base::xaccount_index_t index1{1, std::to_string(1), std::to_string(1), 1, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
    base::xaccount_index_t index2{2, std::to_string(2), std::to_string(2), 2, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
    base::xaccount_index_t index3{3, std::to_string(3), std::to_string(3), 3, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
    base::xaccount_index_t index4{4, std::to_string(4), std::to_string(4), 4, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
    base::xaccount_index_t index5{5, std::to_string(5), std::to_string(5), 5, base::enum_xvblock_class_light, base::enum_xvblock_type_general};

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
    xdbg("hash: %s", to_hex(hash).c_str());
    EXPECT_FALSE(ec);
}

TEST_F(test_state_mpt_fixture, test_get_unknown) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, nullptr, ec);
    EXPECT_EQ(ec.value(), 0);

    ec.clear();
    s->get_account_index(common::xaccount_address_t{"T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoM"}, ec);
    EXPECT_EQ(ec.value(), 0);
}

// TODO: should fix cache
TEST_F(test_state_mpt_fixture, test_basic) {
    std::error_code ec;
    auto s = state_mpt::xstate_mpt_t::create(TABLE_ADDRESS, {}, m_db, state_mpt::xstate_mpt_cache_t::instance(), ec);
    EXPECT_EQ(ec.value(), 0);

    auto origin_hash = s->m_trie->Hash();

    std::vector<std::pair<common::xaccount_address_t, base::xaccount_index_t>> data;
    std::set<common::xaccount_address_t> acc_set;
    while (acc_set.size() != 10) {
        data.clear();
        acc_set.clear();
        for (auto i = 0; i < 10; i++) {
            auto acc = common::xaccount_address_t{top::utl::xcrypto_util::make_address_by_random_key(base::enum_vaccount_addr_type_secp256k1_eth_user_account, 0)};
            std::string state_str{"state_str" + std::to_string(i)};
            auto hash = base::xcontext_t::instance().hash(state_str, enum_xhash_type_sha2_256);
            base::xaccount_index_t index{rand(), hash, hash, rand(), base::enum_xvblock_class_light, base::enum_xvblock_type_general};
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
        EXPECT_TRUE(s->m_journal.dirties.count(data[i].first));
        EXPECT_EQ(s->m_journal.index_changes[i].account, data[i].first);
        // not commit in trie
        auto index_bytes = s->m_trie->TryGet(top::to_bytes(data[i].first), ec);
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
    EXPECT_EQ(s->m_state_objects.size(), 5);
    EXPECT_EQ(s->m_state_objects_pending.size(), 5);
    for (auto i = 0; i < 5; i++) {
        EXPECT_TRUE(s->m_state_objects_pending.count(data[i].first));
        // not commit in trie
        auto index_bytes = s->m_trie->TryGet(top::to_bytes(data[i].first), ec);
        EXPECT_FALSE(ec);
        EXPECT_TRUE(index_bytes.empty());
    }
    // update
    auto prev_hash = s->get_root_hash(ec);
    EXPECT_FALSE(ec);
    EXPECT_NE(origin_hash, s->m_trie->Hash());
    EXPECT_EQ(prev_hash, s->m_trie->Hash());
    EXPECT_TRUE(s->m_state_objects_pending.empty());
    for (auto i = 0; i < 5; i++) {
        // commit in db
        auto index_bytes = s->m_trie->TryGet(to_bytes(data[i].first), ec);
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
    EXPECT_TRUE(s->m_journal.index_changes.empty());
    EXPECT_TRUE(s->m_journal.dirties.empty());
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
    EXPECT_EQ(s->m_journal.index_changes.size(), 6);
    EXPECT_EQ(s->m_journal.dirties.size(), 6);
    EXPECT_EQ(s->m_state_objects[data[0].first]->index, new_index0);
    EXPECT_EQ(s->m_state_objects[data[1].first]->index, index1);
    EXPECT_EQ(s->m_journal.index_changes.size(), 6);
    EXPECT_EQ(s->m_journal.index_changes[0].account, data[0].first);
    EXPECT_EQ(s->m_journal.index_changes[0].prev_index, index0);
    for (auto i = 5; i < 10; i++) {
        EXPECT_EQ(s->m_state_objects[data[i].first]->index, data[i].second);
        EXPECT_EQ(s->m_journal.index_changes[i - 4].account, data[i].first);
        EXPECT_EQ(s->m_journal.index_changes[i - 4].prev_index, base::xaccount_index_t());
        EXPECT_TRUE(s->m_journal.dirties.count(data[i].first));
    }
    // error set
    s->set_account_index(data[2].first, new_index0, ec);
    EXPECT_EQ(ec, state_mpt::error::make_error_code(state_mpt::error::xerrc_t::state_mpt_unit_hash_mismatch));
    ec.clear();
    // EXPECT_EQ(s->m_cache_indexes.size(), 10);
    // commit
    s->commit(ec);
    EXPECT_FALSE(ec);
    // cache is not lost
    EXPECT_EQ(s->m_state_objects.size(), 10);
    for (auto obj : s->m_state_objects) {
        EXPECT_FALSE(obj.second->dirty_unit);
    }
    EXPECT_TRUE(s->m_journal.index_changes.empty());
    EXPECT_TRUE(s->m_journal.dirties.empty());
    // check code
    for (auto i = 2; i < 5; i++) {
        std::string state_str{"state_str" + std::to_string(i)};
        auto hash = base::xcontext_t::instance().hash(state_str, enum_xhash_type_sha2_256);
        auto v = ReadUnitWithPrefix(s->m_db->DiskDB(), xhash256_t({hash.begin(), hash.end()}));
        EXPECT_EQ(to_string(v), state_str);
    }

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

    s1->set_account_index(common::xaccount_address_t("T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"), base::xaccount_index_t(), ec);
    auto hash1 = s1->commit(ec);
    EXPECT_EQ(ec.value(), 0);
    hash1;
    // std::cout << "hash1:" << hash1.as_hex_str() << std::endl;

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
    auto sched = state_mpt::new_state_sync(TABLE_ADDRESS, xhash256_t(from_hex(k1, ec)), m_db, false);
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
    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, TABLE_ADDRESS);
    sched->Commit(kv_db);
    EXPECT_EQ(sched->Pending(), 0);
}

TEST_F(test_state_mpt_fixture, test_trie_callback) {
    std::error_code ec;
    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, TABLE_ADDRESS);
    auto trie_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
    auto trie = evm_common::trie::xsecure_trie_t::NewSecure({},trie_db,ec);
    EXPECT_FALSE(ec);

    std::vector<std::string> accounts = {
        "T8000044b9d8bdf16fd0fbc6804e0aabec1f83b88bc7fb",
        "T80000546f7bd4cfc01be68253ec115818c831a6e3fb9b",
        "T800002ffa6445c3b5f27b1e6ac6c7fa89ff2c7123fd46",
        "T80000d9dd3799932c5da9803d91d53a8ccc35d8b523d0",
        "T800001c2400b66cc6c7c5141f13ef41e98af92afdd2aa",
    };

    // table
    auto table_bstate = make_object_ptr<base::xvbstate_t>(TABLE_ADDRESS.value(), 100, 100, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    auto table_unit_state = std::make_shared<data::xtable_bstate_t>(table_bstate.get());
    auto table_snapshot = table_unit_state->take_snapshot();
    auto table_state_hash_str = base::xcontext_t::instance().hash(table_snapshot, enum_xhash_type_sha2_256);
    auto table_unit_hash = utl::xkeccak256_t::digest(TABLE_ADDRESS.value());
    std::string table_unit_hash_str((char *)table_unit_hash.data(), table_unit_hash.size());
    printf("table, account: %s, unit_hash: %s, state_hash: %s, state: %s\n", TABLE_ADDRESS.c_str(), to_hex(table_unit_hash_str).c_str(), to_hex(table_state_hash_str).c_str(), to_hex(table_snapshot).c_str());

    // unit
    for (auto i = 0; i < 5; i++) {
        auto bstate = make_object_ptr<base::xvbstate_t>(accounts[i], i + 1, i + 1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_var(to_string(i), canvas.get());
        auto obj = bstate->load_string_var(to_string(i));
        obj->reset(to_string(i), canvas.get());
        auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
        auto snapshot = unit_state->take_snapshot();
        auto state_hash_str = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
        auto unit_hash = utl::xkeccak256_t::digest(std::to_string(i));
        std::string unit_hash_str((char *)unit_hash.data(), unit_hash.size());
        base::xaccount_index_t index{i + 1, unit_hash_str, state_hash_str, i + 1, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
        std::string index_str;
        index.serialize_to(index_str);
        trie->Update(to_bytes(accounts[i]), to_bytes(index_str));
        printf("unit, account: %s, value: %s, unit_hash: %s, state_hash: %s, state: %s\n", accounts[i].c_str(), to_hex(index_str).c_str(), to_hex(unit_hash_str).c_str(), to_hex(state_hash_str).c_str(), to_hex(snapshot).c_str());
    }
    auto trie_hash = trie->Commit(ec);
    EXPECT_FALSE(ec);
    printf("hash: %s\n", to_hex(trie_hash.first).c_str());

    auto callback = [&](std::vector<xbytes_t> const & path, xbytes_t const & key, xbytes_t const & value, xhash256_t const & req_hash, std::error_code & ec) {
        printf("on account key: %s, value: %s, req: %s\n", to_hex(key).c_str(), to_hex(value).c_str(), req_hash.as_hex_str().c_str());
    };
    auto sched = evm_common::trie::Sync::NewSync(trie_hash.first, kv_db, callback);

    std::vector<xhash256_t> queue;
    auto res = sched->Missing(1);
    auto nodes = std::get<0>(res);
    queue.insert(queue.end(), nodes.begin(), nodes.end());
    while (queue.size() > 0) {
        for (auto q : queue) {
            auto v = trie_db->Node(q, ec);
            EXPECT_FALSE(ec);
            evm_common::trie::SyncResult result;
            result.Hash = xhash256_t(q);
            result.Data = v;
            printf("node hash: %s, node value: %s\n", to_hex(result.Hash).c_str(), to_hex(result.Data).c_str());
            sched->Process(result, ec);
            EXPECT_FALSE(ec);
        }
        sched->Commit(kv_db);
        EXPECT_FALSE(ec);
        auto miss = sched->Missing(1);
        queue.clear();
        auto n = std::get<0>(miss);
        queue.insert(queue.end(), n.begin(), n.end());
    }
}

}  // namespace top