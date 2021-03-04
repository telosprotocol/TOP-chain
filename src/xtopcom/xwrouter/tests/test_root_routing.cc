// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string.h>

#include <string>

#include <gtest/gtest.h>

#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#define protected public
#define private public
#include "xtransport/udp_transport/udp_transport.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/root/root_routing.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xwrouter/message_handler/wrouter_message_handler.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xkad/nat_detect/nat_manager_intf.h"

namespace top {

namespace kadmlia {

namespace test {

class TestRootRouting : public testing::Test {
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

}  // namespace test

}  // namespace kadmlia

}  // namespace top
