// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "gtest/gtest.h"
#include "test_rpc.h"
#include "xdata/xaction_parse.h"
#include "xelect/client/xelect_client.h"
#include "xrpc/xrpc_init.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xrpc/xerror/xrpc_error.h"

using namespace std;
using namespace top;
using namespace top::xrpc;

using top::vnetwork::xmessage_t;
using top::vnetwork::xvhost_face_t;
using top::vnetwork::xvnetwork_message_ready_callback_t;
using top::vnetwork::xvnode_address_t;

class xtop_dummy_vnetwork_driver : public top::vnetwork::xvnetwork_driver_face_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_vnetwork_driver);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_vnetwork_driver);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_vnetwork_driver);

    void start() override {}

    void stop() override {}

    void register_message_ready_notify(common::xmessage_category_t const, xvnetwork_message_ready_callback_t) override {}

    void unregister_message_ready_notify(common::xmessage_category_t const) override {}

    common::xnetwork_id_t network_id() const noexcept override { return common::xtopchain_network_id; }

    xvnode_address_t address() const override { return xvnode_address_t(common::build_committee_sharding_address(top::common::xbeacon_network_id)); }

    void send_to(xvnode_address_t const & to, xmessage_t const & message, std::error_code & ec) override {}

    void send_to(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) override {}

    void broadcast(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) override {}

    common::xaccount_address_t const & account_address() const noexcept override {
        static common::xnode_id_t nid;
        return nid;
    }

    xvnode_address_t parent_group_address() const override { return {}; }

    observer_ptr<xvhost_face_t> virtual_host() const noexcept override { return observer_ptr<xvhost_face_t>{}; }

    common::xnode_type_t type() const noexcept override { return common::xnode_type_t::invalid; }

    std::vector<xvnode_address_t> archive_addresses(common::xnode_type_t) const override { return {}; }
    std::vector<common::xnode_address_t> fullnode_addresses(std::error_code & ec) const override { return {}; }
    std::vector<common::xnode_address_t> relay_addresses(std::error_code & ec) const override { return {}; }

    std::vector<std::uint16_t> table_ids() const override { return {}; }

    std::uint64_t start_time() const noexcept { return 0; }

    std::map<common::xslot_id_t, data::xnode_info_t> neighbors_info2() const override { return {}; }

    std::map<common::xslot_id_t, data::xnode_info_t> parents_info2() const override { return {}; }

    std::map<common::xslot_id_t, data::xnode_info_t> children_info2(common::xgroup_id_t const & gid, common::xelection_round_t const & version) const override { return {}; }

    common::xelection_round_t const & joined_election_round() const override {
        static common::xelection_round_t joined_election_round;
        return joined_election_round;
    }
};
using xdummy_vnetwork_driver_t = xtop_dummy_vnetwork_driver;

class test_shard : public testing::Test {
protected:
    void SetUp() override {
        m_vhost = std::make_shared<xdummy_vnetwork_driver_t>();
        m_unit_service = std::make_shared<xtop_dummy_txpool_proxy_face>();
        m_thread = top::make_observer(base::xiothread_t::create_thread(base::xcontext_t::instance(), 0, -1));
        m_shard_handler = std::make_shared<xshard_rpc_handler>(m_vhost, m_unit_service, m_thread);
    }

    void TearDown() override {}

public:
    std::shared_ptr<xdummy_vnetwork_driver_t> m_vhost;
    xtxpool_service_v2::xtxpool_proxy_face_ptr m_unit_service;
    observer_ptr<base::xvblockstore_t> m_block_store;
    shared_ptr<xshard_rpc_handler> m_shard_handler;
    observer_ptr<top::base::xiothread_t> m_thread;
};

TEST_F(test_shard, basic) {
    int32_t cnt{0};

    xvnode_address_t edge_sender;
    xmessage_t message;
    try {
        m_shard_handler->on_message(edge_sender, message, 10);
    } catch (...) {
        cnt++;
    }

    xrpc_msg_request_t edge_message;
    m_shard_handler->shard_process_request(edge_message, edge_sender, 111);
    EXPECT_EQ(0, cnt);
}

TEST_F(test_shard, process_msg_illegal) {
    // int32_t cnt{0};
    // xrpc_msg_request_t edge_msg;
    // xjson_proc_t json_proc;
    // try {
    //     m_shard_handler->process_msg(edge_msg, json_proc);
    // } catch (xrpc_error & e) {
    //     EXPECT_EQ(string("unknow msg type"), string(e.what()));
    //     cnt++;
    // }
    // EXPECT_EQ(1, cnt);

    // edge_msg.m_tx_type = enum_xrpc_tx_type::enum_xrpc_query_type;
    // try {
    //     m_shard_handler->process_msg(edge_msg, json_proc);
    // } catch (xrpc_error & e) {
    //     EXPECT_EQ(string("unknow msg type"), string(e.what()));
    //     cnt++;
    // }
    // EXPECT_EQ(2, cnt);

    // auto tx = make_object_ptr<data::xtransaction_t>();
    // data::xproperty_asset asset_out{100};
    // data::xaction_asset_out action_asset_out;
    // action_asset_out.serialze_to(tx->get_source_action(), asset_out);
    // action_asset_out.serialze_to(tx->get_target_action(), asset_out);
    // tx->get_source_action().set_account_addr("m_source_account");
    // tx->get_target_action().set_account_addr("m_target_account");
    // Json::Value tx_json;
    // tx_json["version"] = "1.0";
    // tx_json["method"] = "account_info";
    // edge_msg.m_message_body = tx_json.toStyledString();
    // //"version=1.0&account_address=T-123456789012345678901234567890123&token=1111111&method=account_info&sequence_id=2&body={\"version\": \"1.0\",\"method\": \"account_info\",
    // //\"params\":{\"account\":\"\"}}";
    // try {
    //     m_shard_handler->process_msg(edge_msg, json_proc);
    // } catch (xrpc_error & e) {
    //     EXPECT_EQ(string("miss param method or method is empty"), string(e.what()));
    // }

    // edge_msg.m_tx_type = enum_xrpc_tx_type::enum_xrpc_tx_type;
    // try{
    //     m_shard_handler->process_msg(edge_msg, json_proc);
    // }catch(...){
    //     cnt++;
    // }
    // EXPECT_EQ(3, cnt);
}
