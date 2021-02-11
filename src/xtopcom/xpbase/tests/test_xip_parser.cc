//
//  test_xip_parser.cc
//  test
//
//  Created by Sherlock on 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <string.h>

#include <string>
#include <iostream>

#include <gtest/gtest.h>

#include "xpbase/base/xip_parser.h"

namespace top {

namespace base {

namespace test {

class TestXipParser : public testing::Test {
public:
	static void SetUpTestCase() {
        uint64_t test_num = 0;
        test_num = test_num | (((uint64_t)(1928)) << 32);
        uint64_t test_get = test_num >> 32 & 0xFFFFFF;
        std::cout << test_get << std::endl;
        test_num = test_num & (((uint64_t)(0xF000000)) << 32);
        test_num = test_num | (((uint64_t)(5464)) << 32);
        test_get = test_num >> 32 & 0xFFFFFF;
        std::cout << test_get << std::endl;
        test_num = test_num & (((uint64_t)(0xF000000)) << 32);
        test_num = test_num | (((uint64_t)(234235)) << 32);
        test_get = test_num >> 32 & 0xFFFFFF;
        std::cout << test_get << std::endl;

	}

	static void TearDownTestCase() {
	}

	virtual void SetUp() {

	}

	virtual void TearDown() {
	}

    void SetNewXip(XipParser& xip_parser) {
        xip_parser.set_xnetwork_id(1945745);
        ASSERT_EQ(xip_parser.xnetwork_id(), 1945745);

        xip_parser.set_xnetwork_id(345234);
        ASSERT_EQ(xip_parser.xnetwork_id(), 345234);

        xip_parser.set_xnetwork_version(190);
        ASSERT_EQ(xip_parser.xnetwork_version(), 190);

        xip_parser.set_xnetwork_version(45);
        ASSERT_EQ(xip_parser.xnetwork_version(), 45);


        xip_parser.set_xinterface_id(345345345);
        ASSERT_EQ(xip_parser.xinterface_id(), 345345345);
        xip_parser.set_xinterface_id(5464);
        ASSERT_EQ(xip_parser.xinterface_id(), 5464);

        xip_parser.set_zone_id(65);
        ASSERT_EQ(xip_parser.zone_id(), 65);
        xip_parser.set_zone_id(45);
        ASSERT_EQ(xip_parser.zone_id(), 45);

        xip_parser.set_network_type(31);
        ASSERT_EQ(xip_parser.network_type(), 31);
        xip_parser.set_network_type(5);
        ASSERT_EQ(xip_parser.network_type(), 5);

        xip_parser.set_xaddress_domain_xip(1);
        ASSERT_EQ(xip_parser.xaddress_domain_xip(), 1);
        xip_parser.set_xaddress_domain_xip(0);
        ASSERT_EQ(xip_parser.xaddress_domain_xip(), 0);


        xip_parser.set_xip_type(2);
        ASSERT_EQ(xip_parser.xip_type(), 2);
        xip_parser.set_xip_type(1);
        ASSERT_EQ(xip_parser.xip_type(), 1);

        xip_parser.set_cluster_id(69);
        ASSERT_EQ(xip_parser.cluster_id(), 69);
        xip_parser.set_cluster_id(127);
        ASSERT_EQ(xip_parser.cluster_id(), 127);

        xip_parser.set_group_id(250);
        ASSERT_EQ(xip_parser.group_id(), 250);
        xip_parser.set_group_id(178);
        ASSERT_EQ(xip_parser.group_id(), 178);

        xip_parser.set_node_id(250);
        ASSERT_EQ(xip_parser.node_id(), 250);
        xip_parser.set_node_id(198);
        ASSERT_EQ(xip_parser.node_id(), 198);


        xip_parser.set_process_id(15);
        ASSERT_EQ(xip_parser.process_id(), 15);
        xip_parser.set_process_id(9);
        ASSERT_EQ(xip_parser.process_id(), 9);

        xip_parser.set_router_id(15);
        ASSERT_EQ(xip_parser.router_id(), 15);
        xip_parser.set_router_id(7);
        ASSERT_EQ(xip_parser.router_id(), 7);

        xip_parser.set_switch_id(250);
        ASSERT_EQ(xip_parser.switch_id(), 250);
        xip_parser.set_switch_id(172);
        ASSERT_EQ(xip_parser.switch_id(), 172);

        xip_parser.set_local_id(250);
        ASSERT_EQ(xip_parser.local_id(), 250);
        xip_parser.set_local_id(189);
        ASSERT_EQ(xip_parser.local_id(), 189);

    }

