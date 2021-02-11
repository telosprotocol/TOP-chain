//
//  test_xip_generator.cc
//  test
//
//  Created by Charlie Xie 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <gtest/gtest.h>

#include <iostream>

#include "xpbase/base/xip_generator.h"
#include "xpbase/base/xip_parser.h"

namespace top {

namespace base {

namespace test {

class TestXipGenerator : public testing::Test {
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

TEST_F(TestXipGenerator, All) {
    XipGenerator xip_gen;
    XipParser xip;
    ASSERT_TRUE(xip_gen.CreateXip(19, 120, kRoleEdge, "192.168.0.189", xip));
    ASSERT_EQ((uint32_t)xip.xnetwork_id(), 19);
    ASSERT_EQ((uint32_t)xip.zone_id(), 120);
    ASSERT_EQ((uint32_t)xip.network_type(), kRoleEdge);
    ASSERT_EQ((uint32_t)xip.cluster_id(), (uint32_t)(192 & 0x7F));
    ASSERT_EQ((uint32_t)xip.group_id(), 168);
    ASSERT_EQ((uint32_t)xip.node_id(), 48384 % 255);
}

TEST_F(TestXipGenerator, IpAddr)  {
    struct in_addr addr;
    ASSERT_EQ(inet_pton(AF_INET, "127.0.0.1", &addr.s_addr), 1);
    ASSERT_EQ(addr.s_addr, 0x100007f);
    char str[20];
    ASSERT_TRUE(inet_ntop(AF_INET, &addr.s_addr, str, sizeof(str)));
    ASSERT_EQ(std::string(str), std::string("127.0.0.1"));
}

}  // namespace test

}  // namespace base

}  // namespace top
