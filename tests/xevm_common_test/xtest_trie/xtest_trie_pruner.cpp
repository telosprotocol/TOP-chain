#include "tests/xevm_common_test/trie_test_fixture/xtest_trie_fixture.h"

#include <unordered_set>

NS_BEG4(top, evm_common, trie, tests)

#define UpdateString(trie, key, value) trie->update(top::to_bytes(std::string{key}), top::to_bytes(std::string{value}))

//TEST_F(xtest_trie_fixture, prune_none) {
//    std::error_code ec;
//    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie, "doe", "reindeer");
//    UpdateString(trie, "dog", "puppy");
//    UpdateString(trie, "dogglesworth", "cat");
//
//    UpdateString(trie, "do", "verb");
//    UpdateString(trie, "ether", "wookiedoo");
//    UpdateString(trie, "horse", "stallion");
//    UpdateString(trie, "shaman", "horse");
//    UpdateString(trie, "doge", "coin");
//    UpdateString(trie, "dog", "puppy");
//    UpdateString(trie, "daog1", "pup12dpy1");
//    UpdateString(trie, "dsog2", "pup12epy1");
//    UpdateString(trie, "ado3", "pue21ppy1");
//    UpdateString(trie, "dsog4", "puppqy1");
//    UpdateString(trie, "dog12", "pupd1py1");
//    UpdateString(trie, "dog1242", "pup12epy1");
//    UpdateString(trie, "somethingveryoddindeedthis is", "myothernodedata");
//    UpdateString(trie, "somethisadngveryoddindeedthis is", "myothernodedata");
//
//    auto result = trie->commit(ec);
//    ASSERT_TRUE(!ec);
//
//    test_trie_db_ptr->Commit(result.first, nullptr, ec);
//    ASSERT_TRUE(!ec);
//
//    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//
//    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie2, "doe", "reindeer");
//    UpdateString(trie2, "dog", "puppy");
//    UpdateString(trie2, "dogglesworth", "cat");
//
//    UpdateString(trie2, "do", "verb");
//    UpdateString(trie2, "ether", "wookiedoo");
//    UpdateString(trie2, "horse", "stallion");
//    UpdateString(trie2, "shaman", "horse");
//    UpdateString(trie2, "doge", "coin");
//    UpdateString(trie2, "dog", "puppy");
//    UpdateString(trie2, "daog1", "pup12dpy1");
//    UpdateString(trie2, "dsog2", "pup12epy1");
//    UpdateString(trie2, "ado3", "pue21ppy1");
//    UpdateString(trie2, "dsog4", "puppqy1");
//    UpdateString(trie2, "dog12", "pupd1py1");
//    UpdateString(trie2, "dog1242", "pup12epy1");
//    UpdateString(trie2, "somethingveryoddindeedthis is", "myothernodedata");
//    UpdateString(trie2, "somethisadngveryoddindeedthis is", "myothernodedata");
//
//    auto const result2 = trie2->commit(ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_EQ(result.first, result2.first);
//
//    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
//    ASSERT_TRUE(!ec);
//
//    trie2->prune(result.first, ec);
//    ASSERT_TRUE(!ec);
//    trie2->commit_pruned(ec);
//    ASSERT_TRUE(!ec);
//
//    ASSERT_EQ(size1, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
//}

