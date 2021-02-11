
#include <string.h>

#include <string>
#include <iostream>

#include <gtest/gtest.h>
#define private public
#define protected public
#include "xpbase/base/xid/xid_db_session.h"

namespace top {

namespace base {

namespace test {

class TestXIdDBSession : public testing::Test {
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

TEST_F(TestXIdDBSession, All) {

    {
        const std::string xid_key;
        const XID xid;
        XIdDBSession::Insert(xid_key, xid);
    }

    {
        const std::string xid_key;
        XIDSptr xid;
        XIdDBSession::Select(xid_key, xid);
    }
    
    {
        const std::string xid_key;
        XIdDBSession::Delete(xid_key);
    }
}

}  // namespace test

}  // namespace kadmlia

}  // namespace top
