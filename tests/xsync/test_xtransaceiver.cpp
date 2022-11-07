#include <gtest/gtest.h>
#include "xsync/xsync_sender.h"
#include "xsync/xrole_xips_manager.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xdata/xgenesis_data.h"
#include "common.h"
#include "xdata/xnative_contract_address.h"
#if 0
using namespace top;
using namespace top::syncbase;
using namespace top::vnetwork;
using namespace top::data;

class test_xdummy_vhost_t : public tests::vnetwork::xtop_dummy_vhost {
public:
    using send_cb = std::function<void(xmessage_t const &,
                                    xvnode_address_t const &,
                                    xvnode_address_t const &)>;
    using forward_cb = send_cb;
    using broadcast_cb = std::function<void(xmessage_t const &,
                                    xvnode_address_t const &)>;
    void
    send(xmessage_t const & msg,
         xvnode_address_t const & from,
         xvnode_address_t const & to) override {
        if(_send_cb != nullptr) {
            _send_cb(msg, from, to);
        }
    }

    void
    forward_broadcast_message(xmessage_t const & msg,
                              xvnode_address_t const & from,
                              xvnode_address_t const & to) override {
        if(_forward_cb != nullptr) {
            _forward_cb(msg, from, to);
        }
    }

    void
    broadcast(xmessage_t const & msg,
              xvnode_address_t const & from) override {
        if(_broadcast_cb != nullptr) {
            _broadcast_cb(msg, from);
        }
    }

    send_cb         _send_cb{};
    forward_cb      _forward_cb{};
    broadcast_cb    _broadcast_cb{};
};

#define COMMON_VARS_DEFINE()                                    \
    auto beacons = get_beacon_addresses(0, 0, 5);               \
    auto zecs = get_zec_addresses(0, 0, 5);                     \
    auto validators = get_validator_addresses(0, 0, 0, 5);      \
    auto auditors = get_auditor_addresses(0, 0, 0, 5);          \
    auto archives = get_archive_addresses(0, 0, 5);             \
    auto beacon = get_beacon_address(0, 0, "test0");            \
    auto zec = get_zec_address(0, 0, "test0");                  \
    auto validator = get_validator_address(0, 0, "test0", 0);   \
    auto auditor = get_auditor_address(0, 0, "test0", 0);       \
    auto archive = get_archive_address(0, 0, "test0");

TEST(xtransaceiver_t, gossip) {
    COMMON_VARS_DEFINE();
    xrole_xips_manager_t role_xips_mgr("", nullptr);
    xsync_status_t sync_status;

    role_xips_mgr.add_role(validator, validators, auditors, archives); // validator
    role_xips_mgr.add_role(auditor, auditors, {}, archives);   // auditor

    top::mbus::xmessage_bus_t bus;
    test_xdummy_vhost_t vhost;
    xtransaceiver_t sync_sender("", &bus, &vhost, &role_xips_mgr, &sync_status);

#define TEST_SEND_CALL(arr) \
    count = 0;      \
    vhost._send_cb = [&](xmessage_t const &,    \
                            xvnode_address_t const &,   \
                            xvnode_address_t const & to) {  \
        ++count;    \
        auto it = std::find_if(arr.begin(), arr.end(), [&](const xvnode_address_t& addr) { return to == addr;});\
        ASSERT_TRUE(it != arr.end());\
    };

    vnetwork::xvnode_address_t empty_addr;

    // test validator
    int count = 0;
    TEST_SEND_CALL(validators);
    top::vnetwork::xmessage_t msg{};
    sync_sender.send_gossip(validator, msg, 3, empty_addr);
    ASSERT_TRUE(count == 3) << count;

    // test auditor
    TEST_SEND_CALL(auditors);
    sync_sender.send_gossip(auditor, msg, 3, empty_addr);
    ASSERT_TRUE(count == 3);
}

