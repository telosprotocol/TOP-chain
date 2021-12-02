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

TEST_F(test_utl, address_valid) {

    using namespace top::xverifier;
    std::vector<std::string> addr_list = {"T80000d00e5539d4306fa4e0e274bc4a795e863d44e2b0", "T80000a65b9f527d0c2a80af21a64ceee9ac19cbed55df", "T80000d2d492b587849a51d72b26f0605b693321c34d6d",
        "T80000a65b9f527d0c2a80af21a64ceee9ac19cbed55df"};

    for (auto const& addr: addr_list) {
        auto res = top::xverifier::xtx_utl::address_is_valid(addr);
        // if (res != xverifier_error::xverifier_success) std::cout << "addr: " << addr << " is not valid\n";
        EXPECT_EQ(res, xverifier_error::xverifier_success);
     }

}

