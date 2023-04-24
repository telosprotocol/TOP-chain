#include "tests/xevm_common_test/trie_test_fixture/xtest_trie_fixture.h"
#include "xevm_common/trie/xsecure_trie.h"
#include "xutility/xhash.h"

NS_BEG4(top, evm_common, trie, tests)

#define UpdateString(trie, key, value) trie->update(top::to_bytes(std::string{key}), top::to_bytes(std::string{value}));

TEST_F(xtest_trie_fixture, test_secure_trie_delete) {
    std::error_code ec;

    auto strie = xsecure_trie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(strie, "do", "verb");
    UpdateString(strie, "ether", "wookiedoo");
    UpdateString(strie, "horse", "stallion");
    UpdateString(strie, "shaman", "horse");
    UpdateString(strie, "doge", "coin");
    strie->Delete(top::to_bytes(std::string{"ether"}));
    UpdateString(strie, "dog", "puppy");
    strie->Delete(top::to_bytes(std::string{"shaman"}));

    auto exp = xh256_t{top::from_hex("29b235a58c3c25ab83010c327d5932bcf05324b7d6b1185e650798034783ca9d", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, strie->hash());
}

TEST_F(xtest_trie_fixture, test_secure_trie_get) {
    std::error_code ec;

    auto strie = xsecure_trie_t::build_from({}, test_trie_db_ptr, ec);
    UpdateString(strie, "foo", "bar");

    auto key = top::to_bytes(std::string{"foo"});
    auto value = top::to_bytes(std::string{"bar"});

    auto get_result = strie->get(key);
    ASSERT_EQ(get_result, value);

    xbytes_t secKey;
    utl::xkeccak256_t hasher;
    hasher.update(key.data(), key.size());
    hasher.get_hash(secKey);

    // auto getKey_result = strie->get_key(secKey);
    // ASSERT_EQ(getKey_result, key);

    // commit to triedb
    strie->commit(ec);
    ASSERT_TRUE(!ec);

    // after copy strie, sec cache will be rebuilt.
    // auto strie_c = strie->copy();
    // but it still can get from triedb
    // auto getKey_result_c = strie_c->get_key(secKey);
    // ASSERT_EQ(getKey_result_c, key);
}

NS_END4