TEST(xtransaceiver_t, to_target) {
    top::mbus::xmessage_bus_t bus;
    test_xdummy_vhost_t vhost;
    xsync_status_t sync_status;

    top::sync::xsync_sender_t sync_sender("", &bus, &vhost, nullptr, &sync_status);

    auto self_xip = get_validator_address(0, 0, "test0", 0);
    auto _to = get_validator_address(0, 0, "test1", 1);
    top::vnetwork::xmessage_t msg{};
    vhost._send_cb = [&](xmessage_t const &,
                            xvnode_address_t const & from,
                            xvnode_address_t const & to) {
        ASSERT_TRUE(from == self_xip);
        ASSERT_TRUE(_to == to);
    };

    sync_sender.to_target(_to, self_xip, msg);
}

TEST(xtransaceiver_t, send_request) {
    top::mbus::xmessage_bus_t bus;
    test_xdummy_vhost_t vhost;
    xsync_status_t sync_status;

    top::vnetwork::xmessage_t msg{};
    COMMON_VARS_DEFINE();

    xrole_xips_manager_t role_xips_mgr("", &vhost);

    role_xips_mgr.clear();
    role_xips_mgr.add_role(beacon, beacons, {}, archives);               // beacon
    role_xips_mgr.add_role(zec, zecs, {}, archives);                     // zec
    role_xips_mgr.add_role(validator, validators, auditors, archives);   // validator
    role_xips_mgr.add_role(auditor, auditors, {}, archives);             // auditor
    role_xips_mgr.add_role(archive, archives, {}, archives);             // archive

    xtransaceiver_t sync_sender("", &bus, &vhost, &role_xips_mgr, &sync_status);

#ifdef TEST_SEND_CALL
#undef TEST_SEND_CALL
#endif
    int count = 0;
#define TEST_SEND_CALL(_from, arr) \
    vhost._send_cb = [&](xmessage_t const &,    \
                            xvnode_address_t const & from,   \
                            xvnode_address_t const & to) {  \
        auto it = std::find_if(arr.begin(), arr.end(), [&](const xvnode_address_t& addr) { return to == addr;});\
        ASSERT_TRUE(it != arr.end());\
        ASSERT_TRUE(from == _from); \
        ++count;\
    }

    std::string vote_contract = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, 1);

    // from validator
    auto task = std::make_shared<xtask_query_local_t>(
        std::make_shared<xstore_request_t>(top::syncbase::xrequest_type_query_height, vote_contract, top::syncbase::enum_target_policy_default),
        top::syncbase::xtask_op_type_none
    );
    task->cur_retry = 1;
    TEST_SEND_CALL(validator, validators);
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 1);
    TEST_SEND_CALL(validator, auditors);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 2);
    TEST_SEND_CALL(archive, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 3);

    // from validator : archive policy
    task = std::make_shared<xtask_query_local_t>(
        std::make_shared<xstore_request_t>(top::syncbase::xrequest_type_query_height, vote_contract, top::syncbase::enum_target_policy_archive),
        top::syncbase::xtask_op_type_none
    );
    task->cur_retry = 1;
    count = 0;
    TEST_SEND_CALL(archive, archives);
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 1);
    TEST_SEND_CALL(archive, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 2);
    TEST_SEND_CALL(archive, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 3);

    // children policy
    task = std::make_shared<xtask_query_local_t>(
        std::make_shared<xstore_request_t>(top::syncbase::xrequest_type_query_height, vote_contract, top::syncbase::enum_target_policy_children),
        top::syncbase::xtask_op_type_none
    );
    task->cur_retry = 1;
    count = 0;
    TEST_SEND_CALL(validator, validators);
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 1);
    TEST_SEND_CALL(auditor, validators);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 2);
    TEST_SEND_CALL(archive, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 3);

    // from beacon
    role_xips_mgr.clear();
    role_xips_mgr.add_role(beacon, beacons, {}, archives);               // beacon
    task = std::make_shared<xtask_query_local_t>(
        std::make_shared<xstore_request_t>(top::syncbase::xrequest_type_query_height, sys_contract_rec_tcc_addr, top::syncbase::enum_target_policy_default),
        top::syncbase::xtask_op_type_none
    );
    task->cur_retry = 1;
    count = 0;
    TEST_SEND_CALL(beacon, beacons);
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 1);
    TEST_SEND_CALL(beacon, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 2);
    TEST_SEND_CALL(beacon, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 3);

    // from zec
    role_xips_mgr.clear();
    role_xips_mgr.add_role(zec, zecs, {}, archives);                     // zec
    task = std::make_shared<xtask_query_local_t>(
        std::make_shared<xstore_request_t>(top::syncbase::xrequest_type_query_height, sys_contract_rec_elect_rec_addr, top::syncbase::enum_target_policy_default),
        top::syncbase::xtask_op_type_none
    );
    task->cur_retry = 1;
    count = 0;
    TEST_SEND_CALL(zec, zecs);
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 1);
    TEST_SEND_CALL(zec, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 2);
    TEST_SEND_CALL(zec, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 3);

    // from auditor
    role_xips_mgr.clear();
    role_xips_mgr.add_role(auditor, auditors, {}, archives);                     // auditor
    task = std::make_shared<xtask_query_local_t>(
        std::make_shared<xstore_request_t>(top::syncbase::xrequest_type_query_height, sys_contract_rec_elect_rec_addr, top::syncbase::enum_target_policy_default),
        top::syncbase::xtask_op_type_none
    );
    task->cur_retry = 1;
    count = 0;
    TEST_SEND_CALL(auditor, auditors);
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 1);
    TEST_SEND_CALL(auditor, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 2);
    TEST_SEND_CALL(auditor, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 3);

    // from archive
    role_xips_mgr.clear();
    role_xips_mgr.add_role(archive, archives, {}, archives);             // archive
    task = std::make_shared<xtask_query_local_t>(
       std::make_shared<xstore_request_t>(top::syncbase::xrequest_type_query_height, sys_contract_rec_elect_rec_addr, top::syncbase::enum_target_policy_default),
        top::syncbase::xtask_op_type_none
    );
    task->cur_retry = 1;
    count = 0;
    TEST_SEND_CALL(archive, archives);
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 1);
    TEST_SEND_CALL(archive, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 2);
    TEST_SEND_CALL(archive, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 3);

    // from beacon : system sharding account
    role_xips_mgr.clear();
    role_xips_mgr.add_role(beacon, beacons, {}, archives);               // beacon
    task = std::make_shared<xtask_query_local_t>(
        std::make_shared<xstore_request_t>(top::syncbase::xrequest_type_query_height, vote_contract, top::syncbase::enum_target_policy_default),
        top::syncbase::xtask_op_type_none
    );
    task->cur_retry = 1;
    count = 0;
    role_xips_mgr.clear();
    role_xips_mgr.add_role(beacon, beacons, {}, archives);             // beacon
    TEST_SEND_CALL(beacon, beacons);
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 1);
    TEST_SEND_CALL(beacon, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 2);
    TEST_SEND_CALL(beacon, archives);
    task->cur_retry++;
    sync_sender.send_request(task, msg);
    ASSERT_TRUE(count == 3);
}

TEST(xtransaceiver_t, to_archive) {
    top::mbus::xmessage_bus_t bus;
    test_xdummy_vhost_t vhost;
    xsync_status_t sync_status;

    top::sync::xsync_sender_t sync_sender("", &bus, &vhost, nullptr, &sync_status);

    top::vnetwork::xmessage_t msg{};
    int count = 0;
    // from non-archive
    auto validator = get_validator_address(0, 0, "test0", 0);
    vhost._forward_cb = [&](xmessage_t const &,
                            xvnode_address_t const & from,
                            xvnode_address_t const & to) {
        ++count;
    };
    sync_sender.to_archive(validator, msg);
    ASSERT_TRUE(count == 1);

    // from archive
    auto archive = get_archive_address(0, 0, "test0");
    count = 0;
    vhost._broadcast_cb = [&](xmessage_t const &,
                            xvnode_address_t const & from) {
        ++count;
    };
    sync_sender.to_archive(archive, msg);
    ASSERT_TRUE(count == 1);
}
#endif