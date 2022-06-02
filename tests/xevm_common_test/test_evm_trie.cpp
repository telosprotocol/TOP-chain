#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_node.h"
#include "xevm_common/trie/xtrie_node_coding.h"

#include <gtest/gtest.h>

NS_BEG4(top, evm_common, trie, tests)

// impl a memory test db.

class xtest_memory_db : public xtrie_db_face_t {
public:
    xtrie_node_face_ptr_t node(xhash256_t hash) override {
        return m.at(hash);
    }
    void insert(xhash256_t hash, int32_t size, xtrie_node_face_ptr_t node) override {
        m.insert({hash, node});
    }
    std::map<xhash256_t, xtrie_node_face_ptr_t> m;
};
using xtest_memory_db_ptr = std::shared_ptr<xtest_memory_db>;

#define UpdateString(trie, key, value) trie->Update(top::to_bytes(std::string{key}), top::to_bytes(std::string{value}));

TEST(xtrie, test_insert1) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);
    // std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    // trie->Update(top::to_bytes(std::string{"doe"}), top::to_bytes(std::string{"reindeer"}));
    UpdateString(trie, "doe", "reindeer");
    // std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    UpdateString(trie, "dog", "puppy");
    // std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    UpdateString(trie, "dogglesworth", "cat");
    // std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    auto exp = xhash256_t{top::from_hex("8aad789dff2f538bca5d8ea56e8abe10f4c7ba3a5dea95fea4cd6e7c3a1168d3", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->Hash());
}

TEST(xtrie, test_insert2) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);

    UpdateString(trie, "A", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    // std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    auto exp = xhash256_t{top::from_hex("d23786fb4a010da3ce639d66d5e904a11dbc02746d1ce25029e53290cabf28ab", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->Hash());
}

TEST(xtrie, test_insert3) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);

    UpdateString(trie, "A", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    // std::cout << "root hash: " << top::to_hex(trie->Hash().as_array()) << std::endl;

    auto exp = xhash256_t{top::from_hex("d23786fb4a010da3ce639d66d5e904a11dbc02746d1ce25029e53290cabf28ab", ec)};

    auto res = trie->Commit(ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, res.first);
}

TEST(xtrie, test_get) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);

    UpdateString(trie, "doe", "reindeer");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "dogglesworth", "cat");

    auto res = trie->Get(top::to_bytes(std::string{"dog"}));
    ASSERT_EQ(res, top::to_bytes(std::string{"puppy"}));

    auto unknown = trie->Get(top::to_bytes(std::string{"unknown"}));
    ASSERT_EQ(unknown, top::to_bytes(std::string{""}));
}

TEST(xtrie, test_delete) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);

    UpdateString(trie, "do", "verb");
    // std::cout << "root hash: after [verb] " << top::to_hex(trie->Hash().as_array()) << std::endl;
    UpdateString(trie, "ether", "wookiedoo");
    // std::cout << "root hash: after [wookiedoo] " << top::to_hex(trie->Hash().as_array()) << std::endl;
    UpdateString(trie, "horse", "stallion");
    // std::cout << "root hash: after [stallion] " << top::to_hex(trie->Hash().as_array()) << std::endl;
    UpdateString(trie, "shaman", "horse");
    // std::cout << "root hash: after [horse] " << top::to_hex(trie->Hash().as_array()) << std::endl;
    UpdateString(trie, "doge", "coin");
    // std::cout << "root hash: after [coin] " << top::to_hex(trie->Hash().as_array()) << std::endl;
    trie->Delete(top::to_bytes(std::string{"ether"}));
    // std::cout << "root hash: after [ether] " << top::to_hex(trie->Hash().as_array()) << std::endl;
    UpdateString(trie, "dog", "puppy");
    // std::cout << "root hash: after [puppy] " << top::to_hex(trie->Hash().as_array()) << std::endl;
    trie->Delete(top::to_bytes(std::string{"shaman"}));
    // std::cout << "root hash: after [shaman] " << top::to_hex(trie->Hash().as_array()) << std::endl;

    auto exp = xhash256_t{top::from_hex("5991bb8c6514148a29db676a14ac506cd2cd5775ace63c30a4fe457715e9ac84", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->Hash());
}

