// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#include <iostream>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/line_parser.h"
#include "xkad/routing_table/routing_utils.h"
#define private public
#include "xkad/routing_table/bootstrap_cache.h"

namespace top {
namespace kadmlia {
namespace test {

class TestBootstrapCache : public testing::Test {
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

TEST_F(TestBootstrapCache, GetSetCache) {
    uint64_t service_type = kRoot;
    auto cache = std::make_shared<BootstrapCache>(service_type);
    ASSERT_NE(nullptr, cache);
    VecBootstrapEndpoint vec_bootstrap_endpoint = {
        {"1.1.1.1", 11111},
        {"2.2.2.2", 22222}
    };
    ASSERT_FALSE(cache->SetCache(vec_bootstrap_endpoint));
    VecBootstrapEndpoint vec2;
    ASSERT_FALSE(cache->GetCache(vec2));

    auto get_cache_callback 
        = [](const uint64_t& service_type, VecBootstrapEndpoint& vec_bootstrap_endpoint) -> bool {
            return true;
    };

    auto set_cache_callback 
        = [](const uint64_t& service_type, const VecBootstrapEndpoint& vec_bootstrap_endpoint) -> bool {
            return true;
    };
    cache->RegisterBootstrapCacheCallback(get_cache_callback, set_cache_callback);

    ASSERT_TRUE(cache->SetCache(vec_bootstrap_endpoint));
    ASSERT_TRUE(cache->GetCache(vec2));
}

}  // namespace test
}  // namespace kadmlia
}  // namespace top
