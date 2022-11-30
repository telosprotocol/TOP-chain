#include "tests/xevm_common_test/trie_test_fixture/xtest_trie_fixture.h"

NS_BEG4(top, evm_common, trie, tests)

#define UpdateString(trie, key, value) trie->update(top::to_bytes(std::string{key}), top::to_bytes(std::string{value}));

TEST_F(xtest_trie_fixture, test_prove_sample) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "doe", "reindeer");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "dogglesworth", "cat");

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

    trie->prove(top::to_bytes(std::string{"doe"}), 0, test_proof_db_ptr, ec);

#define ASSERT_PROOF_DB_HAS(key, value)                                                                                                                                            \
    ASSERT_TRUE(test_proof_db_ptr->get(top::from_hex(std::string{key}, ec), ec) == top::from_hex(std::string{value}, ec));                                                         \
    ASSERT_TRUE(!ec);

    // clang-format off
    ASSERT_PROOF_DB_HAS(
        "5799c74bd63c56055cd9aa306837100e52d3b6acdf4112e79014c07a3f175f60",
        "f851808080808080a0ee6c69ac3a683a5fafd3ead30290d714d403f7dbb5bd899ed6e0c10c4724ad45a0cbc3385c96e7851a6d4264d41593b58a313c652626ab6ca5071947d7247f5a52808080808080808080");
    ASSERT_PROOF_DB_HAS(
        "ee6c69ac3a683a5fafd3ead30290d714d403f7dbb5bd899ed6e0c10c4724ad45",
        "f85f80cf8420646f33897075653231707079318080a0081d4eda7707164e42556204091c578db5fee117096d1d15f7ddc3bc9f3f39fdd085207468657289776f6f6b6965646f6f8080cf85206f727365887374616c6c696f6e8080808080808080");
    ASSERT_PROOF_DB_HAS(
        "081d4eda7707164e42556204091c578db5fee117096d1d15f7ddc3bc9f3f39fd",
        "f851808080808080a028d11f16083e833b868cc9547b460c79bd3b70f8c60a862a997e1b4f1d93fa83a02d86edc5b14964ea706671a92e595e4e0a84ef7398b4026b1bbba673567a75f4808080808080808080");
    ASSERT_PROOF_DB_HAS(
        "28d11f16083e833b868cc9547b460c79bd3b70f8c60a862a997e1b4f1d93fa83",
        "f84080cf84206f67318970757031326470793180808080808080808080808080a0eb27d187bc801e21228e0f1f8809bd558c0ba992f65662f67cc5820e51092dee80");
    ASSERT_PROOF_DB_HAS(
        "eb27d187bc801e21228e0f1f8809bd558c0ba992f65662f67cc5820e51092dee",
        "f5808080808080a0f16e5600645fbc01ba7c0c6cdcfd610b904fa60800647cb13890ee019e0fa4c48080808080808080808476657262");
    ASSERT_PROOF_DB_HAS(
        "f16e5600645fbc01ba7c0c6cdcfd610b904fa60800647cb13890ee019e0fa4c4",
        "f83b8080808080ca20887265696e6465657280a09492a10ce8854d76e3cd842b7e89364de72d5170d2f1807a5952776d2a61cb16808080808080808080");
    // clang-format on
#undef ASSERT_PROOF_DB_HAS
}

TEST_F(xtest_trie_fixture, test_verify_one_element_proof) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "k", "v");

    trie->prove(top::to_bytes(std::string{"k"}), 0, test_proof_db_ptr, ec);

    auto result = VerifyProof(trie->hash(), top::to_bytes(std::string{"k"}), test_proof_db_ptr, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(result, top::to_bytes(std::string{"v"}));
}

NS_END4