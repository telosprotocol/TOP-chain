// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xelection/xcache/xdata_accessor_face.h"

#include <gtest/gtest.h>

#include <memory>

NS_BEG3(top, tests, election)

common::xaccount_address_t build_account_address(size_t i);

class xtop_election_cache_data_accessor_fixture : public testing::Test {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_election_cache_data_accessor_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_election_cache_data_accessor_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_election_cache_data_accessor_fixture);

protected:
    std::unique_ptr<top::election::cache::xdata_accessor_face_t> m_election_cache_data_accessor{};
};
using xelection_cache_data_accessor_fixture_t = xtop_election_cache_data_accessor_fixture;

class xtop_committee_fixure : public xelection_cache_data_accessor_fixture_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_committee_fixure);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_committee_fixure);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_committee_fixure);

protected:
    void
    SetUp() override;

    void
    TearDown() override;
};
using xcommittee_fixture_t = xtop_committee_fixure;

class xtop_zec_fixture : public xelection_cache_data_accessor_fixture_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_zec_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_zec_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_zec_fixture);

protected:
    void
    SetUp() override;

    void
    TearDown() override;
};
using xzec_fixture_t = xtop_zec_fixture;

class xtop_edge_fixture : public xelection_cache_data_accessor_fixture_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_edge_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_edge_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_edge_fixture);

protected:
    void
    SetUp() override;

    void
    TearDown() override;
};
using xedge_fixture_t = xtop_edge_fixture;

class xtop_archive_fixture : public xelection_cache_data_accessor_fixture_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_archive_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_archive_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_archive_fixture);

protected:
    void
    SetUp() override;

    void
    TearDown() override;
};
using xarchive_fixture_t = xtop_archive_fixture;

class xtop_consensus_fixture : public xelection_cache_data_accessor_fixture_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_consensus_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_consensus_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_consensus_fixture);

protected:
    void
    SetUp() override;

    void
    TearDown() override;
};
using xconsensus_fixture_t = xtop_consensus_fixture;

NS_END3
