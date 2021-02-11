#include "../test_common.h"
#include <sys/time.h>
#include <iostream>

NS_BEG2(top, xmutisig)
const uint32_t NUM = 10000;

TEST_F(test_schnorr_mutisig, compare_ckey) {
    utl::xecprikey_t prikey;
    utl::xecpubkey_t pubkey = prikey.get_public_key();

    BIGNUM* new_bn = BN_new();

    assert(nullptr != new_bn);

    BIGNUM *bn = BN_bin2bn((unsigned char*)prikey.data(), prikey.size(), new_bn);

    xprikey xmuti_pri(new_bn);
    xpubkey xmuti_pub(xmuti_pri);

    std::string pri_str((char *)prikey.data(), prikey.size());
    std::string pub_str((char *)pubkey.data(), pubkey.size());
    std::string muti_pri_str = xmuti_pri.get_serialize_str();
    std::string muti_pub_str = xmuti_pub.get_serialize_str();
    std::cout << "ckey prikey " << base::xstring_utl::to_hex(pri_str) << std::endl;
    std::cout << "ckey pubkey " << base::xstring_utl::to_hex(pub_str) << std::endl;
    std::cout << "muti prikey " << base::xstring_utl::to_hex(muti_pri_str) << std::endl;
    std::cout << "muti pubkey " << base::xstring_utl::to_hex(muti_pub_str) << std::endl;

}

TEST_F(test_schnorr_mutisig, test_ckey_performance) {
    utl::xecprikey_t prikey;
    utl::xecpubkey_t pubkey = prikey.get_public_key();
    uint256_t data256 = pubkey.to_hash();
    uint8_t tmp[65];
    utl::xecdsasig_t sig_ctx(tmp);
    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        sig_ctx = prikey.sign(data256);
        assert(pubkey.verify_signature(sig_ctx, data256));
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::test_ckey_sign_performance " << NUM / ms_total << "/ms" << std::endl;

    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        bool check = pubkey.verify_signature(sig_ctx, data256);
        assert(check);
    }
    gettimeofday(&end, NULL);
    ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::test_ckey_verify_performance " << NUM / ms_total << "/ms" << std::endl;

}

TEST_F(test_schnorr_mutisig, test_compress_ckey_performance) {
    top::utl::xecprikey_t prikey;
    utl::xecpubkey_t ckey_pub = prikey.get_public_key();
    uint256_t data256 = ckey_pub.to_hash();
    std::string pubkey = prikey.get_compress_public_key();
    uint8_t tmp[65];
    ::memset(tmp, 0, sizeof(tmp));
    ::memcpy(tmp, pubkey.c_str(), pubkey.size());

    utl::xecpubkey_t final_pub(tmp);
    utl::xecdsasig_t sig_ctx = prikey.sign(data256);

    bool check = final_pub.verify_signature(sig_ctx, data256, true);
    assert(check);
}

NS_END2
