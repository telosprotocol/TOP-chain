
#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xckey.h"
#include "xbasic/xhex.h"
#include <algorithm>
#include "xgasfee/xgas_estimate.h"

using namespace top;
using namespace top::base;
using namespace top::data;


class test_eth_hex : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_eth_hex, eth_hex_base_test) {
  for (uint32_t i = 0; i < 256; i++) {
        
        uint64_t test_value = 0;
        if(i == 0) {
            test_value = 0;
        } else {
            test_value = 1;
        }

        top::evm_common::u256 h256_str_0{test_value};
        h256_str_0 <<= i;
        std::string hex_str_1 = top::to_hex_shrink_0((top::evm_common::h256)h256_str_0);
        std::string hex_str_2 = top::to_hex((top::evm_common::h256)h256_str_0);

        std::string::size_type found = hex_str_2.find_first_not_of("0");
        //std::cout << " index " << i << " found " << found <<  " hex_str_1 " << hex_str_1 << std::endl;
        //std::cout << " hex_str_0   " << hex_str_2 << std::endl;
        if (found != std::string::npos) {
            hex_str_2 = hex_str_2.substr(found);
        } else {
            hex_str_2 = "0";
        }

        ASSERT_EQ(hex_str_1, hex_str_2);

        std::string prefixed_hex_str_1 = top::to_hex_prefixed_shrink_0((top::evm_common::h256)h256_str_0);
        std::string prefixed_hex_str_2 = top::to_hex((top::evm_common::h256)h256_str_0);

        found = prefixed_hex_str_2.find_first_not_of("0");
        if (found != std::string::npos) {
            prefixed_hex_str_2 =  "0x" + prefixed_hex_str_2.substr(found);
        } else {
            prefixed_hex_str_2 = "0x0";
        }
        ASSERT_EQ(prefixed_hex_str_1, prefixed_hex_str_2);
  }

}

TEST_F(test_eth_hex, eth_hex_gas_price_test) {
    auto gas_price = top::gasfee::xgas_estimate::base_price();
    std::string gas_price_str_0 = top::to_hex_shrink_0((top::evm_common::h256)gas_price);
    uint64_t top_gas_price = XGET_ONCHAIN_GOVERNANCE_PARAMETER(top_eth_base_price);
    std::string top_gas_price_str =  base::xstring_utl::tostring(top_gas_price);

}


