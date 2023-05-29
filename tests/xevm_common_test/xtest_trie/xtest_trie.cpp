#include "tests/xevm_common_test/trie_test_fixture/xtest_trie_fixture.h"

NS_BEG4(top, evm_common, trie, tests)

#define UpdateString(trie, key, value) trie->update(top::to_bytes(std::string{key}), top::to_bytes(std::string{value}));

#define TESTINTRIE(trie, key, value) ASSERT_EQ(trie->get(top::to_bytes(std::string{key})), top::to_bytes(std::string{value}));

TEST_F(xtest_trie_fixture, test_insert1) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "doe", "reindeer");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "dogglesworth", "cat");

    auto exp = xh256_t{top::from_hex("8aad789dff2f538bca5d8ea56e8abe10f4c7ba3a5dea95fea4cd6e7c3a1168d3", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->hash());
}

TEST_F(xtest_trie_fixture, test_insert2) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "A", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

    auto exp = xh256_t{top::from_hex("d23786fb4a010da3ce639d66d5e904a11dbc02746d1ce25029e53290cabf28ab", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->hash());
}

TEST_F(xtest_trie_fixture, test_insert3) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "A", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

    auto exp = xh256_t{top::from_hex("d23786fb4a010da3ce639d66d5e904a11dbc02746d1ce25029e53290cabf28ab", ec)};

    auto res = trie->commit(ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, res.first);
}

TEST_F(xtest_trie_fixture, test_get) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "doe", "reindeer");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "dogglesworth", "cat");

    auto res = trie->get(top::to_bytes(std::string{"dog"}));
    ASSERT_EQ(res, top::to_bytes(std::string{"puppy"}));

    auto unknown = trie->get(top::to_bytes(std::string{"unknown"}));
    ASSERT_EQ(unknown, top::to_bytes(std::string{""}));
}

TEST_F(xtest_trie_fixture, test_delete) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "do", "verb");
    UpdateString(trie, "ether", "wookiedoo");
    UpdateString(trie, "horse", "stallion");
    UpdateString(trie, "shaman", "horse");
    UpdateString(trie, "doge", "coin");
    trie->Delete(top::to_bytes(std::string{"ether"}));
    UpdateString(trie, "dog", "puppy");
    trie->Delete(top::to_bytes(std::string{"shaman"}));

    auto exp = xh256_t{top::from_hex("5991bb8c6514148a29db676a14ac506cd2cd5775ace63c30a4fe457715e9ac84", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->hash());
}

TEST_F(xtest_trie_fixture, test_empty_value_as_delete) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "do", "verb");
    UpdateString(trie, "ether", "wookiedoo");
    UpdateString(trie, "horse", "stallion");
    UpdateString(trie, "shaman", "horse");
    UpdateString(trie, "doge", "coin");
    UpdateString(trie, "ether", "");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "shaman", "");

    auto exp = xh256_t{top::from_hex("5991bb8c6514148a29db676a14ac506cd2cd5775ace63c30a4fe457715e9ac84", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->hash());
}

TEST_F(xtest_trie_fixture, test_replication) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    UpdateString(trie, "do", "verb");
    UpdateString(trie, "ether", "wookiedoo");
    UpdateString(trie, "horse", "stallion");
    UpdateString(trie, "shaman", "horse");
    UpdateString(trie, "doge", "coin");
    UpdateString(trie, "dog", "puppy");
    UpdateString(trie, "somethingveryoddindeedthis is", "myothernodedata");

    xbytes_t res0;
    res0 = trie->get(top::to_bytes(std::string{"do"}));
    ASSERT_EQ(res0, top::to_bytes(std::string{"verb"}));

    auto exp = trie->commit(ec).first;

    //----------new trie from db && root hash
    auto trie2 = xtrie_t::build_from(exp, test_trie_db_ptr, ec);
    ASSERT_TRUE(!ec);

    TESTINTRIE(trie2, "do", "verb");
    TESTINTRIE(trie2, "ether", "wookiedoo");
    TESTINTRIE(trie2, "horse", "stallion");
    TESTINTRIE(trie2, "shaman", "horse");
    TESTINTRIE(trie2, "doge", "coin");
    TESTINTRIE(trie2, "dog", "puppy");
    TESTINTRIE(trie2, "somethingveryoddindeedthis is", "myothernodedata");
}

