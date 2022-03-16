#include "xbasic/xutility.h"
#include "xchain_upgrade/xchain_data_processor.hnter.h"
#include "xcommon/xaddress.h"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xserialization/xserialization.h"

#include <gtest/gtest.h>

using namespace top;

TEST(test_reset_data, stake_property)
{
    using namespace top::xstake;
    using common::xaccount_address_t;

    /*
    #101 size: 700
    #105 size: 5
    #132 size: 10
    */

    std::vector<std::pair<std::string, std::string>> db_kv_101;
    chain_reset::xchain_reset_center_t::get_reset_stake_map_property(xaccount_address_t(sys_contract_rec_registration_addr), XPORPERTY_CONTRACT_REG_KEY, db_kv_101);
    EXPECT_TRUE(db_kv_101.size() == 700);
    for (auto const &_p : db_kv_101)
    {
        EXPECT_TRUE(_p.first[0] == 'T');
        // MAP_SET(XPORPERTY_CONTRACT_REG_KEY, _p.first, _p.second);
    }

    std::vector<std::pair<std::string, std::string>> db_kv_105;
    chain_reset::xchain_reset_center_t::get_reset_stake_map_property(xaccount_address_t(sys_contract_rec_registration_addr), XPORPERTY_CONTRACT_TICKETS_KEY, db_kv_105);
    EXPECT_TRUE(db_kv_105.size() == 5);
    for (auto const &_p : db_kv_105)
    {
        EXPECT_TRUE(_p.first[0] == 'T');
        // MAP_SET(XPORPERTY_CONTRACT_TICKETS_KEY, _p.first, _p.second);
    }

    std::vector<std::pair<std::string, std::string>> db_kv_132;
    chain_reset::xchain_reset_center_t::get_reset_stake_map_property(xaccount_address_t(sys_contract_rec_registration_addr), XPROPERTY_CONTRACT_SLASH_INFO_KEY, db_kv_132);
    EXPECT_TRUE(db_kv_132.size() == 10);
    for (auto const &_p : db_kv_132)
    {
        EXPECT_TRUE(_p.first[0] == 'T');
        // MAP_SET(XPROPERTY_CONTRACT_SLASH_INFO_KEY, _p.first, _p.second);
    }
}

TEST(test_reset_data, native_propoerty_standby_pool)
{
    using common::xaccount_address_t;
    using common::xnode_id_t;
    using data::election::xstandby_node_info_t;
    using data::election::xstandby_result_store_t;

    std::string p41;
    chain_reset::xchain_reset_center_t::get_reset_property(xaccount_address_t(sys_contract_rec_standby_pool_addr), XPROPERTY_CONTRACT_STANDBYS_KEY, p41);

    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)p41.data(), p41.size());
    base::xdataunit_t *obj = base::xdataunit_t::read_from(stream);

    std::string after_msg_pack_string;
    ((base::xstring_t *)obj)->get(after_msg_pack_string);
    obj->release_ref();

    xstandby_result_store_t standby_result_store = xvm::serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(after_msg_pack_string);

    EXPECT_EQ(standby_result_store.size(common::xnetwork_id_t{0}), 699);
    EXPECT_TRUE(standby_result_store.result_of(common::xnetwork_id_t{0}).activated_state());

    for (auto _p : standby_result_store.result_of(common::xnetwork_id_t{0}))
    {
        auto const &node_id = top::get<common::xnode_id_t const>(_p);
        // auto const &standby_node_info = top::get<xstandby_node_info_t>(_p);
        std::string node_id_str = node_id.to_string();
        EXPECT_TRUE(node_id_str.find("T00000") == 0);
    }
}