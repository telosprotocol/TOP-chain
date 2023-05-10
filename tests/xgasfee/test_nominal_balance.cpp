
#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xckey.h"
#include "xgasfee/xgas_estimate.h"

#include <algorithm>

using namespace top;
using namespace top::base;
using namespace top::data;

class test_nominal_balance : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_nominal_balance, test_nominal_balance_boundary) {
    std::vector<evm_common::u256> top_balance_vec = {
        ASSET_TOP(0),
        ASSET_TOP(199),
        ASSET_TOP(200),
        ASSET_TOP(1000),
        ASSET_TOP(10000),
    };

    evm_common::u256 boundary_lower = top::gasfee::xtop_gas_tx_operator::utop_to_wei(ASSET_TOP(200));
    evm_common::u256 boundary_upper = top::gasfee::xtop_gas_tx_operator::utop_to_wei(ASSET_TOP(1000));

    std::vector<evm_common::u256> eth_balance_vec;
    eth_balance_vec.push_back(evm_common::u256{0});
    eth_balance_vec.push_back(evm_common::u256{1});

    for (auto top_balance : top_balance_vec) {
        for (auto eth_balance : eth_balance_vec) {
            auto balance = top::gasfee::xgas_estimate::get_nominal_balance(top_balance, eth_balance);
            if (top_balance == ASSET_TOP(0) || top_balance == ASSET_TOP(199)) {
                if (eth_balance == 0) {
                    EXPECT_EQ(balance, 0);
                } else {
                    EXPECT_EQ(balance, eth_balance);
                }
            } else if (top_balance == ASSET_TOP(200)) {
                if (eth_balance == 0) {
                    EXPECT_EQ(balance, boundary_lower);
                } else {
                    EXPECT_EQ(balance, eth_balance);
                } 
            } else if (top_balance == ASSET_TOP(1000)) {
                if (eth_balance == 0) {
                    EXPECT_EQ(balance, boundary_upper);
                } else {
                    EXPECT_EQ(balance, eth_balance);
                } 
            } else {
                if (eth_balance == 0) {
                    EXPECT_EQ(balance, boundary_upper);
                } else {
                    EXPECT_EQ(balance, eth_balance);
                } 
            }
        }
    }
}
