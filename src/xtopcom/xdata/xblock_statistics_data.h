// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xserializable_based_on.h"
#include "xbasic/xobject_ptr.h"
#include "xcommon/xip.h"
#include "xdata/xblock.h"

#include <cstdint>
#include <set>
#include <vector>

namespace top {
namespace data {

/// @brief Stores data read from tableblock object directly
struct xtop_block_data : public xserializable_based_on<void> {
    uint32_t transaction_count{0};
    uint32_t block_count{0};

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xblock_data_t = xtop_block_data;

int32_t operator>>(base::xstream_t & stream, xblock_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xblock_data_t const & data_object);

/// @brief Stores consensus execution related data.
struct xtop_consensus_vote_data : public xserializable_based_on<void> {
    uint32_t vote_count{0};

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xconsensus_vote_data_t = xtop_consensus_vote_data;

int32_t operator>>(base::xstream_t & stream, xconsensus_vote_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xconsensus_vote_data_t const & data_object);

/// @brief Account related statistics data.
struct xtop_account_related_statistics_data : public xserializable_based_on<void> {
    xblock_data_t block_data;
    xconsensus_vote_data_t vote_data;

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xaccount_related_statistics_data_t = xtop_account_related_statistics_data;

int32_t operator>>(base::xstream_t & stream, xaccount_related_statistics_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xaccount_related_statistics_data_t const & data_object);

int32_t operator>>(base::xbuffer_t & buffer, xaccount_related_statistics_data_t & data_object);
int32_t operator<<(base::xbuffer_t & buffer, xaccount_related_statistics_data_t const & data_object);

/// @brief Statistics data hold by a group.
struct xtop_group_releated_statistics_data : public xserializable_based_on<void> {
    /// @brief Index is the account slot id.
    std::vector<xaccount_related_statistics_data_t> account_statistics_data;

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xgroup_related_statistics_data_t = xtop_group_releated_statistics_data;

int32_t operator>>(base::xstream_t & stream, xgroup_related_statistics_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xgroup_related_statistics_data_t const & data_object);

int32_t operator>>(base::xbuffer_t & buffer, xgroup_related_statistics_data_t & data_object);
int32_t operator<<(base::xbuffer_t & buffer, xgroup_related_statistics_data_t const & data_object);

/// @brief Statistics data hold by all groups in one election round.
struct xtop_election_related_statistics_data : public xserializable_based_on<void> {
    /// @brief Statistics data grouped by the group id;
    std::map<common::xgroup_id_t, xgroup_related_statistics_data_t> group_statistics_data;

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xelection_related_statistics_data_t = xtop_election_related_statistics_data;

int32_t operator>>(base::xstream_t & stream, xelection_related_statistics_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xelection_related_statistics_data_t const & data_object);

int32_t operator>>(base::xbuffer_t & buffer, xelection_related_statistics_data_t & data_object);
int32_t operator<<(base::xbuffer_t & buffer, xelection_related_statistics_data_t const & data_object);

/// @brief Statistics data grouped by election chain block height.
struct xtop_statistics_data : public xserializable_based_on<void> {
    std::map<uint64_t, xelection_related_statistics_data_t> detail;

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xstatistics_data_t = xtop_statistics_data;

int32_t operator>>(base::xstream_t & stream, xstatistics_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xstatistics_data_t const & data_object);

}  // namespace data
}  // namespace top