TEST_F(xtest_trie_fixture, test_large_value) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

    trie->update(top::to_bytes(std::string{"key1"}), xbytes_t(4, 99));  //{99,99,99,99}
    trie->update(top::to_bytes(std::string{"key2"}), xbytes_t(32, 1));  //{1,1,1,...,1}

    auto exp = xh256_t{top::from_hex("afebee6cfce72f9d2a7a4f5926ac11f2a79bd75f3a9ae6358a08252ba5dce3be", ec)};

    ASSERT_TRUE(!ec);
    ASSERT_EQ(exp, trie->hash());
}

// Meaningless to encode the entire trie.
// trie->Encode() : rlp_encode(trie->m_root)
#if 0
TEST(xtrie, test_encoding_trie) {
    std::error_code ec;
    auto xtest_disk_db_ptr = std::make_shared<xtest_disk_db>();
    auto xtrie_db_ptr = xtrie_db_t::NewDatabase(xtest_disk_db_ptr);
    auto trie = xtrie_t::New({}, xtrie_db_ptr, ec);

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
#endif

TEST_F(xtest_trie_fixture, test_commit_to_disk) {
    std::error_code ec;
    auto trie = xtrie_t::build_from({}, test_trie_db_ptr, ec);

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

    ASSERT_TRUE(!ec);
    auto res = trie->commit(ec).first;
    ASSERT_TRUE(!ec);

    // havn't get anything from disk db for now
    ASSERT_TRUE(test_disk_db_ptr->Counter_Get.load() == 0);
    xdbg("trie->Commit() res: %s", res.hex().c_str());
    test_trie_db_ptr->Commit(res, nullptr, ec);

    // still zero
    ASSERT_TRUE(test_disk_db_ptr->Counter_Get.load() == 0);
    test_disk_db_ptr->debug();

#if 0
from geth log:
commit:  7e9809165850ffc89b2c5a7ef09de0f6794d4feb23fabd5130c9ee8fac2288be  >  e6808080cd82343289707570313265707931808080808080808080808080887075706431707931
commit:  875ba985b6780c0d671696f134d45ee78c2c8fe60e43affc8cbf6b1961467944  >  e4821132a07e9809165850ffc89b2c5a7ef09de0f6794d4feb23fabd5130c9ee8fac2288be
commit:  423bf699a4cf8dc85fb69a7f6d747f6622e56e6ff50648d02a5113480d254c0c  >  f83c808080a0875ba985b6780c0d671696f134d45ee78c2c8fe60e43affc8cbf6b19614679448080c63584636f696e808080808080808080857075707079
commit:  5e6f5838666bd784be80e3bb182ba87c100a0c9e94830104b15055028e3f4f3c  >  e217a0423bf699a4cf8dc85fb69a7f6d747f6622e56e6ff50648d02a5113480d254c0c
commit:  71559032341d9bb77d5642d7b614faddd81d0a70be4a114d9ebbb82236342b53  >  f5808080808080a05e6f5838666bd784be80e3bb182ba87c100a0c9e94830104b15055028e3f4f3c8080808080808080808476657262
commit:  991ad6ea3caf87e2e8e57757b1837a4a88da95413f0fc9ca66fe0600211ac537  >  f84080cf84206f67318970757031326470793180808080808080808080808080a071559032341d9bb77d5642d7b614faddd81d0a70be4a114d9ebbb82236342b5380
commit:  418fcbd2d1cbe16ee6ab25002dc8ea425864b8c17044ac1f7a2967cc95c988b2  >  e58080cb208970757031326570793180c9208770757070717931808080808080808080808080
commit:  2d86edc5b14964ea706671a92e595e4e0a84ef7398b4026b1bbba673567a75f4  >  e6840036f673a0418fcbd2d1cbe16ee6ab25002dc8ea425864b8c17044ac1f7a2967cc95c988b2
commit:  7932b99233bcf7e0ab5b8d64daef38763c56d9e452f6e66d84cd2988f1b55ae2  >  f851808080808080a0991ad6ea3caf87e2e8e57757b1837a4a88da95413f0fc9ca66fe0600211ac537a02d86edc5b14964ea706671a92e595e4e0a84ef7398b4026b1bbba673567a75f4808080808080808080
commit:  7c7fdf58b53675e77324e1009d6a2e3db641ab3765c997e859e84d3d2eac4f22  >  f85f80cf8420646f33897075653231707079318080a07932b99233bcf7e0ab5b8d64daef38763c56d9e452f6e66d84cd2988f1b55ae2d085207468657289776f6f6b6965646f6f8080cf85206f727365887374616c6c696f6e8080808080808080
commit:  9119e93677d864b77e5f5952b65379a66a50c63d11430446df7d1c784b57f3a8  >  e7963e67766572796f6464696e64656564746869732069738f6d796f746865726e6f646564617461
commit:  04609a5b16c1fcc0570f5726166606061fd0aa3e83cd6a8337db954d4a42b9fa  >  ea993361646e67766572796f6464696e64656564746869732069738f6d796f746865726e6f646564617461
commit:  3dcaaddb78eb0466e97725e5fc81cd46d3ef27d1dae85b9fff763898313f7396  >  f851808080808080a09119e93677d864b77e5f5952b65379a66a50c63d11430446df7d1c784b57f3a8a004609a5b16c1fcc0570f5726166606061fd0aa3e83cd6a8337db954d4a42b9fa808080808080808080
commit:  d58ed50236b001ae54f2b889b6c21ca650213719ff16729b45400696955e649c  >  e886006d65746869a03dcaaddb78eb0466e97725e5fc81cd46d3ef27d1dae85b9fff763898313f7396
commit:  1ac01b0bdf9c6f30ad8d063a7d4d9cd264dd2453b6f3dc8ba0db0f56af915c47  >  f83d8080808080808080cc8520616d616e85686f727365808080808080a0d58ed50236b001ae54f2b889b6c21ca650213719ff16729b45400696955e649c80
commit:  cbc3385c96e7851a6d4264d41593b58a313c652626ab6ca5071947d7247f5a52  >  e4820036a01ac01b0bdf9c6f30ad8d063a7d4d9cd264dd2453b6f3dc8ba0db0f56af915c47
commit:  9a53de09e43869f9ee8bea0535c15924a9f5f5f98c6ec81b4b3f2fc8c7090f8c  >  f851808080808080a07c7fdf58b53675e77324e1009d6a2e3db641ab3765c997e859e84d3d2eac4f22a0cbc3385c96e7851a6d4264d41593b58a313c652626ab6ca5071947d7247f5a52808080808080808080
#endif

#define ASSERT_DISK_DB_HAS(key, value)                                                                                                                                             \
    ASSERT_TRUE(test_disk_db_ptr->get(top::from_hex(std::string{key}, ec), ec) == top::from_hex(std::string{value}, ec));                                                          \
    ASSERT_TRUE(!ec);

    // clang-format off
    ASSERT_DISK_DB_HAS("7e9809165850ffc89b2c5a7ef09de0f6794d4feb23fabd5130c9ee8fac2288be",
                       "e6808080cd82343289707570313265707931808080808080808080808080887075706431707931");

    ASSERT_DISK_DB_HAS("875ba985b6780c0d671696f134d45ee78c2c8fe60e43affc8cbf6b1961467944",
                       "e4821132a07e9809165850ffc89b2c5a7ef09de0f6794d4feb23fabd5130c9ee8fac2288be");

    ASSERT_DISK_DB_HAS("423bf699a4cf8dc85fb69a7f6d747f6622e56e6ff50648d02a5113480d254c0c",
                       "f83c808080a0875ba985b6780c0d671696f134d45ee78c2c8fe60e43affc8cbf6b19614679448080c63584636f696e808080808080808080857075707079");

    ASSERT_DISK_DB_HAS("5e6f5838666bd784be80e3bb182ba87c100a0c9e94830104b15055028e3f4f3c",
                       "e217a0423bf699a4cf8dc85fb69a7f6d747f6622e56e6ff50648d02a5113480d254c0c");

    ASSERT_DISK_DB_HAS("71559032341d9bb77d5642d7b614faddd81d0a70be4a114d9ebbb82236342b53",
                       "f5808080808080a05e6f5838666bd784be80e3bb182ba87c100a0c9e94830104b15055028e3f4f3c8080808080808080808476657262");

    ASSERT_DISK_DB_HAS("991ad6ea3caf87e2e8e57757b1837a4a88da95413f0fc9ca66fe0600211ac537",
                       "f84080cf84206f67318970757031326470793180808080808080808080808080a071559032341d9bb77d5642d7b614faddd81d0a70be4a114d9ebbb82236342b5380");

    ASSERT_DISK_DB_HAS("418fcbd2d1cbe16ee6ab25002dc8ea425864b8c17044ac1f7a2967cc95c988b2",
                       "e58080cb208970757031326570793180c9208770757070717931808080808080808080808080");

    ASSERT_DISK_DB_HAS("2d86edc5b14964ea706671a92e595e4e0a84ef7398b4026b1bbba673567a75f4",
                       "e6840036f673a0418fcbd2d1cbe16ee6ab25002dc8ea425864b8c17044ac1f7a2967cc95c988b2");

    ASSERT_DISK_DB_HAS("7932b99233bcf7e0ab5b8d64daef38763c56d9e452f6e66d84cd2988f1b55ae2",
                       "f851808080808080a0991ad6ea3caf87e2e8e57757b1837a4a88da95413f0fc9ca66fe0600211ac537a02d86edc5b14964ea706671a92e595e4e0a84ef7398b4026b1bbba673567a75f4808080808080808080");

    ASSERT_DISK_DB_HAS("7c7fdf58b53675e77324e1009d6a2e3db641ab3765c997e859e84d3d2eac4f22",
                       "f85f80cf8420646f33897075653231707079318080a07932b99233bcf7e0ab5b8d64daef38763c56d9e452f6e66d84cd2988f1b55ae2d085207468657289776f6f6b6965646f6f8080cf85206f727365887374616c6c696f6e8080808080808080");

    ASSERT_DISK_DB_HAS("9119e93677d864b77e5f5952b65379a66a50c63d11430446df7d1c784b57f3a8",
                       "e7963e67766572796f6464696e64656564746869732069738f6d796f746865726e6f646564617461");

    ASSERT_DISK_DB_HAS("04609a5b16c1fcc0570f5726166606061fd0aa3e83cd6a8337db954d4a42b9fa",
                       "ea993361646e67766572796f6464696e64656564746869732069738f6d796f746865726e6f646564617461");

    ASSERT_DISK_DB_HAS("3dcaaddb78eb0466e97725e5fc81cd46d3ef27d1dae85b9fff763898313f7396",
                       "f851808080808080a09119e93677d864b77e5f5952b65379a66a50c63d11430446df7d1c784b57f3a8a004609a5b16c1fcc0570f5726166606061fd0aa3e83cd6a8337db954d4a42b9fa808080808080808080");

    ASSERT_DISK_DB_HAS("d58ed50236b001ae54f2b889b6c21ca650213719ff16729b45400696955e649c",
                       "e886006d65746869a03dcaaddb78eb0466e97725e5fc81cd46d3ef27d1dae85b9fff763898313f7396");

    ASSERT_DISK_DB_HAS("1ac01b0bdf9c6f30ad8d063a7d4d9cd264dd2453b6f3dc8ba0db0f56af915c47",
                       "f83d8080808080808080cc8520616d616e85686f727365808080808080a0d58ed50236b001ae54f2b889b6c21ca650213719ff16729b45400696955e649c80");

    ASSERT_DISK_DB_HAS("cbc3385c96e7851a6d4264d41593b58a313c652626ab6ca5071947d7247f5a52",
                       "e4820036a01ac01b0bdf9c6f30ad8d063a7d4d9cd264dd2453b6f3dc8ba0db0f56af915c47");

    ASSERT_DISK_DB_HAS("9a53de09e43869f9ee8bea0535c15924a9f5f5f98c6ec81b4b3f2fc8c7090f8c",
                       "f851808080808080a07c7fdf58b53675e77324e1009d6a2e3db641ab3765c997e859e84d3d2eac4f22a0cbc3385c96e7851a6d4264d41593b58a313c652626ab6ca5071947d7247f5a52808080808080808080");
    // clang-format on

#undef ASSERT_DISK_DB_HAS

    // read 17 times above
    ASSERT_TRUE(test_disk_db_ptr->Counter_Get.load() == 17);

    // now trie_db.dirties is empty. all from cleans :
    TESTINTRIE(trie, "do", "verb");
    TESTINTRIE(trie, "ether", "wookiedoo");
    TESTINTRIE(trie, "horse", "stallion");
    TESTINTRIE(trie, "shaman", "horse");
    TESTINTRIE(trie, "doge", "coin");
    TESTINTRIE(trie, "dog", "puppy");
    TESTINTRIE(trie, "daog1", "pup12dpy1");
    TESTINTRIE(trie, "dsog2", "pup12epy1");
    TESTINTRIE(trie, "ado3", "pue21ppy1");
    TESTINTRIE(trie, "dsog4", "puppqy1");
    TESTINTRIE(trie, "dog12", "pupd1py1");
    TESTINTRIE(trie, "dog1242", "pup12epy1");
    TESTINTRIE(trie, "somethingveryoddindeedthis is", "myothernodedata");
    TESTINTRIE(trie, "somethisadngveryoddindeedthis is", "myothernodedata");
    
    // still 17, above is read from trie_db.cleans
    ASSERT_TRUE(test_disk_db_ptr->Counter_Get.load() == 17);

    // use diskdb to rebuild a new trie:
    auto new_clean_trie_db_ptr = xtrie_db_t::NewDatabase(test_disk_db_ptr);
    auto new_trie = xtrie_t::build_from(trie->hash(), new_clean_trie_db_ptr, ec);
    // read once of root hash: +1
    ASSERT_TRUE(test_disk_db_ptr->Counter_Get.load() == 18);

    // now trie_db.dirties && cleans is empty. check all nodes from disk db :
    TESTINTRIE(new_trie, "do", "verb");
    TESTINTRIE(new_trie, "ether", "wookiedoo");
    TESTINTRIE(new_trie, "horse", "stallion");
    TESTINTRIE(new_trie, "shaman", "horse");
    TESTINTRIE(new_trie, "doge", "coin");
    TESTINTRIE(new_trie, "dog", "puppy");
    TESTINTRIE(new_trie, "daog1", "pup12dpy1");
    TESTINTRIE(new_trie, "dsog2", "pup12epy1");
    TESTINTRIE(new_trie, "ado3", "pue21ppy1");
    TESTINTRIE(new_trie, "dsog4", "puppqy1");
    TESTINTRIE(new_trie, "dog12", "pupd1py1");
    TESTINTRIE(new_trie, "dog1242", "pup12epy1");
    TESTINTRIE(new_trie, "somethingveryoddindeedthis is", "myothernodedata");
    TESTINTRIE(new_trie, "somethisadngveryoddindeedthis is", "myothernodedata");
    
    // all enc were loaded. +16
    ASSERT_TRUE(test_disk_db_ptr->Counter_Get.load() == 34);
}

NS_END4
