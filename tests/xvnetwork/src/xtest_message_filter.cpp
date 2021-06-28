// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xnetwork/xdummy_network_driver.h"
#include "tests/xvnetwork/xdummy_chain_timer.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "tests/xvnetwork/xtest_vhost_fixture.h"
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
using top::common::xversion_t;
using top::common::xzone_id_t;
using top::vnetwork::xmessage_filter_manager_t;
using top::vnetwork::xmessage_t;
using top::vnetwork::xvnetwork_message_t;
using top::vnetwork::xvnetwork_message_type_t;

NS_BEG3(top, tests, vnetwork)

uint64_t const logic_epoch_1_blk_height{ 0 };
uint64_t const logic_epoch_2_blk_height{ 1 };

common::xlogic_epoch_t const rec_epoch_1{ 1, logic_epoch_1_blk_height };
common::xlogic_epoch_t const rec_epoch_2{ 1, logic_epoch_2_blk_height };
common::xlogic_epoch_t const zec_epoch_1{ 1, logic_epoch_1_blk_height };
common::xlogic_epoch_t const zec_epoch_2{ 1, logic_epoch_2_blk_height };
common::xlogic_epoch_t const con_epoch_1{ 1, logic_epoch_1_blk_height };
common::xlogic_epoch_t const con_epoch_2{ 1, logic_epoch_2_blk_height };
common::xlogic_epoch_t const arc_epoch_1{ 1, logic_epoch_1_blk_height };
common::xlogic_epoch_t const arc_epoch_2{ 1, logic_epoch_2_blk_height };
common::xlogic_epoch_t const edg_epoch_1{ 1, logic_epoch_1_blk_height };
common::xlogic_epoch_t const edg_epoch_2{ 1, logic_epoch_2_blk_height };

common::xversion_t const logic_epoch_1_version{ 0 };
common::xversion_t const logic_epoch_2_version{ 1 };

struct xtop_account_data_bundle {
    common::xaccount_address_t account;
    xpublic_key_t public_key;
    common::xgroup_id_t group_id;

    xtop_account_data_bundle(xtop_account_data_bundle const &) = default;
    xtop_account_data_bundle(xtop_account_data_bundle &&) = default;
    xtop_account_data_bundle & operator=(xtop_account_data_bundle const &) = default;
    xtop_account_data_bundle & operator=(xtop_account_data_bundle &&) = default;
    ~xtop_account_data_bundle() = default;

    xtop_account_data_bundle(std::string account_string,
                             std::string pubkey_string,
                             common::xgroup_id_t gid)
        : account{ std::move(account_string) }, public_key{ std::move(pubkey_string) }, group_id{ std::move(gid) } {
    }
};
using xaccount_data_bundle_t = xtop_account_data_bundle;

xaccount_data_bundle_t const account_pubkey_rec1{ "T00000LcUgUwZaZSd33Zjcd1C3Ht7wRCjptg6xzS", "BE+kB7LJMrX28C1PA3tcNksXrSOq4GaNIaia97kKZZ4IkJQmLwFeTnvvsmx0Njo2qhbjKnd6ZChKt3UfNCmJfKE=", common::xcommittee_group_id };
xaccount_data_bundle_t const account_pubkey_rec2{ "T00000LYU9DnWdDbqfFJJAeNXhvasjpc6dAfmDcH", "BDv+A5IKcXpkUsk8113UnFFYByCUctRNm7/03dcGsH2iukxXM7YftHTblKXGVd3hXb3U1rrCj002xG5RxFMU5EQ=", common::xcommittee_group_id };
xaccount_data_bundle_t const account_pubkey_zec1{ "T00000LbNqFnNw9sUNzCMkkaPajVuSDbVt78SovU", "BHYR3i2Ey2IwXpNDrQzpn31+JyJJuHK/AlF3XzT4NbNiKLHk5BCGwXF49gc0ohBIWm6fxGxZoDYHklVJ1IME+kI=", common::xcommittee_group_id };
xaccount_data_bundle_t const account_pubkey_zec2{ "T00000LcVdbKvxjKzDEJv54UH2U5Cg1a46HxU5jJ", "BKdqc2/gmmV93vBRFtD6hXKFvnbt0+uaxiboz5q8NEcol9VrnHTDZQpxzNFTA6DMWn2pPgPTOOmvceSgd1IBeKs=", common::xcommittee_group_id };

