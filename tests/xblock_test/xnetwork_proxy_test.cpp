#include "gtest/gtest.h"
#include <functional>
#include "xunit_service/xnetwork_proxy.h"
namespace top {
using namespace xunit_service;
class xnetwork_proxy_test : public testing::Test {
 protected:
    void SetUp() override {
     }

    void TearDown() override {
    }
 public:

};

class xnetwork_proxy_mock : public xnetwork_proxy {

};

TEST_F(xnetwork_proxy_test, elect) {
}
}  // namespace top
