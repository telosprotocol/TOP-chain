// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnetwork/xvnetwork_fixture.h"

#include "xvnetwork/xvhost.h"
#include "xvnetwork/xvnetwork_error2.h"

NS_BEG3(top, tests, vnetwork)

void xtop_vnetwork_fixture2::add_rec(data::election::xelection_result_store_t & election_result_store,
                                     common::xlogic_time_t const timestamp,
                                     common::xlogic_time_t const start_time,
                                     common::xelection_round_t const & group_version,
                                     xaccount_data_bundle_t const & rec) {
    auto & rec_group = election_result_store.result_of(common::xtestnet_id).result_of(common::xnode_type_t::rec).result_of(common::xcommittee_cluster_id).result_of(rec.group_id);

    rec_group.election_committee_version(common::xelection_round_t{0});
    rec_group.timestamp(timestamp);
    rec_group.start_time(start_time);
    rec_group.group_version(group_version);

    data::election::xelection_info_t election_info{};
    election_info.joined_version = group_version;
    election_info.stake = 0;
    election_info.consensus_public_key = rec.public_key;

    data::election::xelection_info_bundle_t election_info_bundle{};
    election_info_bundle.account_address(rec.account);
    election_info_bundle.election_info(std::move(election_info));

    rec_group.insert(std::move(election_info_bundle));
}

void xtop_vnetwork_fixture2::add_zec(data::election::xelection_result_store_t & election_result_store,
                                     common::xlogic_time_t const timestamp,
                                     common::xlogic_time_t const start_time,
                                     common::xelection_round_t const & group_version,
                                     xaccount_data_bundle_t const & zec) {
    auto & zec_group = election_result_store.result_of(common::xtestnet_id).result_of(common::xnode_type_t::zec).result_of(common::xcommittee_cluster_id).result_of(zec.group_id);

    zec_group.election_committee_version(common::xelection_round_t{0});
    zec_group.timestamp(timestamp);
    zec_group.start_time(start_time);
    zec_group.group_version(group_version);

    data::election::xelection_info_t election_info{};
    election_info.joined_version = group_version;
    election_info.stake = 0;
    election_info.consensus_public_key = zec.public_key;

    data::election::xelection_info_bundle_t election_info_bundle{};
    election_info_bundle.account_address(zec.account);
    election_info_bundle.election_info(std::move(election_info));

    zec_group.insert(std::move(election_info_bundle));
}

void xtop_vnetwork_fixture2::add_auditor_validator(data::election::xelection_result_store_t & election_result_store,
                                                   common::xlogic_time_t const timestamp,
                                                   common::xlogic_time_t const start_time,
                                                   common::xelection_round_t const & group_version,
                                                   xaccount_data_bundle_t const & auditor,
                                                   std::vector<xaccount_data_bundle_t> const & validators) {
    {
        auto & auditor_group = election_result_store.result_of(common::xtestnet_id)
                                                    .result_of(common::xnode_type_t::consensus_auditor)
                                                    .result_of(common::xdefault_cluster_id)
                                                    .result_of(auditor.group_id);

        auditor_group.election_committee_version(common::xelection_round_t{0});
        auditor_group.timestamp(timestamp);
        auditor_group.start_time(start_time);
        auditor_group.group_version(group_version);

        data::election::xelection_info_t election_info{};
        election_info.joined_version = group_version;
        election_info.stake = 0;
        election_info.consensus_public_key = auditor.public_key;

        data::election::xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.account_address(auditor.account);
        election_info_bundle.election_info(std::move(election_info));

        auditor_group.insert(std::move(election_info_bundle));
    }

    for (auto i = 0u; i < validators.size(); ++i) {
        auto const & validator = validators[i];
        auto const & validator_group_id = validator.group_id;

        auto & validator_group = election_result_store.result_of(common::xtestnet_id)
                                                      .result_of(common::xnode_type_t::consensus_validator)
                                                      .result_of(common::xdefault_cluster_id)
                                                      .result_of(validator_group_id);

        validator_group.election_committee_version(common::xelection_round_t{0});
        validator_group.timestamp(timestamp);
        validator_group.start_time(start_time);
        validator_group.group_version(group_version);

        validator_group.associated_group_id(auditor.group_id);
        // validator_group.cluster_version(association_cluster_result.cluster_version());
        validator_group.associated_group_version(group_version);

        data::election::xelection_info_t election_info{};
        election_info.joined_version = group_version;
        election_info.stake = 0;
        election_info.consensus_public_key = validator.public_key;

        data::election::xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.account_address(validator.account);
        election_info_bundle.election_info(std::move(election_info));

        validator_group.insert(std::move(election_info_bundle));
    }
}

std::shared_ptr<top::vnetwork::xvhost_face_t> xtop_vnetwork_fixture2::create_vhost() const {
    return std::make_shared<top::vnetwork::xvhost_t>(
        make_observer(network_driver_.get()), make_observer(logic_timer_.get()), common::xtestnet_id, make_observer(data_accessor_.get()));
}

