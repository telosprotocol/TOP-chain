// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xnetwork/xdummy_network_driver.h"
#include "tests/xvnetwork/xdummy_chain_timer.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "tests/xvnetwork/xvnetwork_fixture.h"
#include "xcommon/xaddress.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xversion.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xelection/xcache/xdata_accessor.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xmessage_filter.h"
#include "xvnetwork/xmessage_filter_manager.h"
#include "xvnetwork/xvhost.h"
#include "xvnetwork/xvnetwork_error2.h"
#include "xvnetwork/xvnetwork_message.h"

#include <gtest/gtest.h>

#include <list>
#include <memory>

using top::common::xaccount_election_address_t;
using top::common::xcluster_id_t;
using top::common::xgroup_id_t;
using top::common::xlogic_time_t;
using top::common::xnetwork_id_t;
using top::common::xnode_address_t;
using top::common::xnode_id_t;
using top::common::xsharding_address_t;
using top::common::xelection_round_t;
using top::common::xzone_id_t;
using top::vnetwork::xmessage_filter_manager_t;
using top::vnetwork::xmessage_t;
using top::vnetwork::xvnetwork_message_t;
using top::vnetwork::xvnetwork_message_type_t;

NS_BEG3(top, tests, vnetwork)

class xtop_message_filter_chain_timer final : public tests::chain_timer::xdummy_chain_timer_t {
public:
    xtop_message_filter_chain_timer() = default;
    xtop_message_filter_chain_timer(xtop_message_filter_chain_timer const &) = delete;
    xtop_message_filter_chain_timer(xtop_message_filter_chain_timer &&) = default;
    xtop_message_filter_chain_timer & operator=(xtop_message_filter_chain_timer const &) = delete;
    xtop_message_filter_chain_timer & operator=(xtop_message_filter_chain_timer &&) = delete;
    ~xtop_message_filter_chain_timer() override = default;

    common::xlogic_time_t logic_time() const noexcept override {
        return 45;
    }
};
using xmessage_filter_chain_timer_t = xtop_message_filter_chain_timer;


class test_message_filter : public xvnetwork_fixture2_t {
protected:
    std::unique_ptr<top::vnetwork::xmessage_filter_manager_face_t> filter_mgr_;

public:
    test_message_filter() = default;
    test_message_filter(test_message_filter const &) = delete;
    test_message_filter(test_message_filter &&) = default;
    test_message_filter & operator=(test_message_filter const &) = delete;
    test_message_filter & operator=(test_message_filter &&) = default;
    ~test_message_filter() override = default;

public:
    xobject_ptr_t<time::xchain_time_face_t> create_logic_chain_timer() const override {
        return top::xobject_ptr_t<time::xchain_time_face_t>{make_object_ptr<xmessage_filter_chain_timer_t>()};
    }

    std::unique_ptr<election::cache::xdata_accessor_face_t> create_election_data_accessor() const override {
        return top::make_unique<election::cache::xdata_accessor_t>(common::xtestnet_id, make_observer(this->logic_timer_.get()));
    }

    std::shared_ptr<top::network::xnetwork_driver_face_t> create_netwrok_driver() const override {
        return std::make_shared<top::tests::network::xdummy_network_driver_t>();
    }

    void SetUp() override {
        xvnetwork_fixture2_t::SetUp();

        filter_mgr_ = top::make_unique<top::vnetwork::xmessage_filter_manager_t>(make_observer(vhost_.get()), make_observer(data_accessor_.get()));
        filter_mgr_->start();
    }

    void TearDown() override {
        filter_mgr_->stop();
        filter_mgr_.reset();
        xvnetwork_fixture2_t::TearDown();
    }

    //observer_ptr<top::vnetwork::xvhost_face_t> vhost() const noexcept {
    //    return top::make_observer<top::vnetwork::xvhost_face_t>(vhost_.get());
    //}

    //observer_ptr<top::election::cache::xdata_accessor_face_t> data_accessor() const noexcept {
    //    return top::make_observer<top::election::cache::xdata_accessor_face_t>(data_accessor_.get());
    //}
};

common::xnode_address_t build_node_address(common::xaccount_address_t account_address,
                                           common::xslot_id_t slot_id,
                                           common::xlogic_epoch_t logic_epoch,
                                           common::xgroup_address_t group_address) {
    if (logic_epoch.empty()) {
        return common::xnode_address_t{
            std::move(group_address),
            common::xaccount_election_address_t{
                std::move(account_address),
                std::move(slot_id)
            }
        };
    }
    return common::xnode_address_t{
        std::move(group_address),
        common::xaccount_election_address_t{
            std::move(account_address),
            std::move(slot_id)
        },
        std::move(logic_epoch)
    };
}

