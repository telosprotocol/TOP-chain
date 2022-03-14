#pragma once

#include <gtest/gtest.h>

namespace top {
namespace evm {
namespace tests {

class xtest_evm_fixture : public testing::Test {
public:
    xtest_evm_fixture() = default;
    xtest_evm_fixture(xtest_evm_fixture const &) = delete;
    xtest_evm_fixture & operator=(xtest_evm_fixture const &) = delete;
    xtest_evm_fixture(xtest_evm_fixture &&) = default;
    xtest_evm_fixture & operator=(xtest_evm_fixture &&) = default;
    ~xtest_evm_fixture() override = default;

protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();
    }
};

}  // namespace tests
}  // namespace evm
}  // namespace top