xaccount_data_bundle_t const account_pubkey_auditor1{ "T00000LaSpXSpj81nh6AGd4RXMFcSCUagdfL3Mya", "BDmEkMyasFb07c/XIjaQJzN7eU8RfseNFUvVjcOaArgojxZz9W5eNTfkonVzHpur5njX6tRX3KXq8MFXfKHDbYY=", common::xgroup_id_t{common::xauditor_group_id_value_begin} };
xaccount_data_bundle_t const account_pubkey_auditor2{ "T00000Le7aYQwY3dXcj9SfbJWa7uzH8EqzjqyrM4", "BOnR9NUP9XMk4NtT+k6jG0V1SEuTAnsrDYKiwgwRsuvabCAYOWpvPb4rAxDX82A8OJTLHag3NZfC20Mq8VnXunQ=", common::xgroup_id_t{common::xauditor_group_id_value_begin + 1} };
xaccount_data_bundle_t const account_pubkey_auditor3{ "T00000LTZQNJXhEomDFjBWejfv1nSjhaFzYLPac7", "BCwn7Fyc5UcD2G+/fmVc0mfnpwJp+3XnKWft+rvlbLPwbk71C0zXTyY2vyk+hlY3uNi72O0hFwNygOSI7r0Pqr0=", common::xgroup_id_t{common::xauditor_group_id_value_begin} };
xaccount_data_bundle_t const account_pubkey_auditor4{ "T00000LMizP6araEVWkyiBtVdFyB9po4aBMX6N3X", "BCVS6OhkoQOIWeZJIBwnCm09hbwVoYUefDLh+tC1in1hpiseg6d7TrpvW+6c9kWCaVcxmchKeR7/LWGJlpk+O6Y=", common::xgroup_id_t{common::xauditor_group_id_value_begin + 1} };

