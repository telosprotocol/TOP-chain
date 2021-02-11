// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#include <iostream>
#include <algorithm>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/xid/xid_def.h"
#include "xpbase/base/xid/xid_generator.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#define private public
#include "xtransport/udp_transport/udp_transport.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/register_routing_table.h"
#include "xgossip/include/gossip_bloomfilter.h"
#include "xgossip/include/gossip_utils.h"
#include "xgossip/include/header_block_data.h"

namespace top {

using namespace kadmlia;

namespace gossip {

namespace test {

class TestHeaderBlockDataCache : public testing::Test {
public:
    static void SetUpTestCase() {
        header_block_data_ = std::make_shared<HeaderBlockDataCache>();
        ASSERT_TRUE(header_block_data_);
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
        ASSERT_TRUE(header_block_data_);
    }

    virtual void TearDown() {
    }

    static  std::shared_ptr<HeaderBlockDataCache> header_block_data_;

};

std::shared_ptr<HeaderBlockDataCache> TestHeaderBlockDataCache::header_block_data_ = nullptr;

TEST_F(TestHeaderBlockDataCache, ALLAPI) {
    std::string fhash = "hash";
    std::string fdata = RandomString(500);
    header_block_data_->AddData(fhash, fdata);
    ASSERT_TRUE(header_block_data_->HasData(fhash));

    for (uint32_t i = 0; i < 100; ++i) {
        std::string hash = "hash_" + std::to_string(i);
        std::string data = RandomString(500);
        header_block_data_->AddData(hash, data);

        ASSERT_TRUE(header_block_data_->HasData(hash));
        std::string block;
        header_block_data_->GetData(hash, block);
        ASSERT_EQ(block, data);

        header_block_data_->RemoveData(hash);
        block.clear();
        header_block_data_->GetData(hash, block);
        ASSERT_NE(block, data);
        ASSERT_TRUE(block.empty());
    }

    std::cout << "wait timer ... will sleep 5 s" << std::endl;
    //SleepMs(5 * 1000);  // sleep 11 s beyond 10s, wait timer to clear
    ASSERT_TRUE(header_block_data_->HasData(fhash));
}




}  // namespace test

}  // namespace gossip

}  // namespace top
