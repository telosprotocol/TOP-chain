#include <gtest/gtest.h>
#include <thread>
#include "xsync/xsession.h"
#include "xcommon/xsharding_info.h"
#include "common.h"

using namespace top::sync;

extern 
top::vnetwork::xvnode_address_t
get_address(int id, const std::string& name);

TEST(xsession_t, tests) {
    xsession_t s;
    for(uint32_t i=0;i<9;i++) {
        ASSERT_TRUE(s.plus(10, 2));
    }
    ASSERT_FALSE(s.plus(10, 2));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for(uint32_t i=0;i<10;i++) {
        ASSERT_TRUE(s.plus(10, 2));
    }
    ASSERT_FALSE(s.plus(10, 2));
}

TEST(xsession_manager_t, tests) {

    xsession_manager_t session_mgr(2);

    auto addr1 = get_validator_address(0, 0, "test0", 0);
    auto addr2 = get_validator_address(0, 0, "test1", 1);
    auto addr3 = get_validator_address(0, 0, "test2", 2);

    for(uint32_t i=0;i<5;i++) {
        ASSERT_TRUE(session_mgr.plus(addr1, 11, 5, 2));
    }
    for(uint32_t i=0;i<5;i++) {
        ASSERT_TRUE(session_mgr.plus(addr2, 11, 5, 2));
    }
    ASSERT_TRUE(session_mgr.plus(addr3, 11, 5, 2));
    ASSERT_FALSE(session_mgr.plus(addr3, 11, 5, 2));
}