#include "tests/xevm_common_test/trie_test_fixture/xtest_trie_fixture.h"

NS_BEG4(top, evm_common, trie, tests)

#define UpdateString(trie, key, value) trie->update(top::to_bytes(std::string{key}), top::to_bytes(std::string{value}))

TEST_F(xtest_trie_fixture, prune_none) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

    // xdbg("begin trie1 update");
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
    // xdbg("end trie1 update");

    // xdbg("begin trie1 commit: to trie_db");
    auto result = trie->commit(ec);
    // xdbg("end trie1 commit: to trie_db");
    ASSERT_TRUE(!ec);

    // xdbg("begin trie1 commit: to disk_db");
    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    // xdbg("end trie1 commit: to disk_db");
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

    // xdbg("begin trie2 update");
    UpdateString(trie2, "doe", "reindeer");
    UpdateString(trie2, "dog", "puppy");
    UpdateString(trie2, "dogglesworth", "cat");

    UpdateString(trie2, "do", "verb");
    UpdateString(trie2, "ether", "wookiedoo");
    UpdateString(trie2, "horse", "stallion");
    UpdateString(trie2, "shaman", "horse");
    UpdateString(trie2, "doge", "coin");
    UpdateString(trie2, "dog", "puppy");
    UpdateString(trie2, "daog1", "pup12dpy1");
    UpdateString(trie2, "dsog2", "pup12epy1");
    UpdateString(trie2, "ado3", "pue21ppy1");
    UpdateString(trie2, "dsog4", "puppqy1");
    UpdateString(trie2, "dog12", "pupd1py1");
    UpdateString(trie2, "dog1242", "pup12epy1");
    UpdateString(trie2, "somethingveryoddindeedthis is", "myothernodedata");
    UpdateString(trie2, "somethisadngveryoddindeedthis is", "myothernodedata");
    // xdbg("end trie2 update");

    // xdbg("begin trie2 commit: to trie_db");
    auto const result2 = trie2->commit(ec);
    // xdbg("end trie2 commit: to trie_db");
    ASSERT_TRUE(!ec);
    ASSERT_EQ(result.first, result2.first);

    // xdbg("begin trie2 commit: to disk_db");
    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
    // xdbg("end trie2 commit: to disk_db");
    ASSERT_TRUE(!ec);

    trie2->prune(result.first, ec);
    ASSERT_TRUE(!ec);
    test_trie_db_ptr->commit_pruned(ec);
    ASSERT_TRUE(!ec);

    ASSERT_EQ(size1, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
}

TEST_F(xtest_trie_fixture, prune_all) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

    // xdbg("begin trie1 update");
    UpdateString(trie, "a1", "reindeer");
    UpdateString(trie, "a2", "puppy");
    UpdateString(trie, "a3", "cat");

    UpdateString(trie, "a4", "verb");
    UpdateString(trie, "a5", "wookiedoo");
    // xdbg("end trie1 update");

    // xdbg("begin trie1 commit: to trie_db");
    auto result = trie->commit(ec);
    // xdbg("end trie1 commit: to trie_db");
    ASSERT_TRUE(!ec);

    // xdbg("begin trie1 commit: to disk_db");
    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    // xdbg("end trie1 commit: to disk_db");
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

    // xdbg("begin trie2 update");
    UpdateString(trie2, "z6", "preindeer");
    UpdateString(trie2, "z7", "ppuppy");
    UpdateString(trie2, "z8", "pcat");

    UpdateString(trie2, "z9", "pverb");
    UpdateString(trie2, "z0", "pwookiedoo");
    // xdbg("end trie2 update");

    // xdbg("begin trie2 commit: to trie_db");
    auto const result2 = trie2->commit(ec);
    // xdbg("end trie2 commit: to trie_db");
    ASSERT_TRUE(!ec);
    ASSERT_NE(result.first, result2.first);

    // xdbg("begin trie2 commit: to disk_db");
    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
    // xdbg("end trie2 commit: to disk_db");
    ASSERT_TRUE(!ec);
    auto const size2 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    trie2->prune(result.first, ec);
    ASSERT_TRUE(!ec);
    test_trie_db_ptr->commit_pruned(ec);
    ASSERT_TRUE(!ec);

    ASSERT_EQ(size2 - size1, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
}

NS_END4
