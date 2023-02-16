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
#include "tests/xvnetwork/xdummy_vhost.h"

using namespace top::contract;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::tests::vnetwork;
// using namespace top::consensus_service;
using namespace top::mbus;
using namespace top::base;

extern xvnode_address_t
make_address(int zone_id, int cluster_id, int group_id);

extern xblock_ptr_t test_create_timer_block(const std::string& owner, uint64_t time_no);

class test_contract1 : public xcontract_base {
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(test_contract1);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(test_contract1);

    explicit
    test_contract1(top::common::xnetwork_id_t const & network_id) : xcontract_base{ network_id } {
    }

    xcontract_base* clone() override {return new test_contract1(network_id());}

    virtual void setup() {
        setup_called = true;
    }

    void on_timer(top::common::xlogic_time_t const) {
        timer_called = true;
    }

    BEGIN_CONTRACT_WITH_PARAM(test_contract1)
    CONTRACT_FUNCTION_PARAM(test_contract1, on_timer)
    END_CONTRACT_WITH_PARAM

    bool setup_called{false};
    bool timer_called{false};
};

class test_xcontract_manager_vnetwork_driver_t : public xtop_dummy_vnetwork_driver {
public:

    void
    register_message_ready_notify(common::xmessage_category_t const message_category,
                                  xvnetwork_message_ready_callback_t cb) {
        callback = cb;
        registered++;
    }

    void
    unregister_message_ready_notify(common::xmessage_category_t const message_category) {
        registered--;
    }

    xnode_type_t
    type() const noexcept override {
        return real_part_type(node_address.cluster_address().type());
    }

    xvnode_address_t
    address() const {
        return node_address;
    }

    std::vector<std::uint16_t>
    table_ids() const override {
        return {1, 2};
    }

    int registered{0};
    xvnetwork_message_ready_callback_t callback{};
    xvnode_address_t node_address;
};

// class test_xcontract_manager_us_t : public xconsensus_mock {
// public:
//     int32_t  request_transaction_consensus(const data::xtransaction_ptr_t & trans, bool local = false) override {
//         xvm::xvm_service s;
//         xaccount_context_t ac(trans->get_target_addr(), m_store, {});
//         auto trace = s.deal_transaction(trans, &ac);
//         return 0;
//     }

//     store::xstore_face_t* m_store{};
// };

class test_xcontract_manager_store_t : public xstore_face_mock_t {
public:
    int32_t set_sync_block(const data::xblock_ptr_t & block) {
        block_get = true;
        return 0;
    }

    bool block_get{false};
};

class test_xcontract_vhost_t : public tests::vnetwork::xdummy_vhost_t {
public:
    common::xnetwork_id_t const &
    network_id() const noexcept override {
        static common::xnetwork_id_t nid{1};
        return nid;
    }
};

// TEST(xcontract_manager_t, tests) {
//     auto bus = std::make_shared<xmessage_bus_t>();
//     auto us = make_object_ptr<test_xcontract_manager_us_t>();
//     auto driver = std::make_shared<test_xcontract_manager_vnetwork_driver_t>();
//     auto driver1 = std::make_shared<test_xcontract_manager_vnetwork_driver_t>();
//     std::map<std::string, std::string> prop_list;

//     driver->node_address = make_address(0, 0, 0);
//     driver1->node_address = make_address(0, 0, 0);

//     auto store_ptr = store::xstore_factory::create_store_with_memdb(bus);
//     test_datamock_t dm(store_ptr.get());
//     auto block_ptr = dm.create_unit("test.block", prop_list);
//     xstream_t stream(xcontext_t::instance());
//     block_ptr->serialize_to(stream);

//     us->m_store = store_ptr.get();

//     xcontract_deploy_t::instance().clear();

//     //xcontract_manager_t::instance().start();
//     // 1

//     test_xcontract_vhost_t vhost;
//     std::shared_ptr<vnetwork::xmessage_callback_hub_t> nt_callback_manager = std::make_shared<vnetwork::xmessage_callback_hub_t>(make_observer(&vhost));
//     xcontract_manager_t::instance().install_monitors(make_observer(bus.get()), make_observer(nt_callback_manager.get()), make_observer(store_ptr));
//     xcontract_manager_t::instance().push_event(make_shared<xevent_vnode_t>(false, us, driver));
//     ASSERT_TRUE(xcontract_manager_t::instance().get_map().empty());
//     xcontract_manager_t::instance().push_event(make_shared<xevent_vnode_t>(true, us, driver));
//     ASSERT_TRUE(xcontract_manager_t::instance().get_map().empty());

