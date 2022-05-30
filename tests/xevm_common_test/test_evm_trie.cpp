#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_node.h"

#include <gtest/gtest.h>

NS_BEG4(top, evm_common, trie, tests)

// impl a memory test db.

class xtest_memory_db : public xtrie_db_face_t {
public:
    xtrie_node_face_ptr_t node(xhash256_t hash) override {
        return m.at(hash);
    }
    std::map<xhash256_t, xtrie_node_face_ptr_t> m;
};
using xtest_memory_db_ptr = std::shared_ptr<xtest_memory_db>;

#define UpdateString(trie, key, value) trie->Update(top::to_bytes(std::string{key}), top::to_bytes(std::string{value}));

TEST(xtrie, test_insert) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);
    std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    // trie->Update(top::to_bytes(std::string{"doe"}), top::to_bytes(std::string{"reindeer"}));
    UpdateString(trie, "doe", "reindeer");
    std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    UpdateString(trie, "dog", "puppy");
    std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    UpdateString(trie, "dogglesworth", "cat");
    std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

}

NS_END4