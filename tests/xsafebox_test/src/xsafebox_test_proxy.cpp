#include "gtest/gtest.h"
#include "xbase/xns_macro.h"
#include "xbase/xutl.h"
#include "xbasic/xhex.h"
#include "xcrypto/xckey.h"
#include "xmutisig/xmutisig.h"
#include "xmutisig/xrand_pair.h"
#include "xmutisig/xschnorr.h"
#include "xsafebox/safebox_proxy.h"
#include "xutility/xhash.h"

NS_BEG3(top, safebox, tests)

TEST(test_safebox, sign_and_verify) {
    auto & safebox = xsafebox_proxy::get_instance();

    top::utl::xecprikey_t rand_prikey;
    top::utl::xecpubkey_t pubkey = rand_prikey.get_public_key();
    std::string T0_account = rand_prikey.to_account_address('0', 0);
    std::string T8_account = rand_prikey.to_account_address('8', 0);
    std::string pubkey_base64_str = base::xstring_utl::base64_encode(pubkey.data(), pubkey.size());
    std::string prikey_str = std::string((const char *)rand_prikey.data(), rand_prikey.size());
    std::string pubkey_str = std::string((const char *)pubkey.data(), pubkey.size());

    safebox.add_key_pair(top::xpublic_key_t(pubkey_str), std::move(prikey_str));

    for (std::size_t test_cnt = 0; test_cnt < 1000; ++test_cnt) {
        std::string msg = "message" + std::to_string(test_cnt);
        auto sign_result = safebox.get_proxy_signature(pubkey_str, msg);

        auto verify_result = xmutisig::xmutisig::verify_sign(msg, xmutisig::xpubkey{pubkey_str}, sign_result.second, sign_result.first, xmutisig::xschnorr::instance());
        EXPECT_TRUE(verify_result);
    }
}

TEST(test_safebox, secp256sign_and_verify) {
    auto & safebox = xsafebox_proxy::get_instance();
    for (auto _ = 0; _ < 1000; ++_) {
        top::utl::xecprikey_t rand_prikey;
        top::utl::xecpubkey_t pubkey = rand_prikey.get_public_key();
        std::string T0_account = rand_prikey.to_account_address('0', 0);
        std::string T8_account = rand_prikey.to_account_address('8', 0);
        std::string pubkey_base64_str = base::xstring_utl::base64_encode(pubkey.data(), pubkey.size());
        std::string prikey_str = std::string((const char *)rand_prikey.data(), rand_prikey.size());
        std::string pubkey_str = std::string((const char *)pubkey.data(), pubkey.size());

        safebox.add_key_pair(top::xpublic_key_t(pubkey_str), std::move(prikey_str));

        uint256_t hash_value;
        top::utl::xsha2_256_t hasher;
        hasher.update(T0_account.c_str(), T0_account.size());
        hasher.get_hash(hash_value);

        uint8_t signs[65];

        auto sign_result = safebox.get_proxy_secp256_signature(pubkey_str, hash_value);

        assert(sign_result.size() == 65);
        memcpy(signs, sign_result.c_str(), sign_result.size());

        top::utl::xecdsasig_t signature{signs};

        auto verify_result = pubkey.verify_signature(signature, hash_value);

        EXPECT_TRUE(verify_result);
    }
}

NS_END3