common::xnode_address_t build_rec_node_address(common::xaccount_address_t account_address,
                                               common::xslot_id_t slot_id,
                                               common::xlogic_epoch_t logic_epoch) {
    return build_node_address(std::move(account_address),
                              std::move(slot_id),
                              std::move(logic_epoch),
                              common::build_committee_sharding_address(common::xtestnet_id));
}

common::xnode_address_t build_zec_node_address(common::xaccount_address_t account_address,
                                               common::xslot_id_t slot_id,
                                               common::xlogic_epoch_t logic_epoch) {
    return build_node_address(std::move(account_address),
                              std::move(slot_id),
                              std::move(logic_epoch),
                              common::build_zec_sharding_address(common::xtestnet_id));
}

common::xnode_address_t build_edge_node_address(common::xaccount_address_t account_address,
                                                common::xslot_id_t slot_id,
                                                common::xlogic_epoch_t logic_epoch) {
    return build_node_address(std::move(account_address),
                              std::move(slot_id),
                              std::move(logic_epoch),
                              common::build_edge_sharding_address(common::xtestnet_id));
}

common::xnode_address_t build_auditor_node_address(xaccount_data_bundle_t const & account_data_bundle,
                                                   common::xslot_id_t slot_id,
                                                   common::xlogic_epoch_t logic_epoch) {
    auto const & auditor_group_id = account_data_bundle.group_id;
    if (auditor_group_id < common::xauditor_group_id_begin || auditor_group_id >= common::xauditor_group_id_end) {
        assert(false);
        return {};
    }

    return build_node_address(account_data_bundle.account,
                              std::move(slot_id),
                              std::move(logic_epoch),
                              common::build_consensus_sharding_address(auditor_group_id, common::xtestnet_id));
}

common::xnode_address_t build_validator_node_address(xaccount_data_bundle_t const & account_data_bundle,
                                                     common::xslot_id_t slot_id,
                                                     common::xlogic_epoch_t logic_epoch) {
    auto const & validator_group_id = account_data_bundle.group_id;
    if (validator_group_id < common::xvalidator_group_id_begin || validator_group_id >= common::xvalidator_group_id_end) {
        assert(false);
        return {};
    }

    return build_node_address(account_data_bundle.account,
                              std::move(slot_id),
                              std::move(logic_epoch),
                              common::build_consensus_sharding_address(validator_group_id, common::xtestnet_id));
}

xbyte_buffer_t const message_payload{ static_cast<uint8_t>('t'), static_cast<uint8_t>('o'), static_cast<uint8_t>('p') };
common::xmessage_id_t message_id{ static_cast<common::xmessage_id_t>(1) };
xmessage_t raw_message{ message_payload, message_id };

TEST_F(test_message_filter, empty_message) {
    top::vnetwork::xvnetwork_message_t empty_message;
    std::error_code ec;
    top::vnetwork::xtop_message_filter_sender const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(empty_message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::empty_message);

    ec.clear();
    filter_mgr_->filter_message(empty_message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::empty_message);
}

TEST_F(test_message_filter, invalid_sender_no_account_address) {
    xvnetwork_message_t message{ common::xnode_address_t{}, common::xnode_address_t{}, raw_message, 0 };
    std::error_code ec;
    top::vnetwork::xtop_message_filter_sender const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::empty_message);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::empty_message);
}

TEST_F(test_message_filter, invalid_sender_no_group_address) {
    xvnetwork_message_t message{
        build_node_address(common::xaccount_address_t{"T00000LPrAxH1seEjwcrMVfBcEpbHaGVRQi2M5fX"}, common::xslot_id_t{0}, zec_epoch_1, common::xgroup_address_t{common::xtestnet_id}),
        common::xnode_address_t{},
        raw_message,
        common::xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_sender const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_src_address);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_src_address);
}

TEST_F(test_message_filter, invalid_sender_no_slot_id) {
    xvnetwork_message_t message{
        build_zec_node_address(common::xaccount_address_t{"T00000LPrAxH1seEjwcrMVfBcEpbHaGVRQi2M5fX"}, common::xbroadcast_id_t::slot, zec_epoch_1),
        common::xnode_address_t{},
        raw_message,
        common::xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_sender const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_src_address);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_src_address);
}

TEST_F(test_message_filter, recver_broadcast) {
    xvnetwork_message_t message{
        build_rec_node_address(common::xaccount_address_t{"T00000LQ3ammXf22Y9RjDSDetF67DjVNiGK3HeL8"}, common::xslot_id_t{0}, rec_epoch_1),
        common::xnode_address_t{},
        raw_message,
        logic_timer_->logic_time()
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);

    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
}

