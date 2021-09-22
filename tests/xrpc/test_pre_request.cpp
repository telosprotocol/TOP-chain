#include "gtest/gtest.h"
#include "xrpc/prerequest/xpre_request_handler_mgr.h"
#include "xrpc/xjson_proc.h"

using namespace top;
using namespace top::xrpc;

class test_pre_request : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

public:
    xpre_request_handler_mgr m_handle;
    xpre_request_data_t m_request;
    std::string m_request_data{"version=1.0&target_account_addr=T-123456789012345678901234567890123&method=requestToken&sequence_id=1"};
};

TEST_F(test_pre_request, illegal_prerequest) {
    m_handle.execute(m_request, m_request_data);
    EXPECT_EQ(true, m_request.m_finish);
    xjson_proc_t json_proc;
    int32_t cnt{0};
    try {
        json_proc.parse_json(m_request);
    } catch (xrpc_error & e) {
        EXPECT_STREQ("body json parse error", e.what());
        EXPECT_EQ(-32700, e.code().value());
        cnt++;
    } catch (...) {
        cnt++;
    }
    EXPECT_EQ(1, cnt);
}
