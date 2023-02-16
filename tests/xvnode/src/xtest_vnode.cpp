// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xchain_timer/xlocal_logic_timer.h"
#include "tests/xnetwork/xdummy_network_driver.h"
#include "tests/xvnetwork/xvnetwork_fixture.h"
#include "xbasic/xmemory.hpp"
#include "xelection/xcache/xdata_accessor.h"
#include "xcommon/xmessage_id.h"
#include "xvnetwork/xvhost.h"
#include "xvnode/xbasic_vnode.h"

#include <chrono>
#include <thread>

#define private protected
#define final 

NS_BEG3(top, tests, vnode)

class xtop_mocked_vnode : public top::vnode::xbasic_vnode_t {
public:
    xtop_mocked_vnode(xtop_mocked_vnode const &) = delete;
    xtop_mocked_vnode & operator=(xtop_mocked_vnode const &) = delete;
    xtop_mocked_vnode(xtop_mocked_vnode &&) = default;
    xtop_mocked_vnode & operator=(xtop_mocked_vnode &&) = default;
    ~xtop_mocked_vnode() override = default;

    explicit xtop_mocked_vnode(common::xgroup_address_t const & group_address,
                               common::xaccount_election_address_t const & account_election_address,
                               common::xlogic_epoch_t const & group_logic_epoch,
                               observer_ptr<top::vnetwork::xvhost_face_t> const & vhost,
                               observer_ptr<top::election::cache::xdata_accessor_face_t> const & data_accessor)
      : top::vnode::xbasic_vnode_t{common::xnode_address_t{group_address, account_election_address, group_logic_epoch}, common::xelection_round_t{0}, vhost, data_accessor} {
    }

    void start() override {
        running(true);
    }

    void stop() override {
        running(false);
    }

    void fade() override {
    }

    void synchronize() override {
    }

    top::vnode::components::sniffing::xvnode_sniff_config_t sniff_config() noexcept override {
        return top::vnode::components::sniffing::xvnode_sniff_config_t{};
    }
};
using xmocked_vnode_t = xtop_mocked_vnode;

class xtop_vnode_fixture : public top::tests::vnetwork::xvnetwork_fixture2_t {
protected:
    std::shared_ptr<top::vnode::xvnode_face_t> validator_vnode_;
    std::shared_ptr<top::vnode::xvnode_face_t> auditor_vnode1_;
    std::shared_ptr<top::vnode::xvnode_face_t> auditor_vnode2_;

public:
    xtop_vnode_fixture() = default;
    xtop_vnode_fixture(xtop_vnode_fixture const &) = delete;
    xtop_vnode_fixture & operator=(xtop_vnode_fixture const &) = delete;
    xtop_vnode_fixture(xtop_vnode_fixture &&) = default;
    xtop_vnode_fixture & operator=(xtop_vnode_fixture &&) = default;
    ~xtop_vnode_fixture() override = default;

protected:
    xobject_ptr_t<time::xchain_time_face_t> create_logic_chain_timer() const override {
        return make_object_ptr<top::tests::chain_timer::xlocal_logic_timer_t>();
    }

    std::unique_ptr<election::cache::xdata_accessor_face_t> create_election_data_accessor() const override {
        return top::make_unique<election::cache::xdata_accessor_t>(common::xtestnet_id, make_observer(this->logic_timer_.get()));
    }

