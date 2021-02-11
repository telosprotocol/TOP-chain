#include "gtest/gtest.h"

#include <stdio.h>
#include <string>
#include "xconfig/xconfig_register.h"
#include "xverifier/xverifier_utl.h"

class test_utl : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(test_utl, utl) {

    using namespace top::xverifier;

    // privkey_pubkey_is_match
    top::utl::xecprikey_t priv1;
    top::utl::xecpubkey_t pub1 = priv1.get_public_key();
    top::utl::xecprikey_t priv2;
    top::utl::xecpubkey_t pub2 = priv2.get_public_key();

    auto ret = xtx_utl::privkey_pubkey_is_match(std::string((char*)priv1.data(), priv1.size()), std::string((char*)pub1.data(), pub1.size()));
    ASSERT_EQ(top::xverifier::xverifier_success, ret);

    ret = xtx_utl::privkey_pubkey_is_match(std::string((char*)priv1.data(), priv1.size()), std::string((char*)pub2.data(), pub2.size()));
    ASSERT_EQ(top::xverifier::xverifier_error_priv_pub_not_match, ret);
}
