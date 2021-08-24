// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbasic/xcrypto_key.h"
#include "xcommon/xaddress.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xversion.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xnetwork/xnetwork_driver_face.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvhost_face.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <list>
#include <memory>
#include <string>

NS_BEG3(top, tests, vnetwork)

uint64_t const logic_epoch_1_blk_height{0};
uint64_t const logic_epoch_2_blk_height{1};

common::xlogic_epoch_t const rec_epoch_1{1, logic_epoch_1_blk_height};
common::xlogic_epoch_t const rec_epoch_2{1, logic_epoch_2_blk_height};
common::xlogic_epoch_t const zec_epoch_1{1, logic_epoch_1_blk_height};
common::xlogic_epoch_t const zec_epoch_2{1, logic_epoch_2_blk_height};
common::xlogic_epoch_t const con_epoch_1{1, logic_epoch_1_blk_height};
common::xlogic_epoch_t const con_epoch_2{1, logic_epoch_2_blk_height};
common::xlogic_epoch_t const arc_epoch_1{1, logic_epoch_1_blk_height};
common::xlogic_epoch_t const arc_epoch_2{1, logic_epoch_2_blk_height};
common::xlogic_epoch_t const edg_epoch_1{1, logic_epoch_1_blk_height};
common::xlogic_epoch_t const edg_epoch_2{1, logic_epoch_2_blk_height};

common::xelection_round_t const logic_epoch_1_version{0};
common::xelection_round_t const logic_epoch_2_version{1};

struct xtop_account_data_bundle {
    common::xaccount_address_t account;
    xpublic_key_t public_key;
    common::xgroup_id_t group_id;

    xtop_account_data_bundle(xtop_account_data_bundle const &) = default;
    xtop_account_data_bundle(xtop_account_data_bundle &&) = default;
    xtop_account_data_bundle & operator=(xtop_account_data_bundle const &) = default;
    xtop_account_data_bundle & operator=(xtop_account_data_bundle &&) = default;
    ~xtop_account_data_bundle() = default;

    xtop_account_data_bundle(std::string account_string, std::string pubkey_string, common::xgroup_id_t gid)
      : account{std::move(account_string)}, public_key{std::move(pubkey_string)}, group_id{std::move(gid)} {
    }
};
using xaccount_data_bundle_t = xtop_account_data_bundle;

xaccount_data_bundle_t const account_pubkey_rec1{"T00000LcUgUwZaZSd33Zjcd1C3Ht7wRCjptg6xzS",
                                                 "BE+kB7LJMrX28C1PA3tcNksXrSOq4GaNIaia97kKZZ4IkJQmLwFeTnvvsmx0Njo2qhbjKnd6ZChKt3UfNCmJfKE=",
                                                 common::xcommittee_group_id};
xaccount_data_bundle_t const account_pubkey_rec2{"T00000LYU9DnWdDbqfFJJAeNXhvasjpc6dAfmDcH",
                                                 "BDv+A5IKcXpkUsk8113UnFFYByCUctRNm7/03dcGsH2iukxXM7YftHTblKXGVd3hXb3U1rrCj002xG5RxFMU5EQ=",
                                                 common::xcommittee_group_id};
xaccount_data_bundle_t const account_pubkey_zec1{"T00000LbNqFnNw9sUNzCMkkaPajVuSDbVt78SovU",
                                                 "BHYR3i2Ey2IwXpNDrQzpn31+JyJJuHK/AlF3XzT4NbNiKLHk5BCGwXF49gc0ohBIWm6fxGxZoDYHklVJ1IME+kI=",
                                                 common::xcommittee_group_id};
xaccount_data_bundle_t const account_pubkey_zec2{"T00000LcVdbKvxjKzDEJv54UH2U5Cg1a46HxU5jJ",
                                                 "BKdqc2/gmmV93vBRFtD6hXKFvnbt0+uaxiboz5q8NEcol9VrnHTDZQpxzNFTA6DMWn2pPgPTOOmvceSgd1IBeKs=",
                                                 common::xcommittee_group_id};