    std::shared_ptr<top::network::xnetwork_driver_face_t> create_netwrok_driver() const override {
        return std::make_shared<top::tests::network::xdummy_network_driver_t>();
    }

public:
    void SetUp() override {
        top::tests::vnetwork::xvnetwork_fixture2_t::SetUp();

        common::xgroup_address_t group_address_no_logic_epoch{
            common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator1.group_id};
        std::error_code ec;

        auto group_element = data_accessor_->group_element_by_height(group_address_no_logic_epoch, vnetwork::logic_epoch_1_blk_height, ec);

        validator_vnode_ = std::make_shared<xmocked_vnode_t>(
            common::xgroup_address_t{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, tests::vnetwork::account_pubkey_validator1.group_id},
            common::xaccount_election_address_t{tests::vnetwork::account_pubkey_validator1.account, common::xslot_id_t{0}},
            group_element->logic_epoch(),
            make_observer(this->vhost_.get()),
            make_observer(this->data_accessor_.get()));
        validator_vnode_->rotation_status(top::common::xrotation_status_t::started, logic_timer_->logic_time());
        validator_vnode_->start();

        auditor_vnode1_ = std::make_shared<xmocked_vnode_t>(
            common::xgroup_address_t{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, tests::vnetwork::account_pubkey_auditor1.group_id},
            common::xaccount_election_address_t{tests::vnetwork::account_pubkey_auditor1.account, common::xslot_id_t{0}},
            group_element->logic_epoch(),
            make_observer(this->vhost_.get()),
            make_observer(this->data_accessor_.get()));
        auditor_vnode1_->rotation_status(top::common::xrotation_status_t::started, logic_timer_->logic_time());
        auditor_vnode1_->start();

        auditor_vnode2_ = std::make_shared<xmocked_vnode_t>(
            common::xgroup_address_t{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, tests::vnetwork::account_pubkey_auditor2.group_id},
            common::xaccount_election_address_t{tests::vnetwork::account_pubkey_auditor2.account, common::xslot_id_t{0}},
            group_element->logic_epoch(),
            make_observer(this->vhost_.get()),
            make_observer(this->data_accessor_.get()));
        auditor_vnode2_->rotation_status(top::common::xrotation_status_t::started, logic_timer_->logic_time());
        auditor_vnode2_->start();
    }

    void TearDown() override {
        validator_vnode_->stop();
        validator_vnode_.reset();
        top::tests::vnetwork::xvnetwork_fixture2_t::TearDown();
    }
};
using xvnode_fixture_t = xtop_vnode_fixture;

