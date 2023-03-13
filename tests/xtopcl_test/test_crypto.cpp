#include "xtopcl/include/api_method.h"
#include "xtopcl/include/xcrypto.h"

#include <gtest/gtest.h>

#if 0
TEST(test_crypto, decrypt_T0) {
    Json::Value keystore_info;
    EXPECT_TRUE(xChainSDK::xcrypto::parse_keystore("./T0", keystore_info));

    std::string pri_key;
    EXPECT_TRUE(xChainSDK::xcrypto::decrypt_keystore_by_password("*******", keystore_info, pri_key));

    std::cout << pri_key << std::endl;
}

TEST(test_crypto, decrypt_T8) {
    Json::Value keystore_info;
    EXPECT_TRUE(xChainSDK::xcrypto::parse_keystore("./T8", keystore_info));

    std::string pri_key;  // hex string
    EXPECT_TRUE(xChainSDK::xcrypto::decrypt_keystore_by_password("123", keystore_info, pri_key));

    std::cout << pri_key << std::endl;
}
#endif