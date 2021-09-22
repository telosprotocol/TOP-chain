#include "gtest/gtest.h"
#include <map>
#include <string>
#include "xrpc/xrpc_signature.h"

TEST(xrpt_signature_case, test_signature_false) {
    std::string msg = "target_account_addr=T-123456789012345678901234567890123&body={\n   \"account_address\" : \"T-123456789012345678901234567890123\"\n}\n&method=requestToken&sequence_id=1&identity_token=12345abcde&version=1.0&signature=JM2BX3WLLCFMLLEHDLTBQS7ABGQEMWEAGJRBCSH3OSWRZKL5WNPA";
    top::xChainRPC::xrpc_signature sign;
    bool br = sign.check(msg, "12345abcde");
    EXPECT_FALSE(br);
}

TEST(xrpt_signature_case, test_signature_true) {
    std::string msg = "target_account_addr=T-123456789012345678901234567890123&body={\n   \"account_address\" : \"T-123456789012345678901234567890123\"\n}\n&method=requestToken&sequence_id=1&identity_token=12345abcde&version=1.0&signature=47ZWMBSCUFD5SVI3HOXNQ7QS5SPAQFQPDSHANTUJG3AWCEFB2AMA";
    top::xChainRPC::xrpc_signature sign;
    bool br = sign.check(msg, "12345abcde");
    EXPECT_TRUE(br);
}
