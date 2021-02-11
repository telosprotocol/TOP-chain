
#include <string.h>

#include <string>
#include <iostream>
#include <memory>

#include <gtest/gtest.h>
#include "xpbase/base/top_utils.h"
#define private public
#define protected public
#include "xpbase/base/kad_key/chain_kadmlia_key.h"

namespace top {

namespace base {

namespace test {

class TestChainKadmliaKey : public testing::Test {
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

TEST_F(TestChainKadmliaKey, ChainKadmliaKey) {
    ChainKadmliaKey kadkey;
    auto ret = kadkey.Get();
    ASSERT_EQ(36u, ret.size());

    kadkey.GetPrivateKey();
    kadkey.GetPublicKey();

    {
        kadkey.xip_.set_xnetwork_id(1);
        kadkey.GetServiceType();
        uint8_t network_type = 1;
        kadkey.GetServiceType(network_type);
    }
}

TEST_F(TestChainKadmliaKey, ChainKadmliaKey_reserve) {
    uint32_t reserve = 0;
    KadmliaKey* kadkey = new ChainKadmliaKey(reserve);
    delete kadkey;
    kadkey = nullptr;
}

TEST_F(TestChainKadmliaKey, ChainKadmliaKey_service_type) {
    uint64_t service_type = kRoot;
    ChainKadmliaKey kadkey(service_type);
}

TEST_F(TestChainKadmliaKey, ChainKadmliaKey_xip) {
    const base::XipParser xip;
    ChainKadmliaKey kadkey(xip);
}

TEST_F(TestChainKadmliaKey, ChainKadmliaKey_strkey) {
    std::string str_key;
    str_key.resize(kNodeIdSize);
    ChainKadmliaKey kadkey(str_key);
}

TEST_F(TestChainKadmliaKey, ChainKadmliaKey_hash) {
    const std::string str_for_hash;
    bool hash_tag = true;
    ChainKadmliaKey kadkey(str_for_hash, hash_tag);
}

}  // namespace test

}  // namespace kadmlia

}  // namespace top
