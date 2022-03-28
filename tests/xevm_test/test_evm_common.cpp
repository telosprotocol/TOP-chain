#include "xevm_common/address.h"
#include "xevm_common/common.h"

#include <gtest/gtest.h>

using namespace top::evm_common;

TEST(test_boost, uint) {
    u512 n = 1;
    for (int index = 1; index < 90; ++index) {
        n = n * index;
        std::cout << n << std::endl;
    }
}

TEST(test_boost, hash) {
    auto raddr = Address::random();
    std::cout << raddr << std::endl;
}