TEST_F(test_message_filter, invalid_recver) {
    xvnetwork_message_t message{
        build_rec_node_address(common::xaccount_address_t{"T00000LQ3ammXf22Y9RjDSDetF67DjVNiGK3HeL8"}, common::xslot_id_t{common::xmax_slot_id_value}, rec_epoch_1),
        build_node_address(common::xaccount_address_t{"T00000LTc7a4kLj7bhPht5h855HJM8YbpzX5dJYZ"},
                                  common::xslot_id_t{common::xmax_slot_id_value},
                                  rec_epoch_1,
                                  common::xgroup_address_t{
                                      common::xtopchain_network_id,
                                      common::xconsensus_zone_id,
                                      common::xdefault_cluster_id,
                                      common::xdefault_group_id
                                  }),
        raw_message,
        logic_timer_->logic_time()
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_account_address);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_account_address);
}

xaccount_data_bundle_t const unknown_account_pubkey_auditor1{ "T00000LUqxLRra5ktPd8v2vWaDTrssLVydmeLgBS", "BCuG6j8vPID6A+/fcq07kZIkIq8oX43f3CvUtaf8u/pzFUUt/w61K21/FyjR2yOZE9tXizMaZ4nE8DVB46iokYQ=", common::xgroup_id_t{common::xauditor_group_id_value_begin} };
xaccount_data_bundle_t const unknown_account_pubkey_auditor2{ "T00000LU7uA8YSV75n16VbaGvT4fyQEYjdiMRLnF", "BLuvOW4brME5MRRX25vgYky70mtzQG1eDigUFMDDTXfHOfpnW+PH7RoDKLogDB2UQvbwadc4SjFczh0VaDpDKxc=", common::xgroup_id_t{common::xauditor_group_id_value_begin + 1} };

xaccount_data_bundle_t const unknown_account_pubkey_validator1{ "T00000LVVqpTz658tq25e14UE5Zfup9Y3o4YJ42h", "BI5z7hZ/OcQMfl4w3rTUcFuhso+0wkE2EQzOrT6LNwqBegbS3na1DzU+cUe+cYj5ay0ND181ZQnq5EwJXMkmAoY=", common::xgroup_id_t{common::xvalidator_group_id_value_begin} };
xaccount_data_bundle_t const unknown_account_pubkey_validator2{ "T00000LhjGbL3WZ1dD4ofsTXGMLQKUwtc1i4krgD", "BFjazuDp1qdMZpD0prJ/HTS1m1amGOpvjTQmRlzH9pmlsydTV3CWaHs6678drwWwF2Cu9qTbysCdeL+uukIqE08=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 1} };
xaccount_data_bundle_t const unknown_account_pubkey_validator3{ "T00000LhFGRbv7ZrwDj9a29RPA9rioinEfewtwdE", "BHA75mIlAl3FB0qW7+gSxFJxx1prRlZhUu61bjv/nwUh1WSAdk2iJqxFU132KatjxcxPaxzkmpFkAZv/BxgGtwo=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 2} };
xaccount_data_bundle_t const unknown_account_pubkey_validator4{ "T00000LKgwLvGJEQYfE5Lkdx2DD5h2QCgMt2og4u", "BPSS65cgpqbywcixy04uCeO1fpz6hYLrMxtZ4nfbcGwpynGKGYOQz0wn6lRfuiXjY4FhkL1ty+ycCgUUg6o5fKo=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 3} };

TEST_F(test_message_filter, validator_to_associated_auditor_1) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        build_auditor_node_address(account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        raw_message,
        0
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_auditor1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);

    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, validator_to_associated_auditor_2) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        build_auditor_node_address(account_pubkey_auditor1,
                                   common::xbroadcast_id_t::slot,
                                   common::xlogic_epoch_t{}),
        raw_message,
        logic_timer_->logic_time()
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_auditor1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);

    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, validator_to_associated_auditor_3) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_2),
        build_auditor_node_address(account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        raw_message,
        xlogic_time_t{0}
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_auditor1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::epoch_mismatch);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::epoch_mismatch);
}

