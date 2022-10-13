#include "tests/xevm_common_test/trie_test_fixture/xtest_trie_fixture.h"

#include "xevm_common/trie/xtrie_iterator.h"

#include <random>
#include <set>

NS_BEG4(top, evm_common, trie, tests)

#define UPDATE_BYTES(trie, key, value) trie->update(key, value)

TEST_F(xtest_trie_fixture, iterator) {
    std::error_code ec;
    auto const trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

    std::set<xbytes_t> dict;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 999999999);

    for (auto i = 0; i < 100000; ++i) {
        dict.insert(to_bytes(std::to_string(distrib(gen))));
    }

    for (auto const & bytes : dict) {
        UPDATE_BYTES(trie, bytes, bytes);
    }

    auto const result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const & leafs = top::evm_common::trie::xtrie_simple_iterator_t::trie_leafs(result.first, make_observer(test_trie_db_ptr));
    ASSERT_EQ(dict.size(), leafs.size());

    for (auto const & leaf : leafs) {
        ASSERT_TRUE(dict.find(leaf) != std::end(dict));
    }
}

NS_END4