void xtop_vnetwork_fixture2::SetUp() {
    logic_timer_ = create_logic_chain_timer();
    network_driver_ = create_netwrok_driver();
    data_accessor_ = create_election_data_accessor();
    vhost_ = create_vhost();

    // +------------------------------------------------------------------------------------------------------------------------------------------------+
    // |                                                                   epoch 1                                                                      |
    // +------------------------------------------------------------------------------------------------------------------------------------------------+
    // | rec    | account_pubkey_rec1                                                                                                                   |
    // | zec    | account_pubkey_zec1                                                                                                                   |
    // +-----------------------------------------------------------------------+------------------------------------------------------------------------+
    // |                   account_pubkey_auditor1                             |                            account_pubkey_auditor2                     |
    // |                       /     |      \                                  |                                /      |   \                            |
    // |             /               |             \                           |                          /            |           \                    |
    // | account_pubkey_validator1   |   account_pubkey_validator2             |             account_pubkey_validator3 | account_pubkey_validator4      |
    // +-----------------------------+-----------------------------------------+---------------------------------------+--------------------------------+
    auto const epoch_1_timestamp = logic_timer_->logic_time();
    auto const epoch_1_start_time = epoch_1_timestamp;

    data::election::xelection_result_store_t election_result_store_epoch_1;
    add_rec(election_result_store_epoch_1, epoch_1_timestamp, epoch_1_start_time, logic_epoch_1_version, account_pubkey_rec1);
    add_zec(election_result_store_epoch_1, epoch_1_timestamp, epoch_1_start_time, logic_epoch_1_version, account_pubkey_zec1);
    add_auditor_validator(election_result_store_epoch_1,
                          epoch_1_timestamp,
                          epoch_1_start_time,
                          logic_epoch_1_version,
                          account_pubkey_auditor1,
                          {account_pubkey_validator1, account_pubkey_validator2});

    add_auditor_validator(election_result_store_epoch_1,
                          epoch_1_timestamp,
                          epoch_1_start_time,
                          logic_epoch_1_version,
                          account_pubkey_auditor2,
                          {account_pubkey_validator3, account_pubkey_validator4});

    // +------------------------------------------------------------------------------------------------------------------------------------------------+
    // |                                                                   epoch 2                                                                      |
    // +------------------------------------------------------------------------------------------------------------------------------------------------+
    // | rec    | account_pubkey_rec2                                                                                                                   |
    // | zec    | account_pubkey_zec2                                                                                                                   |
    // +-----------------------------------------------------------------------+------------------------------------------------------------------------+
    // |                   account_pubkey_auditor3                             |                            account_pubkey_auditor4                     |
    // |                       /     |      \                                  |                                /      |   \                            |
    // |             /               |             \                           |                          /            |           \                    |
    // | account_pubkey_validator5   |   account_pubkey_validator6             |             account_pubkey_validator7 | account_pubkey_validator8      |
    // +-----------------------------+-----------------------------------------+---------------------------------------+--------------------------------+

    auto const epoch_2_timestamp = epoch_1_timestamp + 10;
    auto const epoch_2_rec_start_time = epoch_2_timestamp + 10;
    auto const epoch_2_zec_start_time = epoch_2_timestamp + 20;
    auto const epoch_2_auditor_validator_356 = epoch_2_timestamp + 30;
    auto const epoch_2_auditor_validator_478 = epoch_2_timestamp + 40;

    data::election::xelection_result_store_t election_result_store_epoch_2;
    add_rec(election_result_store_epoch_2, epoch_2_timestamp, epoch_2_rec_start_time, logic_epoch_2_version, account_pubkey_rec2);
    add_zec(election_result_store_epoch_2, epoch_2_timestamp, epoch_2_zec_start_time, logic_epoch_2_version, account_pubkey_zec2);
    add_auditor_validator(election_result_store_epoch_2,
                          epoch_2_timestamp,
                          epoch_2_auditor_validator_356,
                          logic_epoch_2_version,
                          account_pubkey_auditor3,
                          {account_pubkey_validator5, account_pubkey_validator6});
    add_auditor_validator(election_result_store_epoch_2,
                          epoch_2_timestamp,
                          epoch_2_auditor_validator_478,
                          logic_epoch_2_version,
                          account_pubkey_auditor4,
                          {account_pubkey_validator7, account_pubkey_validator8});

    std::error_code ec;
    data_accessor_->update_zone(common::xcommittee_zone_id, election_result_store_epoch_1, logic_epoch_1_blk_height, ec);
    ASSERT_TRUE(!ec);
    data_accessor_->update_zone(common::xzec_zone_id, election_result_store_epoch_1, logic_epoch_1_blk_height, ec);
    ASSERT_TRUE(!ec);
    data_accessor_->update_zone(common::xconsensus_zone_id, election_result_store_epoch_1, logic_epoch_1_blk_height, ec);
    ASSERT_TRUE(!ec);

    data_accessor_->update_zone(common::xcommittee_zone_id, election_result_store_epoch_2, logic_epoch_2_blk_height, ec);
    ASSERT_TRUE(!ec);
    data_accessor_->update_zone(common::xzec_zone_id, election_result_store_epoch_2, logic_epoch_2_blk_height, ec);
    ASSERT_TRUE(!ec);
    data_accessor_->update_zone(common::xconsensus_zone_id, election_result_store_epoch_2, logic_epoch_2_blk_height, ec);
    ASSERT_TRUE(!ec);

    logic_timer_->start();
    network_driver_->start();
    vhost_->start();
}

void xtop_vnetwork_fixture2::TearDown() {
    vhost_->stop();

    vhost_.reset();
    data_accessor_.reset();
    network_driver_->stop();
    logic_timer_->stop();
}

NS_END3