TEST_F(xvnode_fixture_t, broadcast_normal_in_group) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    common::xgroup_address_t group_address_no_logic_epoch{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator1.group_id};
    std::error_code ec;

    auto group_element = data_accessor_->group_element_by_height(group_address_no_logic_epoch, vnetwork::logic_epoch_1_blk_height, ec);
    EXPECT_TRUE(!ec);

    validator_vnode_->broadcast(common::xnode_address_t{group_address_no_logic_epoch, group_element->logic_epoch()}.xip2(),
                      top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id},
                      ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(1, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);

    validator_vnode_->broadcast(common::xnode_address_t{group_address_no_logic_epoch}.xip2(), top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id}, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(2, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

TEST_F(xvnode_fixture_t, broadcast_normal_cross_group_associated) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    common::xgroup_address_t associated_group_address_no_logic_epoch{
        common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator2.group_id};
    std::error_code ec;

    auto group_element = data_accessor_->group_element_by_height(associated_group_address_no_logic_epoch, vnetwork::logic_epoch_1_blk_height, ec);
    EXPECT_TRUE(!ec);

    validator_vnode_->broadcast(common::xnode_address_t{associated_group_address_no_logic_epoch, group_element->logic_epoch()}.xip2(),
                      top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id},
                      ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(1, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);

    validator_vnode_->broadcast(common::xnode_address_t{associated_group_address_no_logic_epoch}.xip2(), top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id}, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(2, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

TEST_F(xvnode_fixture_t, broadcast_normal_cross_group_nonassociated) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    common::xgroup_address_t nonassociated_group_address_no_logic_epoch{
        common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator3.group_id};
    std::error_code ec;

    auto group_element = data_accessor_->group_element_by_height(nonassociated_group_address_no_logic_epoch, vnetwork::logic_epoch_1_blk_height, ec);
    EXPECT_TRUE(!ec);

    validator_vnode_->broadcast(common::xnode_address_t{nonassociated_group_address_no_logic_epoch, group_element->logic_epoch()}.xip2(),
                      top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id},
                      ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(1, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);

    validator_vnode_->broadcast(common::xnode_address_t{nonassociated_group_address_no_logic_epoch}.xip2(), top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id}, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(2, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

TEST_F(xvnode_fixture_t, broadcast_normal_network) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    common::xgroup_address_t network_group{common::xtestnet_id};
    std::error_code ec;

    validator_vnode_->broadcast(common::xnode_address_t{network_group}.xip2(),
                      top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id},
                      ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(1, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

TEST_F(xvnode_fixture_t, broadcast_abnormal_dst_is_not_broadcast_addr) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    common::xgroup_address_t broadcast_group{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator2.group_id};
    std::error_code ec;

    auto group_element = data_accessor_->group_element_by_height(broadcast_group, vnetwork::logic_epoch_1_blk_height, ec);
    EXPECT_TRUE(!ec);

    auto const dst_children = group_element->children(ec);
    ASSERT_TRUE(!ec);

    auto const dst_child = dst_children.begin()->second;
    auto const dst_address = dst_child->address();

    validator_vnode_->broadcast(dst_address.xip2(), top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id}, ec);
    ASSERT_TRUE(!!ec);
    ASSERT_EQ(0, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

//TEST_F(xvnode_fixture_t, broadcast_abnormal_cross_group_associated_logic_epoch_mismatch) {
//    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());
//
//    common::xgroup_address_t associated_group_address_no_logic_epoch{
//        common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator2.group_id};
//    std::error_code ec;
//
//    auto group_element = data_accessor_->group_element_by_height(associated_group_address_no_logic_epoch, vnetwork::logic_epoch_1_blk_height, ec);
//    EXPECT_TRUE(!ec);
//
//    validator_vnode_->broadcast(common::xnode_address_t{associated_group_address_no_logic_epoch, common::xlogic_epoch_t{10, 5}}.xip2(),
//                      top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id},
//                      ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_EQ(1, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);
//
//    std::this_thread::sleep_for(std::chrono::milliseconds{50});
//}

//TEST_F(xvnode_fixture_t, send_to_normal_in_group) {
//    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());
//
//    common::xgroup_address_t group_address_no_logic_epoch{
//        common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator1.group_id};
//    std::error_code ec;
//
//    auto group_element = data_accessor_->group_element_by_height(group_address_no_logic_epoch, vnetwork::logic_epoch_1_blk_height, ec);
//    EXPECT_TRUE(!ec);
//
//    validator_vnode_->broadcast(common::xnode_address_t{group_address_no_logic_epoch, group_element->logic_epoch()}.xip2(),
//                      top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id},
//                      ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_EQ(1, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);
//
//    validator_vnode_->broadcast(common::xnode_address_t{group_address_no_logic_epoch}.xip2(), top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id}, ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_EQ(2, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter_spread_rumor);
//
//    std::this_thread::sleep_for(std::chrono::milliseconds{50});
//}

TEST_F(xvnode_fixture_t, send_to_normal_cross_group_associated) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    common::xgroup_address_t associated_group_address_no_logic_epoch{
        common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator2.group_id};
    std::error_code ec;

    auto group_element = data_accessor_->group_element_by_height(associated_group_address_no_logic_epoch, vnetwork::logic_epoch_1_blk_height, ec);
    ASSERT_TRUE(!ec);

    auto const dst_children = group_element->children(ec);
    ASSERT_TRUE(!ec);

    auto const dst_child = dst_children.begin()->second;
    auto const dst_address = dst_child->address();

    validator_vnode_->send_to(dst_address.xip2(), top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id}, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(1, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

TEST_F(xvnode_fixture_t, send_to_abnormal_self) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    std::error_code ec;
    validator_vnode_->send_to(validator_vnode_->address().xip2(), top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id}, ec);
    ASSERT_TRUE(!!ec);
    ASSERT_EQ(0, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

TEST_F(xvnode_fixture_t, send_to_abnormal_broadcast_dst) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    common::xgroup_address_t associated_group_address_no_logic_epoch{
        common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator2.group_id};
    std::error_code ec;

    auto group_element = data_accessor_->group_element_by_height(associated_group_address_no_logic_epoch, vnetwork::logic_epoch_1_blk_height, ec);
    EXPECT_TRUE(!ec);

    validator_vnode_->send_to(common::xnode_address_t{associated_group_address_no_logic_epoch, group_element->logic_epoch()}.xip2(),
                    top::vnetwork::xmessage_t{xbyte_buffer_t{}, xmessage_block_broadcast_id},
                    ec);
    ASSERT_TRUE(!!ec);
    ASSERT_EQ(0, std::dynamic_pointer_cast<tests::network::xdummy_network_driver_t>(network_driver_)->m_counter);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

TEST_F(xvnode_fixture_t, neighbors_xip2_normal) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    std::error_code ec;
    auto neighbors = validator_vnode_->neighbors_xip2(ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ(1, neighbors.size());
    ASSERT_EQ(0, neighbors[0].slot_id().value());

    auto const node_element = data_accessor_->node_element(common::xgroup_address_t{neighbors[0].xip()}, common::xlogic_epoch_t{neighbors[0].size(), neighbors[0].height()}, neighbors[0].slot_id(), ec);
    ASSERT_NE(nullptr, node_element);
    ASSERT_EQ(node_element->account_address().to_string(), validator_vnode_->address().account_address().to_string());
}

//TEST_F(xvnode_fixture_t, associated_nodes_xip2_normal) {
//    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());
//
//    std::error_code ec;
//    common::xgroup_address_t associated_group_addr{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator2.group_id};
//    auto associated_nodes = validator_vnode_->associated_nodes_xip2(associated_group_addr.xip(), ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_EQ(1, associated_nodes.size());
//
//    associated_group_addr = common::xgroup_address_t{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_auditor1.group_id};
//    associated_nodes = validator_vnode_->associated_nodes_xip2(associated_group_addr.xip(), ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_EQ(1, associated_nodes.size());
//}

//TEST_F(xvnode_fixture_t, associated_nodes_xip2_abnormal_broadcast_xip) {
//    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());
//
//    std::error_code ec;
//    common::xgroup_address_t associated_group_addr{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id};
//    auto associated_nodes = validator_vnode_->associated_nodes_xip2(associated_group_addr.xip(), ec);
//    ASSERT_TRUE(!!ec);
//    ASSERT_TRUE(associated_nodes.empty());
//}

//TEST_F(xvnode_fixture_t, associated_nodes_xip2_abnormal_nonassociated_xip) {
//    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());
//
//    std::error_code ec;
//    common::xgroup_address_t nonassociated_group_addr{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator3.group_id};
//    auto nodes = validator_vnode_->associated_nodes_xip2(nonassociated_group_addr.xip(), ec);
//    ASSERT_TRUE(!!ec);
//    ASSERT_TRUE(nodes.empty());
//
//    ec.clear();
//    nonassociated_group_addr = common::xgroup_address_t{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_auditor2.group_id};
//    nodes = validator_vnode_->associated_nodes_xip2(nonassociated_group_addr.xip(), ec);
//    ASSERT_TRUE(!!ec);
//    ASSERT_TRUE(nodes.empty());
//}

//TEST_F(xvnode_fixture_t, nonassociated_nodes_xip2_normal) {
//    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());
//
//    std::error_code ec;
//    common::xgroup_address_t nonassociated_group_addr{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator3.group_id};
//    auto nodes = validator_vnode_->nonassociated_nodes_xip2(nonassociated_group_addr.xip(), ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_EQ(1, nodes.size());
//
//    nonassociated_group_addr = common::xgroup_address_t{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_auditor2.group_id};
//    nodes = validator_vnode_->nonassociated_nodes_xip2(nonassociated_group_addr.xip(), ec);
//    ASSERT_TRUE(!ec);
//    ASSERT_EQ(1, nodes.size());
//}

//TEST_F(xvnode_fixture_t, nonassociated_nodes_xip2_abnormal_associated_xip) {
//    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());
//
//    std::error_code ec;
//    common::xgroup_address_t associated_group_addr{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_validator2.group_id};
//    auto nodes = validator_vnode_->nonassociated_nodes_xip2(associated_group_addr.xip(), ec);
//    ASSERT_TRUE(!!ec);
//    ASSERT_TRUE(nodes.empty());
//
//    ec.clear();
//    associated_group_addr = common::xgroup_address_t{common::xtestnet_id, common::xconsensus_zone_id, common::xdefault_cluster_id, vnetwork::account_pubkey_auditor1.group_id};
//    nodes = validator_vnode_->nonassociated_nodes_xip2(associated_group_addr.xip(), ec);
//    ASSERT_TRUE(!!ec);
//    ASSERT_TRUE(nodes.empty());
//}

TEST_F(xvnode_fixture_t, associated_parent_nodes_xip2_normal) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    std::error_code ec;
    auto const & parent_nodes_xip2 = validator_vnode_->associated_parent_nodes_xip2(ec);
    ASSERT_TRUE(!ec);
    ASSERT_FALSE(parent_nodes_xip2.empty());
    ASSERT_EQ(1, parent_nodes_xip2.size());

    auto const & parent_nodes_xip2_again = validator_vnode_->associated_parent_nodes_xip2(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(parent_nodes_xip2_again, parent_nodes_xip2);
}

TEST_F(xvnode_fixture_t, associated_parent_nodes_xip2_abnormal) {
    ASSERT_EQ(vnetwork::account_pubkey_auditor1.account.to_string(), auditor_vnode1_->address().account_address().to_string());

    std::error_code ec;
    auto const & parent_nodes_xip2 = auditor_vnode1_->associated_parent_nodes_xip2(ec);
    ASSERT_FALSE(!ec);
    ASSERT_TRUE(parent_nodes_xip2.empty());

    ec.clear();
    auto const & parent_nodes_xip2_again = auditor_vnode1_->associated_parent_nodes_xip2(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(parent_nodes_xip2_again, parent_nodes_xip2);
}

TEST_F(xvnode_fixture_t, associated_child_nodes_xip2_normal) {
    ASSERT_EQ(vnetwork::account_pubkey_auditor1.account.to_string(), auditor_vnode1_->address().account_address().to_string());

    std::error_code ec;
    auto const & child_nodes_xip2 = auditor_vnode1_->associated_child_nodes_xip2(validator_vnode_->address().xip2(), ec);
    ASSERT_TRUE(!ec);
    ASSERT_FALSE(child_nodes_xip2.empty());
    ASSERT_EQ(1, child_nodes_xip2.size());

    auto const & child_nodes_xip2_again = auditor_vnode1_->associated_child_nodes_xip2(validator_vnode_->address().xip2(), ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(child_nodes_xip2_again, child_nodes_xip2);
}

TEST_F(xvnode_fixture_t, associated_child_nodes_xip2_abnormal_nonassociated_validator) {
    ASSERT_EQ(vnetwork::account_pubkey_auditor2.account.to_string(), auditor_vnode2_->address().account_address().to_string());

    std::error_code ec;
    auto const & child_nodes_xip2 = auditor_vnode2_->associated_child_nodes_xip2(validator_vnode_->address().xip2(), ec);
    ASSERT_FALSE(!ec);
    ASSERT_TRUE(child_nodes_xip2.empty());

    ec.clear();
    auto const & child_nodes_xip2_again = auditor_vnode2_->associated_child_nodes_xip2(validator_vnode_->address().xip2(), ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(child_nodes_xip2_again, child_nodes_xip2);
}

TEST_F(xvnode_fixture_t, associated_child_nodes_xip2_abnormal_validator) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    std::error_code ec;
    auto const & child_nodes_xip2 = validator_vnode_->associated_child_nodes_xip2(validator_vnode_->address().xip2(), ec);
    ASSERT_FALSE(!ec);
    ASSERT_TRUE(child_nodes_xip2.empty());

    ec.clear();
    auto const & child_nodes_xip2_again = validator_vnode_->associated_child_nodes_xip2(validator_vnode_->address().xip2(), ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(child_nodes_xip2_again, child_nodes_xip2);
}

TEST_F(xvnode_fixture_t, status) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    auto status = validator_vnode_->status();
    ASSERT_EQ(status, common::xrotation_status_t::started);

    validator_vnode_->rotation_status(top::common::xrotation_status_t::faded, logic_timer_->logic_time() + 1);
    status = validator_vnode_->status();
    ASSERT_EQ(status, common::xrotation_status_t::started);

    std::this_thread::sleep_for(std::chrono::seconds{10});
    status = validator_vnode_->status();
    ASSERT_EQ(status, common::xrotation_status_t::faded);

    validator_vnode_->rotation_status(top::common::xrotation_status_t::outdated, logic_timer_->logic_time() + 1);
    status = validator_vnode_->status();
    ASSERT_EQ(status, common::xrotation_status_t::faded);

    std::this_thread::sleep_for(std::chrono::seconds{10});
    status = validator_vnode_->status();
    ASSERT_EQ(status, common::xrotation_status_t::outdated);
}

TEST_F(xvnode_fixture_t, type) {
    ASSERT_EQ(vnetwork::account_pubkey_validator1.account.to_string(), validator_vnode_->address().account_address().to_string());

    auto type = validator_vnode_->type();
    ASSERT_EQ(type, common::xnode_type_t::consensus_validator);

    type = auditor_vnode1_->type();
    ASSERT_EQ(type, common::xnode_type_t::consensus_auditor);
}


NS_END3