xaccount_data_bundle_t const account_pubkey_auditor1{"T00000LaSpXSpj81nh6AGd4RXMFcSCUagdfL3Mya",
                                                     "BDmEkMyasFb07c/XIjaQJzN7eU8RfseNFUvVjcOaArgojxZz9W5eNTfkonVzHpur5njX6tRX3KXq8MFXfKHDbYY=",
                                                     common::xgroup_id_t{common::xauditor_group_id_value_begin}};
xaccount_data_bundle_t const account_pubkey_auditor2{"T00000Le7aYQwY3dXcj9SfbJWa7uzH8EqzjqyrM4",
                                                     "BOnR9NUP9XMk4NtT+k6jG0V1SEuTAnsrDYKiwgwRsuvabCAYOWpvPb4rAxDX82A8OJTLHag3NZfC20Mq8VnXunQ=",
                                                     common::xgroup_id_t{common::xauditor_group_id_value_begin + 1}};
xaccount_data_bundle_t const account_pubkey_auditor3{"T00000LTZQNJXhEomDFjBWejfv1nSjhaFzYLPac7",
                                                     "BCwn7Fyc5UcD2G+/fmVc0mfnpwJp+3XnKWft+rvlbLPwbk71C0zXTyY2vyk+hlY3uNi72O0hFwNygOSI7r0Pqr0=",
                                                     common::xgroup_id_t{common::xauditor_group_id_value_begin}};
xaccount_data_bundle_t const account_pubkey_auditor4{"T00000LMizP6araEVWkyiBtVdFyB9po4aBMX6N3X",
                                                     "BCVS6OhkoQOIWeZJIBwnCm09hbwVoYUefDLh+tC1in1hpiseg6d7TrpvW+6c9kWCaVcxmchKeR7/LWGJlpk+O6Y=",
                                                     common::xgroup_id_t{common::xauditor_group_id_value_begin + 1}};

xaccount_data_bundle_t const account_pubkey_validator1{"T00000LaSck6QhQi1m9qftdVJ1UB6qVceLwh78kT",
                                                       "BJXR/P4ridigvmkzVz+zBvz7Rq0FWKa2+SoxljY6Ec7J/kYTCoARuWlFlmmBC0yQ+GGnYybsiXf/abF9ztgCqlg=",
                                                       common::xgroup_id_t{common::xvalidator_group_id_value_begin}};
xaccount_data_bundle_t const account_pubkey_validator2{"T00000LNLPsxqAsPVQVbeY2wvQYjBA3PrgDG4VZG",
                                                       "BITWoMosu6gQghgE4dyMqg/neq49J3XA34RzVWg3CiYILj/Vni0IeQ1n9RlR181xN9MCQgBojYzBmUle/MiyRJY=",
                                                       common::xgroup_id_t{common::xvalidator_group_id_value_begin + 1}};
xaccount_data_bundle_t const account_pubkey_validator3{"T00000LcDUW3zwybcsT9uMTueaNxearZngTMHo58",
                                                       "BChaDdjtSL4EA05lBqZgOborgcuEExik9l8WLCKd6B3nnd/uCYaiqg9VrK5e5NA+9VRZOz0+wAMcKDuMUL6RgyU=",
                                                       common::xgroup_id_t{common::xvalidator_group_id_value_begin + 2}};
xaccount_data_bundle_t const account_pubkey_validator4{"T00000LSs9ST9JQA2zn5EwutcEct5MhGvkQS6Nzr",
                                                       "BE4Oj3i/K+ptz0AaRuPCnaufIkwXpG6EGDxALSmyynVg+m80NMk2pB50JXjRWDv5JhK76aY1c9lZbLFLXMlhBsk=",
                                                       common::xgroup_id_t{common::xvalidator_group_id_value_begin + 3}};
xaccount_data_bundle_t const account_pubkey_validator5{"T00000LWUMxH125uF6UuAzzHduBB1kn68S7HnUJt",
                                                       "BAo/TbJ//1+EKc3njAQKAk6dqnN4986q/9EWnueV1OqDUNSSam5Vclr5Wh8OMQS7/gWTnYOpF8idYWnR4AOg5MQ=",
                                                       common::xgroup_id_t{common::xvalidator_group_id_value_begin}};