TEST_F(test_message_filter, validator_to_associated_auditor_4) {
    xvnetwork_message_t message{
        build_validator_node_address(unknown_account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        build_auditor_node_address(unknown_account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        raw_message,
        xlogic_time_t{0}
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_auditor1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, validator_to_associated_auditor_5) {
    xvnetwork_message_t message{
        build_validator_node_address(unknown_account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        build_auditor_node_address(unknown_account_pubkey_auditor1,
                                   common::xbroadcast_id_t::slot,
                                   common::xlogic_epoch_t{}),
        raw_message,
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().empty());
}

TEST_F(test_message_filter, validator_to_associated_auditor_6) {
    xvnetwork_message_t message{
        build_validator_node_address(unknown_account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_2),
        build_auditor_node_address(unknown_account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        raw_message,
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
}

TEST_F(test_message_filter, auditor_to_associated_validator_1) {
    xvnetwork_message_t message{
        build_auditor_node_address(account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        build_validator_node_address(account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        raw_message,
        logic_timer_->logic_time()
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);

    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, auditor_to_associated_validator_2) {
    xvnetwork_message_t message{
        build_auditor_node_address(account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        build_validator_node_address(account_pubkey_validator1,
                                     common::xbroadcast_id_t::slot,
                                     common::xlogic_epoch_t{}),
        raw_message,
        logic_timer_->logic_time()
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);

    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, auditor_to_associated_validator_3) {
    xvnetwork_message_t message{
        build_auditor_node_address(account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_2),
        build_validator_node_address(account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        raw_message,
        logic_timer_->logic_time()
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::epoch_mismatch);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::epoch_mismatch);
}

TEST_F(test_message_filter, auditor_to_associated_validator_4) {
    xvnetwork_message_t message{
        build_auditor_node_address(unknown_account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        build_validator_node_address(unknown_account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        raw_message,
        xlogic_time_t{0}
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, auditor_to_associated_validator_5) {
    xvnetwork_message_t message{
        build_auditor_node_address(unknown_account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        build_validator_node_address(unknown_account_pubkey_validator1,
                                     common::xbroadcast_id_t::slot,
                                     common::xlogic_epoch_t{}),
        raw_message,
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().empty());
}

TEST_F(test_message_filter, auditor_to_associated_validator_6) {
    xvnetwork_message_t message{
        build_auditor_node_address(unknown_account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_2),
        build_validator_node_address(unknown_account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        raw_message,
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, validator_to_non_associated_auditor_1) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator3,
                                     common::xslot_id_t{0},
                                     con_epoch_2),
        build_auditor_node_address(account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_1),
        raw_message,
        xlogic_time_t{0}
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_auditor1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, validator_to_non_associated_auditor_2) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator3,
                                     common::xslot_id_t{0},
                                     con_epoch_2),
        build_auditor_node_address(account_pubkey_auditor1,
                                   common::xbroadcast_id_t::slot,
                                   common::xlogic_epoch_t{}),
        raw_message,
        logic_timer_->logic_time()
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_auditor1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);


    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().has_value());
}

TEST_F(test_message_filter, validator_to_non_associated_auditor_3) {
    xvnetwork_message_t message{
        build_validator_node_address(unknown_account_pubkey_validator3,
                                     common::xslot_id_t{0},
                                     con_epoch_2),
        build_auditor_node_address(unknown_account_pubkey_auditor1,
                                   common::xslot_id_t{0},
                                   con_epoch_2),
        raw_message,
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().has_value());
}

TEST_F(test_message_filter, validator_to_validator_same_associated_auditor_1) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator2,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        build_validator_node_address(account_pubkey_validator1,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        raw_message,
        xlogic_time_t{0}
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);

    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().has_value());
}

TEST_F(test_message_filter, validator_to_validator_same_associated_auditor_2) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator2,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        build_validator_node_address(account_pubkey_validator1,
                                     common::xbroadcast_id_t::slot,
                                     common::xlogic_epoch_t{}),
        raw_message,
        xlogic_time_t{0}
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_TRUE(!ec);

    filter_mgr_->filter_message(message, ec);
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().has_value());
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_1);
}

TEST_F(test_message_filter, validator_to_validator_same_associated_auditor_3) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator2,
                                     common::xslot_id_t{0},
                                     con_epoch_2),
        build_validator_node_address(account_pubkey_validator1,
                                     common::xbroadcast_id_t::slot,
                                     common::xlogic_epoch_t{}),
        raw_message,
        logic_timer_->logic_time()
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().empty());

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().empty());
}

TEST_F(test_message_filter, validator_to_validator_same_associated_auditor_4) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator2,
                                     common::xslot_id_t{0},
                                     con_epoch_1),
        build_validator_node_address(account_pubkey_validator1,
                                     common::xbroadcast_id_t::slot,
                                     con_epoch_2),
        raw_message,
        xlogic_time_t{0}
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_2);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_2);
}

TEST_F(test_message_filter, validator_to_validator_same_associated_auditor_5) {
    xvnetwork_message_t message{
        build_validator_node_address(account_pubkey_validator2,
                                     common::xslot_id_t{0},
                                     con_epoch_2),
        build_validator_node_address(account_pubkey_validator1,
                                     common::xbroadcast_id_t::slot,
                                     con_epoch_2),
        raw_message,
        xlogic_time_t{0}
    };

    std::dynamic_pointer_cast<top::tests::network::xdummy_network_driver_t>(this->network_driver_)->account_address(account_pubkey_validator1.account);

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_EQ(top::vnetwork::xfilter_result_t::stop_filtering, filter.filter(message, ec));
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_2);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_2);
}

NS_END3
