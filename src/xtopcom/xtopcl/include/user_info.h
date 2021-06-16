#pragma once

#ifndef XCHAIN_USER_INFO_H
#define XCHAIN_USER_INFO_H

#include <stdio.h>

#include <array>
#include <cstring>
#include <iostream>
#include <string>

const std::string TOP_ACCOUNT_PREFIX = "T00000";
constexpr int BASE64_PRI_KEY_LEN = 44;
constexpr int HEX_PRI_KEY_LEN = 64;
constexpr int HEX_PUB_KEY_LEN = 128;

namespace xChainSDK {
constexpr uint8_t PRI_KEY_LEN = 32;
struct uinfo {
    std::string account;
    std::array<uint8_t, PRI_KEY_LEN> private_key;
};

struct user_info {
    user_info() {
        clear();
    }

    std::string identity_token;
    std::string account;
    uint64_t balance;
    std::string last_hash;
    uint64_t last_hash_xxhash64;
    uint32_t nonce;
    std::string secret_key;
    std::string sign_method;
    std::string sign_version;
    std::array<uint8_t, PRI_KEY_LEN> private_key;
    uinfo child;
    uinfo contract;

    void clear() {
        identity_token.clear();
        account.clear();
        balance = 0;
        last_hash.clear();
        last_hash_xxhash64 = 0;
        nonce = 0;
        secret_key.clear();
        sign_method.clear();
        sign_version.clear();
        private_key.fill(0);
    }
};
}  // namespace xChainSDK

#endif  // !XCHAIN_USER_INFO_H