xaccount_data_bundle_t const account_pubkey_validator6{"T00000Lb8N4rsH2BLdDeKDChvURZSVKKmUjgecak",
                                                       "BPJ1RQWCbO3ZXnWVU85YD7xsg6dd3yeQZ80xJlV698znblPhtnVnAoSrreMOMokzIntJIEJoIIW9v8If/ZtesnA=",
                                                       common::xgroup_id_t{common::xvalidator_group_id_value_begin + 1}};
xaccount_data_bundle_t const account_pubkey_validator7{"T00000LdZssB2ayjJyVr94qprUjbw6LuRBreFzQn",
                                                       "BJpAFgA6EFM3GJTh7aqVjYAsv7KbdYzThNzwze3z0j8htze9CBqomOSSwyjQ/NZ/aTMRD31ccsHJG8E4eIMyI8Y=",
                                                       common::xgroup_id_t{common::xvalidator_group_id_value_begin + 2}};
xaccount_data_bundle_t const account_pubkey_validator8{"T00000LLTaHKwki5i6QsxzvLCvdjDsv2v3Kcxqu2",
                                                       "BGkwfXHAW/YuQRzneuSdfgd2nsH6xIglo12V9LMQXIQNDiFbJmtB8v4BWCKR9WoR9K6g+RhlmUbJiwvUkZgO0UA=",
                                                       common::xgroup_id_t{common::xvalidator_group_id_value_begin + 3}};

class xtop_vnetwork_fixture2 : public testing::Test {
protected:
    xobject_ptr_t<time::xchain_time_face_t> logic_timer_;
    std::shared_ptr<top::network::xnetwork_driver_face_t> network_driver_;
    std::unique_ptr<election::cache::xdata_accessor_face_t> data_accessor_;
    std::shared_ptr<top::vnetwork::xvhost_face_t> vhost_;

public:
    xtop_vnetwork_fixture2() = default;
    xtop_vnetwork_fixture2(xtop_vnetwork_fixture2 const &) = delete;
    xtop_vnetwork_fixture2(xtop_vnetwork_fixture2 &&) = default;
    xtop_vnetwork_fixture2 & operator=(xtop_vnetwork_fixture2 const &) = delete;
    xtop_vnetwork_fixture2 & operator=(xtop_vnetwork_fixture2 &&) = default;
    ~xtop_vnetwork_fixture2() override = default;

private:
    void add_rec(data::election::xelection_result_store_t & election_result_store,
                 common::xlogic_time_t const timestamp,
                 common::xlogic_time_t const start_time,
                 common::xelection_round_t const & group_version,
                 xaccount_data_bundle_t const & rec);

    void add_zec(data::election::xelection_result_store_t & election_result_store,
                 common::xlogic_time_t const timestamp,
                 common::xlogic_time_t const start_time,
                 common::xelection_round_t const & group_version,
                 xaccount_data_bundle_t const & zec);

    void add_auditor_validator(data::election::xelection_result_store_t & election_result_store,
                               common::xlogic_time_t const timestamp,
                               common::xlogic_time_t const start_time,
                               common::xelection_round_t const & group_version,
                               xaccount_data_bundle_t const & auditor,
                               std::vector<xaccount_data_bundle_t> const & validators);

protected:
    virtual xobject_ptr_t<time::xchain_time_face_t> create_logic_chain_timer() const = 0;
    virtual std::unique_ptr<election::cache::xdata_accessor_face_t> create_election_data_accessor() const = 0;
    virtual std::shared_ptr<top::network::xnetwork_driver_face_t> create_netwrok_driver() const = 0;
    virtual std::shared_ptr<top::vnetwork::xvhost_face_t> create_vhost() const;

public:
    void SetUp() override;

    void TearDown() override;

    observer_ptr<top::vnetwork::xvhost_face_t> vhost() const noexcept {
        return top::make_observer<top::vnetwork::xvhost_face_t>(vhost_.get());
    }

    observer_ptr<top::election::cache::xdata_accessor_face_t> data_accessor() const noexcept {
        return top::make_observer<top::election::cache::xdata_accessor_face_t>(data_accessor_.get());
    }
};
using xvnetwork_fixture2_t = xtop_vnetwork_fixture2;

NS_END3