xaccount_data_bundle_t const account_pubkey_validator1{ "T00000LaSck6QhQi1m9qftdVJ1UB6qVceLwh78kT", "BJXR/P4ridigvmkzVz+zBvz7Rq0FWKa2+SoxljY6Ec7J/kYTCoARuWlFlmmBC0yQ+GGnYybsiXf/abF9ztgCqlg=", common::xgroup_id_t{common::xvalidator_group_id_value_begin} };
xaccount_data_bundle_t const account_pubkey_validator2{ "T00000LNLPsxqAsPVQVbeY2wvQYjBA3PrgDG4VZG", "BITWoMosu6gQghgE4dyMqg/neq49J3XA34RzVWg3CiYILj/Vni0IeQ1n9RlR181xN9MCQgBojYzBmUle/MiyRJY=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 1} };
xaccount_data_bundle_t const account_pubkey_validator3{ "T00000LcDUW3zwybcsT9uMTueaNxearZngTMHo58", "BChaDdjtSL4EA05lBqZgOborgcuEExik9l8WLCKd6B3nnd/uCYaiqg9VrK5e5NA+9VRZOz0+wAMcKDuMUL6RgyU=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 2} };
xaccount_data_bundle_t const account_pubkey_validator4{ "T00000LSs9ST9JQA2zn5EwutcEct5MhGvkQS6Nzr", "BE4Oj3i/K+ptz0AaRuPCnaufIkwXpG6EGDxALSmyynVg+m80NMk2pB50JXjRWDv5JhK76aY1c9lZbLFLXMlhBsk=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 3} };
xaccount_data_bundle_t const account_pubkey_validator5{ "T00000LWUMxH125uF6UuAzzHduBB1kn68S7HnUJt", "BAo/TbJ//1+EKc3njAQKAk6dqnN4986q/9EWnueV1OqDUNSSam5Vclr5Wh8OMQS7/gWTnYOpF8idYWnR4AOg5MQ=", common::xgroup_id_t{common::xvalidator_group_id_value_begin} };
xaccount_data_bundle_t const account_pubkey_validator6{ "T00000Lb8N4rsH2BLdDeKDChvURZSVKKmUjgecak", "BPJ1RQWCbO3ZXnWVU85YD7xsg6dd3yeQZ80xJlV698znblPhtnVnAoSrreMOMokzIntJIEJoIIW9v8If/ZtesnA=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 1} };
xaccount_data_bundle_t const account_pubkey_validator7{ "T00000LdZssB2ayjJyVr94qprUjbw6LuRBreFzQn", "BJpAFgA6EFM3GJTh7aqVjYAsv7KbdYzThNzwze3z0j8htze9CBqomOSSwyjQ/NZ/aTMRD31ccsHJG8E4eIMyI8Y=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 2} };
xaccount_data_bundle_t const account_pubkey_validator8{ "T00000LLTaHKwki5i6QsxzvLCvdjDsv2v3Kcxqu2", "BGkwfXHAW/YuQRzneuSdfgd2nsH6xIglo12V9LMQXIQNDiFbJmtB8v4BWCKR9WoR9K6g+RhlmUbJiwvUkZgO0UA=", common::xgroup_id_t{common::xvalidator_group_id_value_begin + 3} };

class test_message_filter : public testing::Test {
protected:
    std::unique_ptr<election::cache::xdata_accessor_face_t> data_accessor_;
    std::shared_ptr<top::vnetwork::xvhost_t> vhost_;
    std::unique_ptr<top::vnetwork::xmessage_filter_manager_face_t> filter_mgr_;

public:
    test_message_filter() = default;
    test_message_filter(test_message_filter const &) = delete;
    test_message_filter(test_message_filter &&) = default;
    test_message_filter & operator=(test_message_filter const &) = delete;
    test_message_filter & operator=(test_message_filter &&) = default;
    ~test_message_filter() override = default;

private:
    void add_rec(data::election::xelection_result_store_t & election_result_store,
                 common::xlogic_time_t const timestamp,
                 common::xlogic_time_t const start_time,
                 common::xversion_t const & group_version,
                 xaccount_data_bundle_t const & rec) {
        auto & rec_group = election_result_store.result_of(common::xtestnet_id).result_of(common::xnode_type_t::rec).result_of(common::xcommittee_cluster_id).result_of(rec.group_id);

        rec_group.election_committee_version(common::xversion_t{ 0 });
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

    void add_zec(data::election::xelection_result_store_t & election_result_store,
                 common::xlogic_time_t const timestamp,
                 common::xlogic_time_t const start_time,
                 common::xversion_t const & group_version,
                 xaccount_data_bundle_t const & zec) {
        auto & zec_group = election_result_store.result_of(common::xtestnet_id).result_of(common::xnode_type_t::zec).result_of(common::xcommittee_cluster_id).result_of(zec.group_id);

        zec_group.election_committee_version(common::xversion_t{ 0 });
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

    void add_auditor_validator(data::election::xelection_result_store_t & election_result_store,
                               common::xlogic_time_t const timestamp,
                               common::xlogic_time_t const start_time,
                               common::xversion_t const & group_version,
                               xaccount_data_bundle_t const & auditor,
                               std::vector<xaccount_data_bundle_t> const & validators) {
        {
            auto & auditor_group = election_result_store.result_of(common::xtestnet_id).result_of(common::xnode_type_t::consensus_auditor).result_of(common::xdefault_cluster_id).result_of(auditor.group_id);

            auditor_group.election_committee_version(common::xversion_t{ 0 });
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

            auto & validator_group = election_result_store.result_of(common::xtestnet_id).result_of(common::xnode_type_t::consensus_validator).result_of(common::xdefault_cluster_id).result_of(validator_group_id);

            validator_group.election_committee_version(common::xversion_t{ 0 });
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
public:

    void SetUp() override {
        data_accessor_ = top::make_unique<election::cache::xdata_accessor_t>(common::xtestnet_id, make_observer(tests::vnetwork::xdummy_chain_timer));

        vhost_ = std::make_shared<top::vnetwork::xvhost_t>(make_observer(&tests::network::xdummy_network_driver),
                                                           make_observer(tests::vnetwork::xdummy_chain_timer),
                                                           common::xtestnet_id,
                                                           make_observer(data_accessor_.get()));

        filter_mgr_ = top::make_unique<top::vnetwork::xmessage_filter_manager_t>(make_observer(vhost_.get()), make_observer(data_accessor_.get()));

        data::election::xelection_result_store_t election_result_store_epoch_1;
        add_rec(election_result_store_epoch_1, 0, 0, logic_epoch_1_version, account_pubkey_rec1);
        add_zec(election_result_store_epoch_1, 0, 0, logic_epoch_1_version, account_pubkey_zec1);
        add_auditor_validator(election_result_store_epoch_1,
                              0, 0, logic_epoch_1_version,
                              account_pubkey_auditor1,
                              { account_pubkey_validator1, account_pubkey_validator2 });

        add_auditor_validator(election_result_store_epoch_1,
                              0, 0, logic_epoch_1_version,
                              account_pubkey_auditor2,
                              { account_pubkey_validator3, account_pubkey_validator4 });

        data::election::xelection_result_store_t election_result_store_epoch_2;
        add_rec(election_result_store_epoch_2, 10, 20, logic_epoch_2_version, account_pubkey_rec2);
        add_zec(election_result_store_epoch_2, 5, 10, logic_epoch_2_version, account_pubkey_zec2);
        add_auditor_validator(election_result_store_epoch_2,
                              1, 3, logic_epoch_2_version,
                              account_pubkey_auditor3,
                              { account_pubkey_validator5, account_pubkey_validator6 });

        add_auditor_validator(election_result_store_epoch_2,
                              2, 4, logic_epoch_2_version,
                              account_pubkey_auditor4,
                              { account_pubkey_validator7, account_pubkey_validator8 });

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

        vhost_->start();
        filter_mgr_->start();
    }

    void TearDown() override {
        filter_mgr_->stop();
        vhost_->stop();

        filter_mgr_.reset();
        vhost_.reset();
        data_accessor_.reset();
    };

    observer_ptr<top::vnetwork::xvhost_face_t> vhost() const noexcept {
        return top::make_observer<top::vnetwork::xvhost_face_t>(vhost_.get());
    }

    observer_ptr<top::election::cache::xdata_accessor_face_t> data_accessor() const noexcept {
        return top::make_observer<top::election::cache::xdata_accessor_face_t>(data_accessor_.get());
    }
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

common::xnode_address_t build_data_node_address(common::xaccount_address_t account_address,
                                                common::xslot_id_t slot_id,
                                                common::xlogic_epoch_t logic_epoch,
                                                common::xgroup_id_t data_group_id) {
    if (data_group_id != common::xarchive_group_id && data_group_id != common::xfull_node_group_id) {
        assert(false);
        return{};
    }
    return build_node_address(std::move(account_address),
                              std::move(slot_id),
                              std::move(logic_epoch),
                              common::build_archive_sharding_address(data_group_id, common::xtestnet_id));
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
    EXPECT_FALSE(filter.filter(empty_message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::empty_message);

    ec.clear();
    filter_mgr_->filter_message(empty_message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::empty_message);
}

TEST_F(test_message_filter, invalid_sender_no_account_address) {
    xvnetwork_message_t message{ common::xnode_address_t{}, common::xnode_address_t{}, raw_message, 0 };
    std::error_code ec;
    top::vnetwork::xtop_message_filter_sender const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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
    EXPECT_FALSE(filter.filter(message, ec));
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
    EXPECT_FALSE(filter.filter(message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_src_address);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_src_address);
}

TEST_F(test_message_filter, recver_broadcast) {
    xvnetwork_message_t message{
        build_rec_node_address(common::xaccount_address_t{"T00000LQ3ammXf22Y9RjDSDetF67DjVNiGK3HeL8"}, common::xslot_id_t{common::xmax_slot_id_value}, rec_epoch_1),
        common::xnode_address_t{},
        raw_message,
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver const filter{ vhost(), data_accessor() };
    EXPECT_TRUE(filter.filter(message, ec));
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
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_dst_address);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_EQ(ec, top::vnetwork::xvnetwork_errc2_t::invalid_dst_address);
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
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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
    EXPECT_FALSE(filter.filter(message, ec));
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
    EXPECT_FALSE(filter.filter(message, ec));
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
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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
    EXPECT_FALSE(filter.filter(message, ec));
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
    EXPECT_FALSE(filter.filter(message, ec));
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

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_auditor const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
    EXPECT_FALSE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().empty());

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_TRUE(message.receiver().logic_epoch().empty());
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
    EXPECT_FALSE(filter.filter(message, ec));
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

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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
        xlogic_time_t{0}
    };

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
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

    std::error_code ec;
    top::vnetwork::xtop_message_filter_recver_is_validator const filter{ vhost(), data_accessor() };
    EXPECT_FALSE(filter.filter(message, ec));
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_2);

    ec.clear();
    filter_mgr_->filter_message(message, ec);
    EXPECT_FALSE(!ec);
    EXPECT_EQ(message.receiver().logic_epoch(), con_epoch_2);
}

NS_END3
