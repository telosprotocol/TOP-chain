//
//  test_trezor_crypto.cc
//  test
//
//  Created by Sherlock on 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <gtest/gtest.h>

extern "C"
{
    #include "trezor-crypto/rand.h"
}

namespace top {
namespace base {
namespace test {

class TestTrezorCrypto : public testing::Test {
public:
	static void SetUpTestCase() {
	}

	static void TearDownTestCase() {
	}

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}

};

TEST_F(TestTrezorCrypto, SetGet) {
    ASSERT_NE(random32(), 1013904223);
}

}  // namespace test
}  // namespace kadmlia
}  // namespace top
