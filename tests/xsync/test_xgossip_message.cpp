#include <gtest/gtest.h>
#include <map>
#include <set>
#include <list>
#include <memory>
#include "xsync/xgossip_message.h"
#include "xdata/xgenesis_data.h"

using namespace top;
using namespace top::data;
using namespace top::sync;
using namespace top::vnetwork;

TEST(xgossip_message, test) {
    xgossip_message_t gm;

    std::vector<xgossip_chain_info_ptr_t> info_list;
    xbyte_buffer_t bloom_data;

    // empty info_list && empty bloom_data
    auto payload = gm.create_payload(info_list, bloom_data);
    gm.parse_payload(payload, info_list, bloom_data);
    ASSERT_TRUE(info_list.empty());
    ASSERT_TRUE(bloom_data.empty());

    // empty info_list && bloom_data
    for(int i=0;i<13;i++) {
        bloom_data.push_back(i);
    }
    payload = gm.create_payload(info_list, bloom_data);
    bloom_data.clear();
    gm.parse_payload(payload, info_list, bloom_data);
    ASSERT_TRUE(info_list.empty());
    ASSERT_TRUE(bloom_data.size() == 13);
    for(int i=0;i<13;i++) {
        ASSERT_TRUE(i == bloom_data[i]);
    }

    std::string table_addr = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 0);

    // info_list && empty bloom_data
    bloom_data.clear();
    xgossip_chain_info_t infos[] = {
        {sys_contract_rec_elect_rec_addr, 11},
        {sys_contract_rec_elect_edge_addr, 11},
        //{sys_contract_sharding_vote_addr "-0001", 11},
        {table_addr, 11},
        {"dun-sys-contract-addr", 11},
    };
    for(auto& _info : infos) {
        std::shared_ptr<xgossip_chain_info_t> info = std::make_shared<xgossip_chain_info_t>();
        info->owner = _info.owner;
        info->max_height = _info.max_height;
        info_list.push_back(info);
    }

    payload = gm.create_payload(info_list, bloom_data);
    info_list.clear();
    gm.parse_payload(payload, info_list, bloom_data);
    ASSERT_TRUE(info_list.size() == 3);

    std::set<uint32_t> indexes;
    for(auto& info : info_list) {
        uint32_t index = -1;
        ASSERT_TRUE(info->owner != "un-sys-contract-addr");
        for(uint32_t i=0;i<4;i++) {
            if(info->owner == infos[i].owner) {
                index = i;
                break;
            }
        }

        ASSERT_TRUE(index != (uint32_t) -1) << info->owner << " " << info->max_height;
        ASSERT_TRUE(indexes.find(index) == indexes.end());
        indexes.insert(index);
        ASSERT_TRUE(info->owner == infos[index].owner);
        ASSERT_TRUE(info->max_height == infos[index].max_height);
    }

    // inf_list && bloom_data
    info_list.clear();
    bloom_data.clear();
    for(int i=0;i<13;i++) {
        bloom_data.push_back(i);
    }
    for(int i=0;i<4;i++) {
        std::shared_ptr<xgossip_chain_info_t> info = std::make_shared<xgossip_chain_info_t>();
        info->owner = infos[i].owner;
        info->max_height = infos[i].max_height;
        info_list.push_back(info);
    }

    payload = gm.create_payload(info_list, bloom_data);
    info_list.clear();
    bloom_data.clear();
    indexes.clear();
    gm.parse_payload(payload, info_list, bloom_data);

    ASSERT_TRUE(info_list.size() == 3);
    for(auto& info : info_list) {
        std::cout << info->owner.c_str() << std::endl;
        uint32_t index = -1;
        ASSERT_TRUE(info->owner != "un-sys-contract-addr");
        for(uint32_t i=0;i<4;i++) {
            if(info->owner == infos[i].owner) {
                index = i;
                break;
            }
        }
        ASSERT_TRUE(index != (uint32_t) -1);
        ASSERT_TRUE(indexes.find(index) == indexes.end());
        indexes.insert(index);
        ASSERT_TRUE(info->owner == infos[index].owner);
        ASSERT_TRUE(info->max_height == infos[index].max_height);
    }

    for(int i=0;i<13;i++) {
        ASSERT_TRUE(i == bloom_data[i]);
    }
}
