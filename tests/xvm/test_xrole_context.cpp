#include <gtest/gtest.h>
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"
// #include "xunit/test/vconsensus_mock.h"
#include "tests/xvnetwork/xdummy_vnetwork_driver.h"
#include "xstore/test/xstore_face_mock.h"
#include "xstore/test/test_datamock.hpp"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xcommon/xmessage_id.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xmbus/xevent_store.h"
#include "xdata/xblock.h"

using namespace top::contract;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::tests::vnetwork;
// using namespace top::consensus_service;
using namespace top::mbus;
using namespace top::base;
using namespace top::data;

extern xvnode_address_t
make_address(int zone_id, int cluster_id, int group_id);

// xblock_ptr_t test_create_block(const std::string& owner) {
//     xblock_ptr_t block = make_object_ptr<data::xblock_t>((top::base::xdataunit_t::enum_xdata_type) 0, owner, 0);
//     block->get_block_header()->get_signature_data().validator_first_round_sign().leader_addr("account_addr");
//     block->set_prev_height(1);
//     return block;
// }

class test_contract2 : public xcontract_base {
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(test_contract2);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(test_contract2);
    explicit
    test_contract2(top::common::xnetwork_id_t const & network_id) : xcontract_base{ network_id } {

    }
    xcontract_base* clone() override {return new test_contract2(network_id());}
    virtual void setup() {
        setup_called = true;
    }

    void on_timer(common::xlogic_time_t const) {
        timer_called = true;
    }

    // void on_block(const xblock_ptr_t& block_ptr) {
    //     block_called = true;
    // }

    BEGIN_CONTRACT_WITH_PARAM(test_contract2)
    CONTRACT_FUNCTION_PARAM(test_contract2, on_timer)
    // CONTRACT_FUNCTION_PARAM(test_contract2, on_block)
    END_CONTRACT_WITH_PARAM

    bool setup_called{false};
    bool timer_called{false};
    bool block_called{false};
};

class test_xrole_context_vnetwork_driver_t : public xtop_dummy_vnetwork_driver {
public:

    void
    register_message_ready_notify(common::xmessage_category_t const message_category,
                                  xvnetwork_message_ready_callback_t cb) {
        callback = cb;
        registered = true;
    }

    common::xnetwork_id_t
    network_id() const noexcept override {
        return node_address.network_id();
    }

    void
    unregister_message_ready_notify(common::xmessage_category_t const message_category) {
        registered = false;
    }

    xnode_type_t
    type() const noexcept override {
        return real_part_type(node_address.cluster_address().type());
    }

    xvnode_address_t
    address() const {
        return node_address;
    }

    void
    forward_broadcast_message(xmessage_t const &,
                              xvnode_address_t const & dest) override {
        dest_address = dest;
        send_times++;
    }

    void
    broadcast_to(xvnode_address_t const & dst, xmessage_t const & message) {
        dest_address = dst;
        send_times++;
    }

    bool registered{false};
    xvnetwork_message_ready_callback_t callback{};
    xvnode_address_t node_address;
    xvnode_address_t dest_address;
    int send_times{};
};

// class test_xrole_context_us_t : public xconsensus_mock {
// public:
//     int32_t  request_transaction_consensus(const data::xtransaction_ptr_t & trans, bool local = false) override {
//         xvm::xvm_service s;
//         xaccount_context_t ac(trans->get_target_addr(), m_store, {});
//         auto trace = s.deal_transaction(trans, &ac);
//         return 0;
//     }

//     store::xstore_face_t* m_store;
// };

class test_xrole_context_store_t : public xstore_face_mock_t {
public:
    int32_t set_sync_block(const data::xblock_ptr_t & block) {
        block_get = true;
        return 0;
    }

    bool block_get{false};
};

// TEST(xrole_context_t, invalid) {
//     auto driver = std::make_shared<test_xrole_context_vnetwork_driver_t>();
//     driver->node_address = make_address(0, 0, 0);

//     xcontract_info_t* pinfo = new xcontract_info_t(top::common::xaccount_address_t{"T-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::invalid);
//     xrole_context_t rc(nullptr, nullptr, driver, pinfo);
//     rc.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_block("owner"), "owner", false));

//     pinfo->add_block_monitor(top::common::xaccount_address_t{"beacon"}, "on_block", enum_call_action_way_t::consensus);
// }

// TEST(xrole_context_t, broadcast) {
//     auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
//     auto driver = std::make_shared<test_xrole_context_vnetwork_driver_t>();
//     driver->node_address = make_address(0, 0, 0);
//     driver->dest_address = make_address(111, 11, 11);

//     // not new block
//     xcontract_info_t* pinfo = new xcontract_info_t(top::common::xaccount_address_t{"T-s-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::all);
//     xrole_context_t rc(make_observer(store_ptr), nullptr, driver, pinfo);
//     rc.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_block("T-s-test"), "T-s-test", false));
//     ASSERT_TRUE(driver->dest_address == make_address(111, 11, 11));