    void CheckXip(const XipParser& xip_parser) {
        ASSERT_EQ(xip_parser.xnetwork_id(), 1945745);
        ASSERT_EQ(xip_parser.xnetwork_version(), 190);
        ASSERT_EQ(xip_parser.xinterface_id(), 345345345);
        ASSERT_EQ(xip_parser.zone_id(), 182);
        ASSERT_EQ(xip_parser.network_type(), 31);
        ASSERT_EQ(xip_parser.xaddress_domain_xip(), 1);
        ASSERT_EQ(xip_parser.xip_type(), 2);
        ASSERT_EQ(xip_parser.cluster_id(), 250);
        ASSERT_EQ(xip_parser.group_id(), 250);
        ASSERT_EQ(xip_parser.node_id(), 250);
        ASSERT_EQ(xip_parser.process_id(), 15);
        ASSERT_EQ(xip_parser.router_id(), 15);
        ASSERT_EQ(xip_parser.switch_id(), 250);
        ASSERT_EQ(xip_parser.local_id(), 250);
    }
};

TEST_F(TestXipParser, SetGet) {
    XipParser xip_parser;
    SetNewXip(xip_parser);
}

TEST_F(TestXipParser, xip) {
    XipParser xip_parser;
    SetNewXip(xip_parser);
    std::string src_str_xip = xip_parser.xip();
    ASSERT_EQ(src_str_xip.size(), 16);
    XipParser str_xip_parser(src_str_xip);
    std::string des_str_xip = str_xip_parser.xip();
    ASSERT_EQ(des_str_xip, src_str_xip);

    uint64_t src_high = 0;
    uint64_t src_low = 0;
    xip_parser.xip(src_high, src_low);
    uint64_t des_high = 0;
    uint64_t des_low = 0;
    str_xip_parser.xip(des_high, des_low);
    ASSERT_EQ(src_high, des_high);
    ASSERT_EQ(src_low, des_low);

    XipParser u64_xip_parser(des_high, des_low);
    std::string u64_str = u64_xip_parser.xip();
    ASSERT_EQ(u64_str, src_str_xip);
    uint64_t u64_high = 0;
    uint64_t u64_low = 0;
    u64_xip_parser.xip(u64_high, u64_low);
    ASSERT_EQ(src_high, u64_high);
    ASSERT_EQ(src_low, u64_low);
}

TEST_F(TestXipParser, ServerId) {
    XipParser xip_parser;
    SetNewXip(xip_parser);
    xip_parser.set_server_id(16777214);
    ASSERT_EQ(xip_parser.server_id(), 16777214);

    std::string src_str_xip = xip_parser.xip();
    ASSERT_EQ(src_str_xip.size(), 16);
    XipParser str_xip_parser(src_str_xip);
    std::string des_str_xip = str_xip_parser.xip();
    ASSERT_EQ(des_str_xip, src_str_xip);

    uint64_t src_high = 0;
    uint64_t src_low = 0;
    xip_parser.xip(src_high, src_low);
    uint64_t des_high = 0;
    uint64_t des_low = 0;
    str_xip_parser.xip(des_high, des_low);
    ASSERT_EQ(src_high, des_high);
    ASSERT_EQ(src_low, des_low);

    XipParser u64_xip_parser(des_high, des_low);
    std::string u64_str = u64_xip_parser.xip();
    ASSERT_EQ(u64_str, src_str_xip);
    uint64_t u64_high = 0;
    uint64_t u64_low = 0;
    u64_xip_parser.xip(u64_high, u64_low);
    ASSERT_EQ(src_high, u64_high);
    ASSERT_EQ(src_low, u64_low);


    uint64_t test_num = 0;
    test_num = test_num | (((uint64_t)(1928)) << 32);
    uint64_t test_get = test_num >> 32 & 0xFFFFFF;
    std::cout << test_get << std::endl;
    test_num = test_num & (((uint64_t)(0xF000000)) << 32);
    test_num = test_num | (((uint64_t)(5464)) << 32);
    test_get = test_num >> 32 & 0xFFFFFF;
    std::cout << test_get << std::endl;
    test_num = test_num & (((uint64_t)(0xF000000)) << 32);
    test_num = test_num | (((uint64_t)(234235)) << 32);
    test_get = test_num >> 32 & 0xFFFFFF;
    std::cout << test_get << std::endl;

    xip_parser.set_server_id(0);
    ASSERT_EQ(xip_parser.server_id(), 0);
}

}  // namespace test

}  // namespace kadmlia

}  // namespace top