//     // 2
//     xcontract_deploy_t::instance().deploy(
//     top::common::xaccount_address_t{"T-x-test10"},
//     top::common::xnode_type_t::committee | top::common::xnode_type_t::zec);

//     xcontract_manager_t::instance().register_contract<test_contract1>(top::common::xaccount_address_t{"T-x-test10"}, top::common::xtopchain_network_id);
//     xcontract_manager_t::instance().push_event(make_shared<xevent_vnode_t>(false, us, driver));
//     ASSERT_TRUE(xcontract_manager_t::instance().get_map().size() == 1);
//     ASSERT_TRUE(xcontract_manager_t::instance().get_contract(top::common::xaccount_address_t{"T-x-test10"}) != nullptr);
//     ASSERT_TRUE(xcontract_manager_t::instance().get_contract(top::common::xaccount_address_t{"T"}) == nullptr);
//     test_contract1* tc = (test_contract1*) xcontract_manager_t::instance().get_contract(top::common::xaccount_address_t{"T-x-test10"});
//     ASSERT_TRUE(tc != nullptr);
//     //ASSERT_TRUE(tc->setup_called); // commented due to cloned

//     xcontract_manager_t::instance().push_event(make_shared<xevent_vnode_t>(true, us, driver));
//     ASSERT_TRUE(xcontract_manager_t::instance().get_map().empty());

//     xcontract_deploy_t::instance().deploy(
//     top::common::xaccount_address_t{"T-x-test1"},
//     top::common::xnode_type_t::committee | top::common::xnode_type_t::zec,
//     "",
//     enum_broadcast_policy_t::normal,
//     "T-x-topsystemtimer,on_timer,NC,100");
//     xcontract_manager_t::instance().register_contract<test_contract1>(top::common::xaccount_address_t{"T-x-test1"}, top::common::xtopchain_network_id);

//     xcontract_deploy_t::instance().deploy(
//     top::common::xaccount_address_t{"T-x-test2"},
//     top::common::xnode_type_t::committee | top::common::xnode_type_t::zec,
//     "",
//     enum_broadcast_policy_t::normal,
//     "T-x-topsystemtimer,on_timer,NC,100");
//     xcontract_manager_t::instance().register_contract<test_contract1>(top::common::xaccount_address_t{"T-x-test2"}, top::common::xtopchain_network_id);

//     xcontract_manager_t::instance().push_event(make_shared<xevent_vnode_t>(false, us, driver));
//     test_contract1* tc1 = (test_contract1*) xcontract_manager_t::instance().get_contract(top::common::xaccount_address_t{"T-x-test1"});
//     test_contract1* tc2 = (test_contract1*) xcontract_manager_t::instance().get_contract(top::common::xaccount_address_t{"T-x-test2"});
//     //ASSERT_TRUE(tc->setup_called);
//     //ASSERT_TRUE(tc1->setup_called);
//     //ASSERT_TRUE(tc2->setup_called);

//     // xcontract_manager_t::instance().push_event(std::make_shared<xevent_store_block_to_db_t>(
//     //     test_create_timer_block("T-x-topsystemtimer", 10), "T-x-topsystemtimer", false
//     // ));
//     // std::this_thread::sleep_for(std::chrono::seconds(1));
//     //ASSERT_FALSE(tc->timer_called);
//     //ASSERT_TRUE(tc1->timer_called);
//     //ASSERT_TRUE(tc2->timer_called);

//     // test auditor creates validator contracts interfaces
//     xcontract_manager_t::instance().clear();
//     xcontract_deploy_t::instance().clear();
//     // xcontract_deploy_t::instance().deploy("T-s-test1",  top::common::xnode_type_t::consensus_validator);
//     // xcontract_deploy_t::instance().deploy("T-s-test2",  top::common::xnode_type_t::consensus_validator);
//     // xcontract_manager_t::instance().register_contract<test_contract1>("T-s-test1", top::common::xtopchain_network_id);
//     // xcontract_manager_t::instance().register_contract<test_contract1>("T-s-test2", top::common::xtopchain_network_id);
//     // driver->node_address = make_address(1, 1, 1);

//     // xcontract_manager_t::instance().push_event(make_shared<xevent_vnode_t>(false, us, driver));
//     // ASSERT_TRUE(xcontract_manager_t::instance().get_contract_inst_map()->size() == 2048);
//     // ASSERT_TRUE(xcontract_manager_t::instance().get_map()->size() == 2);

//     // xcontract_manager_t::instance().push_event(make_shared<xevent_vnode_t>(false, us, driver));
//     // ASSERT_TRUE(xcontract_manager_t::instance().get_contract_inst_map()->size() == 2048);

//     // xcontract_manager_t::instance().push_event(make_shared<xevent_vnode_t>(true, us, driver));
// }