//     // not monitor block
//     xcontract_info_t* pinfo1 = new xcontract_info_t(top::common::xaccount_address_t{"T-s-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::all);
//     xrole_context_t rc1(make_observer(store_ptr), nullptr, driver, pinfo1);
//     rc1.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_block("any"), "any", true));
//     ASSERT_TRUE(driver->dest_address == make_address(111, 11, 11));

//     // all
//     xcontract_info_t* pinfo2 = new xcontract_info_t(top::common::xaccount_address_t{"T-s-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::all);
//     pinfo2->broadcast_policy = enum_broadcast_policy_t::normal;
//     xrole_context_t rc2(make_observer(store_ptr), nullptr, driver, pinfo2);
//     rc2.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_block("T-s-test"), "T-s-test", true));
//     xvnode_address_t all_addr{xcluster_address_t{driver->node_address.network_id()}};
//     ASSERT_TRUE(driver->dest_address == all_addr);
//     ASSERT_TRUE(driver->send_times == 1);

//     // beacon / archive / zec
//     xcontract_info_t* pinfo3 = new xcontract_info_t(top::common::xaccount_address_t{"T-s-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::archive);
//     pinfo3->broadcast_policy = enum_broadcast_policy_t::normal;
//     xrole_context_t rc3(make_observer(store_ptr), nullptr, driver, pinfo3);
//     rc3.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_block("T-s-test"), "T-s-test", true));
//     common::xnode_address_t arc_addr{
//         common::build_archive_sharding_address(driver->dest_address.network_id())
//     };
//     ASSERT_TRUE(driver->dest_address == arc_addr);
//     ASSERT_TRUE(driver->send_times == 2);

//     xcontract_info_t* pinfo4 = new xcontract_info_t(top::common::xaccount_address_t{"T-s-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::zec);
//     pinfo4->broadcast_policy = enum_broadcast_policy_t::normal;
//     xrole_context_t rc4(make_observer(store_ptr), nullptr, driver, pinfo4);
//     rc4.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_block("T-s-test"), "T-s-test", true));
//     common::xnode_address_t zec_addr{
//         common::build_zec_sharding_address(driver->node_address.network_id())
//     };
//     ASSERT_TRUE(driver->dest_address == zec_addr);
//     ASSERT_TRUE(driver->send_times == 3);

//     xcontract_info_t* pinfo5 = new xcontract_info_t(top::common::xaccount_address_t{"T-s-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::committee);
//     pinfo5->broadcast_policy = enum_broadcast_policy_t::normal;
//     xrole_context_t rc5(make_observer(store_ptr), nullptr, driver, pinfo5);
//     rc5.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_block("T-s-test"), "T-s-test", true));
//     ASSERT_TRUE(driver->dest_address == xvnode_address_t{common::build_committee_sharding_address()});
//     ASSERT_TRUE(driver->send_times == 4);
// }

// TEST(xrole_context_t, call_contract) {
//     std::map<std::string, std::string> prop_list;
//     auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
//     test_datamock_t dm(store_ptr.get());
//     dm.create_unit("T-test", prop_list);

//     auto driver = std::make_shared<test_xrole_context_vnetwork_driver_t>();
//     driver->node_address = make_address(0, 0, 0);

//     xcontract_manager_t::instance().clear();
//     xcontract_manager_t::instance().register_contract<test_contract2>(top::common::xaccount_address_t{"T-test"}, top::common::xtopchain_network_id);
//     xcontract_manager_t::instance().register_contract_cluster_address(top::common::xaccount_address_t{"T-test"}, top::common::xaccount_address_t{"T-test"});
//     test_contract2* c = (test_contract2*) xcontract_manager_t::instance().get_contract(top::common::xaccount_address_t{"T-test"});

//     // no US
//     xcontract_info_t* pinfo = new xcontract_info_t(top::common::xaccount_address_t{"T-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::all);
//     pinfo->add_block_monitor(top::common::xaccount_address_t{"block_src_addr"}, "on_block", enum_call_action_way_t::direct);
//     xrole_context_t rc(make_observer(store_ptr), nullptr, driver, pinfo);
//     rc.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_block("block_src_addr"), "block_src_addr", false));
//     // ASSERT_TRUE(c->block_called); // commented due to cloned

//     // consensus
//     xcontract_info_t* pinfo1 = new xcontract_info_t(top::common::xaccount_address_t{"T-test"}, top::common::xnode_type_t::committee, top::common::xnode_type_t::all);
//     pinfo1->add_timer_monitor(top::common::xaccount_address_t{"chain_timer_addr"}, "on_timer", enum_call_action_way_t::consensus, 10, "");
//     auto us = make_object_ptr<test_xrole_context_us_t>();
//     us->m_store = store_ptr.get();
//     xrole_context_t rc1(make_observer(store_ptr), us, driver, pinfo1);
//     // rc1.on_block(std::make_shared<xevent_store_block_to_db_t>(test_create_timer_block("chain_timer_addr", 10), "chain_timer_addr", false));
//     //ASSERT_TRUE(c->timer_called);
// }
