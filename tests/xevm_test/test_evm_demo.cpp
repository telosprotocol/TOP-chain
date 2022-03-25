#include "xevm/xevm.h"

#include <gtest/gtest.h>

TEST(test_evm, demo1) {
    top::evm::xtop_evm evm{nullptr, nullptr};
    evm.execute({}, {});
}