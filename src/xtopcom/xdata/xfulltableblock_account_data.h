#pragma once
// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xserializable_based_on.h"
#include "xcommon/xaddress.h"

#include <vector>

NS_BEG2(top, data)

struct xtop_fulltableblock_account_data : public xserializable_based_on<void> {
    /// @brief Index is the account slot id.
    std::vector<common::xaccount_address_t> account_data;

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xfulltableblock_account_data_t = xtop_fulltableblock_account_data;

int32_t operator>>(base::xstream_t& stream, xfulltableblock_account_data_t& data_object);
int32_t operator<<(base::xstream_t& stream, xfulltableblock_account_data_t const& data_object);
int32_t operator>>(base::xbuffer_t & buffer, xfulltableblock_account_data_t& data_object);
int32_t operator<<(base::xbuffer_t & buffer, xfulltableblock_account_data_t const& data_object);

struct xtop_fulltableblock_group_data : public xserializable_based_on<void> {
    std::map<common::xgroup_address_t, xfulltableblock_account_data_t> group_data;

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xfulltableblock_group_data_t = xtop_fulltableblock_group_data;

int32_t operator>>(base::xstream_t& stream, xfulltableblock_group_data_t& data_object);
int32_t operator<<(base::xstream_t& stream, xfulltableblock_group_data_t const& data_object);
int32_t operator>>(base::xbuffer_t & buffer, xfulltableblock_group_data_t& data_object);
int32_t operator<<(base::xbuffer_t & buffer, xfulltableblock_group_data_t const& data_object);

struct xtop_fulltableblock_statistic_accounts : public xserializable_based_on<void> {
    std::map<uint64_t, xfulltableblock_group_data_t> accounts_detail;

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xfulltableblock_statistic_accounts = xtop_fulltableblock_statistic_accounts;

int32_t operator>>(base::xstream_t& stream, xfulltableblock_statistic_accounts& data_object);
int32_t operator<<(base::xstream_t& stream, xfulltableblock_statistic_accounts const& data_object);
int32_t operator>>(base::xbuffer_t & buffer, xfulltableblock_statistic_accounts& data_object);
int32_t operator<<(base::xbuffer_t & buffer, xfulltableblock_statistic_accounts const& data_object);

NS_END2