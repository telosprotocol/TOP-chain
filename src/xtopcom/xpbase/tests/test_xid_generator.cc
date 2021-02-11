//
//  test_xid_manager.cc
//  test
//
//  Created by Sherlock on 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <string.h>

#include <string>
#include <iostream>

#include <gtest/gtest.h>
#define private public
#define protected public
#include "xpbase/base/xip_parser.h"
#include "xpbase/base/xip_generator.h"
#include "xpbase/base/xid/xid_generator.h"

namespace top {

namespace base {

namespace test {

class TestXIdGenerator : public testing::Test {
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

TEST_F(TestXIdGenerator, All) {
    XIDGenerator xid_generator;
    XIDType xid_type;
    xid_type.xnetwork_id_ = 19;
    xid_type.zone_id_ = 250;
    ASSERT_TRUE(xid_generator.CreateXID(xid_type));
    XID xid;
    ASSERT_TRUE(xid_generator.GetXID(xid));
    ASSERT_TRUE(xid_generator.SaveXID());
    XID xid_get;
    // ASSERT_TRUE(xid_generator.GetXIDFromDB(xid_type, xid_get));
    // ASSERT_EQ(xid.ToString(), xid_get.ToString());
    ASSERT_EQ(xid.ToString().size(), kNodeIdSize);
    ASSERT_EQ(xid.GetPublicKey().size(), 32);
    ASSERT_EQ(xid.GetXNetworkID(), 19);
    ASSERT_EQ(xid.GetZoneID(), 250);
    ASSERT_TRUE(xid_generator.DeleteXID(xid_type));
    xid_type.Reset();
    xid.Reset();
    xid_get.Reset();
    xid_generator.Reset();
}

TEST_F(TestXIdGenerator, GetXIDFromDB) {
    XIDGenerator xid_generator;
    XIDType xid_type;
    xid_type.xnetwork_id_ = 19;
    xid_type.zone_id_ = 250;
    XID xid_get;
    xid_generator.GetXIDFromDB(xid_type, xid_get);
}

}  // namespace test

}  // namespace kadmlia

}  // namespace top
