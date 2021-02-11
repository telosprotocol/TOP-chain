
#include <gtest/gtest.h>

#include <iostream>

#include "xpbase/base/xid/xid_def.h"

namespace top {
namespace base {
namespace test {

class TestXidDef : public testing::Test {
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

TEST_F(TestXidDef, XID) {
    XID xid;
    xid.Reset();
}

TEST_F(TestXidDef, XID_2) {
    const uint32_t xnetwork_id = 1;
    const uint8_t zone_id = 2;
    const std::string public_key;
    const std::string private_key;
    XID xid(xnetwork_id, zone_id, public_key, private_key);
    xid.GetPrivateKey();

    XID xid2;
    ASSERT_FALSE(xid == xid2);
}

TEST_F(TestXidDef, XIDType) {
    XIDType xid_type;
    xid_type.Reset();
}

TEST_F(TestXidDef, XIDType_2) {
    const uint32_t xnetwork_id = 1;
    const uint8_t zone_id = 2;
    XIDType xid_type(xnetwork_id, zone_id);

    XIDType xid_type2;
    ASSERT_TRUE(xid_type < xid_type2);
}

}  // namespace test
}  // namespace base
}  // namespace top