TEST_F(xtest_trie_fixture, prune_none2) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

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

    auto result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

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

    auto const result2 = trie2->commit(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(result.first, result2.first);

    test_trie_db_ptr->Commit(result2.first, nullptr, ec);

    ASSERT_TRUE(!ec);

    std::unordered_set<xh256_t> pruned_hashes;
    trie2->prune(result.first, pruned_hashes, ec);
    ASSERT_TRUE(!ec);
    ASSERT_TRUE(pruned_hashes.empty());
    trie2->commit_pruned(pruned_hashes, ec);
    ASSERT_TRUE(!ec);

    ASSERT_EQ(size1, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
}

//TEST_F(xtest_trie_fixture, prune_all) {
//    std::error_code ec;
//    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie, "a1", "reindeer");
//    UpdateString(trie, "a2", "puppy");
//    UpdateString(trie, "a3", "cat");
//
//    UpdateString(trie, "a4", "verb");
//    UpdateString(trie, "a5", "wookiedoo");
//
//    auto result = trie->commit(ec);
//    ASSERT_TRUE(!ec);
//
//    test_trie_db_ptr->Commit(result.first, nullptr, ec);
//    ASSERT_TRUE(!ec);
//
//    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//
//    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie2, "z6", "preindeer");
//    UpdateString(trie2, "z7", "ppuppy");
//    UpdateString(trie2, "z8", "pcat");
//
//    UpdateString(trie2, "z9", "pverb");
//    UpdateString(trie2, "z0", "pwookiedoo");
//
//    auto const result2 = trie2->commit(ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_NE(result.first, result2.first);
//
//    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
//    
//    ASSERT_TRUE(!ec);
//    auto const size2 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//
//    trie2->prune(result.first, ec);
//    ASSERT_TRUE(!ec);
//    trie2->commit_pruned(ec);
//    ASSERT_TRUE(!ec);
//
//    ASSERT_EQ(size2 - size1, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
//}

TEST_F(xtest_trie_fixture, prune_all2) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

    UpdateString(trie, "a1", "reindeer");
    UpdateString(trie, "a2", "puppy");
    UpdateString(trie, "a3", "cat");

    UpdateString(trie, "a4", "verb");
    UpdateString(trie, "a5", "wookiedoo");

    auto result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

    UpdateString(trie2, "z6", "preindeer");
    UpdateString(trie2, "z7", "ppuppy");
    UpdateString(trie2, "z8", "pcat");

    UpdateString(trie2, "z9", "pverb");
    UpdateString(trie2, "z0", "pwookiedoo");

    auto const result2 = trie2->commit(ec);
    ASSERT_TRUE(!ec);
    ASSERT_NE(result.first, result2.first);

    test_trie_db_ptr->Commit(result2.first, nullptr, ec);

    ASSERT_TRUE(!ec);
    auto const size2 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    std::unordered_set<xh256_t> pruned_hashes;
    trie2->prune(result.first, pruned_hashes, ec);
    ASSERT_TRUE(!ec);
    // ASSERT_EQ(pruned_hashes.size(), size1);
    trie2->commit_pruned(pruned_hashes, ec);
    ASSERT_TRUE(!ec);

    ASSERT_EQ(size2 - size1, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
}

//TEST_F(xtest_trie_fixture, prune_old_root) {
//    // trie root is a full node. thus only root hash is different.
//
//    std::error_code ec;
//    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie, "0a", "0a"); // '0' begins with 3 in hex format
//    UpdateString(trie, "@b", "@b"); // '@' begins with 4 in hex format
//    UpdateString(trie, "Pc", "Pc"); // 'P' begins with 5 in hex format
//    UpdateString(trie, "`d", "`d"); // '`' begins with 6 in hex foramt
//    UpdateString(trie, "pf", "pf"); // 'p' begins with 7 in hex format
//
//    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
//    /*
//     * [
//     *    0: <nil> 1: <nil> 2: <nil> 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil> b: <nil> c: <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
//     * ]
//     */
//
//    auto result = trie->commit(ec);
//    ASSERT_TRUE(!ec);
//
//    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
//    // <ee4c150749d336ed0d47ef98f9fd5977c2276670eeb5159a5949fa089f33505f>
//
//    test_trie_db_ptr->Commit(result.first, nullptr, ec);
//    ASSERT_TRUE(!ec);
//
//    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//
//    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie2, "0a", "0a");    // '0' begins with 3 in hex format
//    UpdateString(trie2, "@b", "@b");    // 'A' begins with 4 in hex format
//    UpdateString(trie2, "Pc", "Pc");    // 'P' begins with 5 in hex format
//    UpdateString(trie2, "`d", "`d");    // '`' begins with 6 in hex foramt
//    UpdateString(trie2, "pf", "pf");    // 'p' begins with 7 in hex format
//    UpdateString(trie2, "'g", "'g");    // ''' begins with 9 in hex format
//
//    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
//    /*
//     * [
//     *    0: <nil> 1: <nil> 2: {07060710: 2767 } 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil> b: <nil> c: <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
//     * ]
//     */
//
//    auto const result2 = trie2->commit(ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_NE(result.first, result2.first);
//
//    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
//    // <a0ccbffedf224b3e88c53c516d394f5d481e003cc76a11e483a1e4692dfd627f>
//
//    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
//
//    ASSERT_TRUE(!ec);
//    auto const size2 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//
//    trie2->prune(result.first, ec);
//    ASSERT_TRUE(!ec);
//    trie2->commit_pruned(ec);
//    ASSERT_TRUE(!ec);
//
//    ASSERT_EQ(size1, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
//    ASSERT_TRUE(size2 > size1);
//    ASSERT_EQ(size2 - size1, 1);
//}

//TEST_F(xtest_trie_fixture, prune_old_root2) {
//    // trie root is a full node. thus only root hash is different.
//
//    std::error_code ec;
//    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie, "0a", "0a");  // '0' begins with 3 in hex format
//    UpdateString(trie, "@b", "@b");  // '@' begins with 4 in hex format
//    UpdateString(trie, "Pc", "Pc");  // 'P' begins with 5 in hex format
//    UpdateString(trie, "`d", "`d");  // '`' begins with 6 in hex foramt
//    UpdateString(trie, "pf", "pf");  // 'p' begins with 7 in hex format
//
//    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
//    /*
//     * [
//     *    0: <nil> 1: <nil> 2: <nil> 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil> b: <nil> c:
//     * <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
//     * ]
//     */
//
//    auto result = trie->commit(ec);
//    ASSERT_TRUE(!ec);
//
//    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
//    // <ee4c150749d336ed0d47ef98f9fd5977c2276670eeb5159a5949fa089f33505f>
//
//    test_trie_db_ptr->Commit(result.first, nullptr, ec);
//    ASSERT_TRUE(!ec);
//
//    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//
//    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie2, "0a", "0a");  // '0' begins with 3 in hex format
//    UpdateString(trie2, "@b", "@b");  // 'A' begins with 4 in hex format
//    UpdateString(trie2, "Pc", "Pc");  // 'P' begins with 5 in hex format
//    UpdateString(trie2, "`d", "`d");  // '`' begins with 6 in hex foramt
//    UpdateString(trie2, "pf", "pf");  // 'p' begins with 7 in hex format
//    UpdateString(trie2, "'g", "'g");  // ''' begins with 9 in hex format
//
//    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
//    /*
//     * [
//     *    0: <nil> 1: <nil> 2: {07060710: 2767 } 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil>
//     * b: <nil> c: <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
//     * ]
//     */
//
//    auto const result2 = trie2->commit(ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_NE(result.first, result2.first);
//
//    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
//    // <a0ccbffedf224b3e88c53c516d394f5d481e003cc76a11e483a1e4692dfd627f>
//
//    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
//
//    ASSERT_TRUE(!ec);
//    auto const size2 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//
//    std::unordered_set<xh256_t> pruned_hashes;
//    trie2->prune(result.first, ec);
//    ASSERT_TRUE(!ec);
//    // ASSERT_EQ(pruned_hashes.size(), size1);
//    trie2->commit_pruned(ec);
//    ASSERT_TRUE(!ec);
//
//    ASSERT_EQ(size1, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
//    ASSERT_TRUE(size2 > size1);
//    ASSERT_EQ(size2 - size1, 1);
//}

//TEST_F(xtest_trie_fixture, prune_empty) {
//    // trie root is a full node. thus only root hash is different.
//
//    std::error_code ec;
//    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//
//    ASSERT_TRUE(!ec);
//
//    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
//    /*
//     * [
//     *    0: <nil> 1: <nil> 2: <nil> 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil> b: <nil> c:
//     * <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
//     * ]
//     */
//
//    auto result = trie->commit(ec);
//    ASSERT_TRUE(!ec);
//
//    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
//    // <ee4c150749d336ed0d47ef98f9fd5977c2276670eeb5159a5949fa089f33505f>
//
//    test_trie_db_ptr->Commit(result.first, nullptr, ec);
//    ASSERT_TRUE(!ec);
//
//    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//    ASSERT_EQ(0, size1);
//
//    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
//    ASSERT_TRUE(!ec);
//
//    UpdateString(trie2, "0a", "0a");  // '0' begins with 3 in hex format
//    UpdateString(trie2, "@b", "@b");  // 'A' begins with 4 in hex format
//    UpdateString(trie2, "Pc", "Pc");  // 'P' begins with 5 in hex format
//    UpdateString(trie2, "`d", "`d");  // '`' begins with 6 in hex foramt
//    UpdateString(trie2, "pf", "pf");  // 'p' begins with 7 in hex format
//    UpdateString(trie2, "'g", "'g");  // ''' begins with 9 in hex format
//
//    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
//    /*
//     * [
//     *    0: <nil> 1: <nil> 2: {07060710: 2767 } 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil>
//     * b: <nil> c: <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
//     * ]
//     */
//
//    auto const result2 = trie2->commit(ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_NE(result.first, result2.first);
//
//    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
//    // <a0ccbffedf224b3e88c53c516d394f5d481e003cc76a11e483a1e4692dfd627f>
//
//    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
//    ASSERT_TRUE(!ec);
//    auto const size2 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
//
//    trie2->prune(result.first, ec);
//    ASSERT_TRUE(!ec);
//    trie2->commit_pruned(ec);
//    ASSERT_TRUE(!ec);
//
//    ASSERT_EQ(size2, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
//}

TEST_F(xtest_trie_fixture, prune_empty2) {
    // trie root is a full node. thus only root hash is different.

    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
    /*
     * [
     *    0: <nil> 1: <nil> 2: <nil> 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil> b: <nil> c:
     * <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
     * ]
     */

    auto result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
    // <ee4c150749d336ed0d47ef98f9fd5977c2276670eeb5159a5949fa089f33505f>

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
    ASSERT_EQ(0, size1);

    auto trie2 = xtrie_t::build_from({}, test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

    UpdateString(trie2, "0a", "0a");  // '0' begins with 3 in hex format
    UpdateString(trie2, "@b", "@b");  // 'A' begins with 4 in hex format
    UpdateString(trie2, "Pc", "Pc");  // 'P' begins with 5 in hex format
    UpdateString(trie2, "`d", "`d");  // '`' begins with 6 in hex foramt
    UpdateString(trie2, "pf", "pf");  // 'p' begins with 7 in hex format
    UpdateString(trie2, "'g", "'g");  // ''' begins with 9 in hex format

    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
    /*
     * [
     *    0: <nil> 1: <nil> 2: {07060710: 2767 } 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil>
     * b: <nil> c: <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
     * ]
     */

    auto const result2 = trie2->commit(ec);
    ASSERT_TRUE(!ec);
    ASSERT_NE(result.first, result2.first);

    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
    // <a0ccbffedf224b3e88c53c516d394f5d481e003cc76a11e483a1e4692dfd627f>

    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
    ASSERT_TRUE(!ec);
    auto const size2 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    std::unordered_set<xh256_t> pruned_hashes;
    trie2->prune(result.first, pruned_hashes, ec);
    ASSERT_TRUE(!ec);
    // ASSERT_EQ(pruned_hashes.size(), 0);
    trie2->commit_pruned(pruned_hashes, ec);
    ASSERT_TRUE(!ec);

    ASSERT_EQ(size2, std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size());
}

TEST_F(xtest_trie_fixture, prune_none_) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

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

    auto const result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    trie->prune(ec);
    ASSERT_TRUE(!ec);

    trie->commit_pruned(std::vector<xh256_t>{xh256_t{}}, ec);
    ASSERT_TRUE(!ec);
}

TEST_F(xtest_trie_fixture, prune_none2_) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

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

    auto const result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    auto const trie2 = xtrie_t::build_from(trie->hash(), test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

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

    auto const result2 = trie2->commit(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(result.first, result2.first);

    test_trie_db_ptr->Commit(result2.first, nullptr, ec);

    ASSERT_TRUE(!ec);

    auto const pruned_size = trie2->pending_pruned_size();
    ASSERT_TRUE(pruned_size == 0);

    trie2->prune(ec);
    ASSERT_TRUE(!ec);

    trie2->commit_pruned(std::vector<xh256_t>{xh256_t{}}, ec);
    ASSERT_TRUE(!ec);
}

TEST_F(xtest_trie_fixture, prune_all_) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

    UpdateString(trie, "a1", "reindeer");
    UpdateString(trie, "a2", "puppy");
    UpdateString(trie, "a3", "cat");

    UpdateString(trie, "a4", "verb");
    UpdateString(trie, "a5", "wookiedoo");

    auto const result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const trie1_root_hash = trie->hash();
    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    auto const trie2 = xtrie_t::build_from(trie1_root_hash, test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

    UpdateString(trie2, "z6", "preindeer");
    UpdateString(trie2, "z7", "ppuppy");
    UpdateString(trie2, "z8", "pcat");

    UpdateString(trie2, "z9", "pverb");
    UpdateString(trie2, "z0", "pwookiedoo");

    auto const result2 = trie2->commit(ec);
    ASSERT_TRUE(!ec);
    ASSERT_NE(result.first, result2.first);

    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    trie2->prune(ec);
    ASSERT_TRUE(!ec);
    trie2->commit_pruned(std::vector<xh256_t>{trie2->hash()}, ec);
    ASSERT_TRUE(!ec);

    auto const trie1_ = xtrie_t::build_from(trie1_root_hash, test_trie_db_ptr, ec);
    ASSERT_TRUE(ec);
    ASSERT_TRUE(trie1_ == nullptr);
}

TEST_F(xtest_trie_fixture, prune_old_root_) {
    // trie root is a full node. thus only root hash is different.

    std::error_code ec;
    auto const trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

    UpdateString(trie, "0a", "0a");  // '0' begins with 3 in hex format
    UpdateString(trie, "@b", "@b");  // '@' begins with 4 in hex format
    UpdateString(trie, "Pc", "Pc");  // 'P' begins with 5 in hex format
    UpdateString(trie, "`d", "`d");  // '`' begins with 6 in hex foramt
    UpdateString(trie, "pf", "pf");  // 'p' begins with 7 in hex format

    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
    /*
     * [
     *    0: <nil> 1: <nil> 2: <nil> 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil> b: <nil> c:
     * <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
     * ]
     */

    auto const result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
    // <ee4c150749d336ed0d47ef98f9fd5977c2276670eeb5159a5949fa089f33505f>

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();

    auto const trie1_root_hash = trie->hash();
    auto const trie2 = xtrie_t::build_from(trie1_root_hash, test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

    UpdateString(trie2, "0a", "0a");  // '0' begins with 3 in hex format
    UpdateString(trie2, "@b", "@b");  // 'A' begins with 4 in hex format
    UpdateString(trie2, "Pc", "Pc");  // 'P' begins with 5 in hex format
    UpdateString(trie2, "`d", "`d");  // '`' begins with 6 in hex foramt
    UpdateString(trie2, "pf", "pf");  // 'p' begins with 7 in hex format
    UpdateString(trie2, "'g", "'g");  // ''' begins with 9 in hex format

    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
    /*
     * [
     *    0: <nil> 1: <nil> 2: {07060710: 2767 } 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil>
     * b: <nil> c: <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
     * ]
     */

    auto const result2 = trie2->commit(ec);
    ASSERT_TRUE(!ec);
    ASSERT_NE(result.first, result2.first);

    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
    // <a0ccbffedf224b3e88c53c516d394f5d481e003cc76a11e483a1e4692dfd627f>

    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const pending_pruned_size = trie2->pending_pruned_size();
    ASSERT_EQ(0, pending_pruned_size);  // old root hash is not in counted.

    trie2->prune(ec);
    ASSERT_TRUE(!ec);

    auto const db_pending_pruned_size = test_trie_db_ptr->pending_pruned_size(trie2->hash());
    ASSERT_EQ(pending_pruned_size + 1, db_pending_pruned_size);

    trie2->commit_pruned(std::vector<xh256_t>{trie2->hash()}, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(0, test_trie_db_ptr->pending_pruned_size(trie2->hash()));
}

TEST_F(xtest_trie_fixture, prune_empty_) {
    // trie root is a full node. thus only root hash is different.

    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    ASSERT_TRUE(!ec);

    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
    /*
     * [
     *    0: <nil> 1: <nil> 2: <nil> 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil> b: <nil> c:
     * <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
     * ]
     */

    auto result = trie->commit(ec);
    ASSERT_TRUE(!ec);

    // std::printf("trie1 resp:\n%s\n", trie->to_string().c_str());
    // <ee4c150749d336ed0d47ef98f9fd5977c2276670eeb5159a5949fa089f33505f>

    test_trie_db_ptr->Commit(result.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const size1 = std::dynamic_pointer_cast<xmock_disk_db>(test_trie_db_ptr->DiskDB())->size();
    ASSERT_EQ(0, size1);

    auto trie2 = xtrie_t::build_from(trie->hash(), test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

    UpdateString(trie2, "0a", "0a");  // '0' begins with 3 in hex format
    UpdateString(trie2, "@b", "@b");  // 'A' begins with 4 in hex format
    UpdateString(trie2, "Pc", "Pc");  // 'P' begins with 5 in hex format
    UpdateString(trie2, "`d", "`d");  // '`' begins with 6 in hex foramt
    UpdateString(trie2, "pf", "pf");  // 'p' begins with 7 in hex format
    UpdateString(trie2, "'g", "'g");  // ''' begins with 9 in hex format

    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
    /*
     * [
     *    0: <nil> 1: <nil> 2: {07060710: 2767 } 3: {00060110: 3061 } 4: {00060210: 4062 } 5: {00060310: 5063 } 6: {00060410: 6064 } 7: {00060610: 7066 } 8: <nil> 9: <nil> a: <nil>
     * b: <nil> c: <nil> d: <nil> e: <nil> f: <nil> [17]: <nil>
     * ]
     */

    auto const result2 = trie2->commit(ec);
    ASSERT_TRUE(!ec);
    ASSERT_NE(result.first, result2.first);

    // std::printf("trie2 resp:\n%s\n", trie2->to_string().c_str());
    // <a0ccbffedf224b3e88c53c516d394f5d481e003cc76a11e483a1e4692dfd627f>

    test_trie_db_ptr->Commit(result2.first, nullptr, ec);
    ASSERT_TRUE(!ec);

    auto const pending_pruned_size = trie2->pending_pruned_size();
    ASSERT_EQ(0, pending_pruned_size);  // old root hash is not in counted.

    trie2->prune(ec);
    ASSERT_EQ(0, trie2->pending_pruned_size());

    ASSERT_EQ(pending_pruned_size + 1, test_trie_db_ptr->pending_pruned_size(trie2->hash()));

    ASSERT_TRUE(!ec);
    trie2->commit_pruned(std::vector<xh256_t>{trie2->hash()}, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(0, test_trie_db_ptr->pending_pruned_size(trie2->hash()));
}

NS_END4
