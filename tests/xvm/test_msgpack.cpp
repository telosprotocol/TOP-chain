#include "gtest/gtest.h"
#include "xdata/xtransaction.h"

#include "xdata/xaction.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract.h"
#include "xbase/xutl.h"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xgenesis_data.h"
#include "xvm/xserialization/xserialization.h"

using top::data::election::xstandby_result_store_t;
using top::data::election::xstandby_node_info_t;

using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::xvm;
using namespace top::xvm::xcontract;
// using namespace top::elect;

class xtest_msgpack : public testing::Test {
public:
    void SetUp() override {
    }
    void TearDown() override {

    }
public:
    top::xvm::system_contracts::rec::xrec_standby_pool_contract_t add_node_contract{ top::common::xtopchain_network_id };
};

TEST_F(xtest_msgpack, msgpack_big_size) {
    xstandby_result_store_t standby_result_store;

    for (int i = 0; i < 1000000; ++i) {
        xstandby_node_info_t new_node_info;
#if defined (XENABLE_MOCK_ZEC_STAKE)
        new_node_info.user_request_role = common::xminer_type_t::advance;
#endif
        new_node_info.consensus_public_key = xpublic_key_t{};
        common::xnode_id_t xnode_id{"T-00000000000000000000000000000" + std::to_string(i)};
        new_node_info.stake_container.insert({common::xnode_type_t::rec, 0});
        new_node_info.stake_container.insert({common::xnode_type_t::zec, 0});
        new_node_info.stake_container.insert({common::xnode_type_t::archive, 0});
        new_node_info.stake_container.insert({common::xnode_type_t::consensus_auditor, 0});
        new_node_info.stake_container.insert({common::xnode_type_t::consensus_validator, 0});
        new_node_info.stake_container.insert({common::xnode_type_t::edge, 0});
        common::xnode_type_t node_type = common::xnode_type_t::consensus_auditor | common::xnode_type_t::rec | common::xnode_type_t::zec | common::xnode_type_t::archive |
                                         common::xnode_type_t::edge | common::xnode_type_t::consensus_validator;
        standby_result_store.result_of(common::xtopchain_network_id).insert({xnode_id, new_node_info});

        common::xnode_id_t xnode_id2{"T-00000000000000000000000000001" + std::to_string(i)};
#if defined(XENABLE_MOCK_ZEC_STAKE)
        new_node_info.user_request_role = common::xminer_type_t::edge;
#endif
        standby_result_store.result_of(common::xtopchain_network_id).insert({xnode_id2, new_node_info});

        common::xnode_id_t xnode_id3{"T-00000000000000000000000000002" + std::to_string(i)};
#if defined(XENABLE_MOCK_ZEC_STAKE)
        new_node_info.user_request_role = common::xminer_type_t::validator;
#endif
        standby_result_store.result_of(common::xtopchain_network_id).insert({xnode_id3, new_node_info});
    }

    auto bytes = codec::msgpack_encode(standby_result_store);
    std::string obj_str{ std::begin(bytes), std::end(bytes) };
    printf("msgpack_size:%lu\n", obj_str.size());
    EXPECT_TRUE(obj_str.size() >= 2*1024*1024);
    auto standby_result_store2 = codec::msgpack_decode<xstandby_result_store_t>({ std::begin(bytes), std::end(bytes) });
    EXPECT_TRUE(true);
}