TEST(xtrie, test_empty_value_as_delete) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);

    UpdateString(trie, "do", "verb");
    UpdateString(trie, "ether", "wookiedoo");
    UpdateString(trie, "horse", "stallion");
    UpdateString(trie, "shaman", "horse");
    UpdateString(trie, "doge", "coin");
    UpdateString(trie, "ether", "");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "shaman", "");

    auto exp = xhash256_t{top::from_hex("5991bb8c6514148a29db676a14ac506cd2cd5775ace63c30a4fe457715e9ac84", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->Hash());
}

// todo
TEST(xtrie, test_replication) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);

    UpdateString(trie, "do", "verb");
    UpdateString(trie, "ether", "wookiedoo");
    UpdateString(trie, "horse", "stallion");
    UpdateString(trie, "shaman", "horse");
    UpdateString(trie, "doge", "coin");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "somethingveryoddindeedthis is", "myothernodedata");

    // need impl Commit
}

TEST(xtrie, test_large_value) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);

    trie->Update(top::to_bytes(std::string{"key1"}), xbytes_t(4, 99));  //{99,99,99,99}
    trie->Update(top::to_bytes(std::string{"key2"}), xbytes_t(32, 1));  //{1,1,1,...,1}

    auto exp = xhash256_t{top::from_hex("afebee6cfce72f9d2a7a4f5926ac11f2a79bd75f3a9ae6358a08252ba5dce3be", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->Hash());
}

TEST(xtrie, test_encoding_trie) {
    std::error_code ec;
    auto xtest_memory_db_ptr = std::make_shared<xtest_memory_db>();
    auto trie = xtrie_t::New({}, xtest_memory_db_ptr, ec);

    UpdateString(trie, "do", "verb");
    UpdateString(trie, "ether", "wookiedoo");
    UpdateString(trie, "horse", "stallion");
    UpdateString(trie, "shaman", "horse");
    UpdateString(trie, "doge", "coin");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "daog1", "pup12dpy1");
    UpdateString(trie, "dsog2", "pup12epy1");
    UpdateString(trie, "ado3", "pue21ppy1");
    UpdateString(trie, "dsog4", "puppqy1");
    UpdateString(trie, "dog12", "pupd1py1");
    UpdateString(trie, "dog1242", "pup12epy1");
    UpdateString(trie, "somethingveryoddindeedthis is", "myothernodedata");
    UpdateString(trie, "somethisadngveryoddindeedthis is", "myothernodedata");

    auto res = trie->Encode();
    xbytes_t exp = top::from_hex(
        "f901ed808080808080f9011280d2870604060f030310897075653231707079318080f8c7808080808080f88880d287060f06070301108970757031326470793180808080808080808080808080f864808080808080"
        "f84e07f84b808080ed83010302e8808080cf8404030210897075703132657079318080808080808080808080808870757064317079318080c882051084636f696e8080808080808080808570757070798080808080"
        "80808080847665726280ed8603060f060703e58080cb108970757031326570793180c9108770757070717931808080808080808080808080808080808080808080d48907040608060507021089776f6f6b6965646f"
        "6f8080d389060f07020703060510887374616c6c696f6e8080808080808080f8c7820306f8c28080808080808080d0890601060d0601060e1085686f727365808080808080f8a08a060d0605070406080609f89380"
        "8080808080f83dac0e06070706060507020709060f060406040609060e06040605060506040704060806090703020006090703108f6d796f746865726e6f646564617461f843b20306010604060e06070706060507"
        "020709060f060406040609060e06040605060506040704060806090703020006090703108f6d796f746865726e6f64656461746180808080808080808080808080808080808080",
        ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, res);
}

NS_END4