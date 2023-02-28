// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xserializable_based_on.h"
#include "xbasic/xutility.h"
#include "xcommon/xaddress.h"
#include "xdata/xblock.h"

#include <cstdint>
#include <sstream>
#include <vector>
#include <iomanip>

namespace top {
namespace data {


/// @brief Account related statistics data.
struct xtop_account_statistics_cons_data : public xserializable_based_on<void> {
    uint64_t burn_gas_value;

    std::string to_json_string() const;

    template <typename JsonObjectT>
    JsonObjectT to_json_object() const {
        JsonObjectT json_object;
        json_object["burn_gas"] =  std::to_string(burn_gas_value);
        return json_object;
    }

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xaccount_statistics_cons_data_t = xtop_account_statistics_cons_data;

int32_t operator>>(base::xstream_t & stream, xaccount_statistics_cons_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xaccount_statistics_cons_data_t const & data_object);

int32_t operator>>(base::xbuffer_t & buffer, xaccount_statistics_cons_data_t & data_object);
int32_t operator<<(base::xbuffer_t & buffer, xaccount_statistics_cons_data_t const & data_object);

/// @brief Statistics data hold by a group.
struct xtop_group_statistics_cons_data : public xserializable_based_on<void> {
    /// @brief Index is the account slot id.
    std::vector<xaccount_statistics_cons_data_t> account_statistics_data;

    std::string to_json_string() const;

    template <typename JsonObjectT>
    JsonObjectT to_json_object() const {
        JsonObjectT json_object;

        for (auto slot_id = 0u; slot_id < account_statistics_data.size(); ++slot_id) {
            json_object[std::to_string(slot_id)] = account_statistics_data[slot_id].to_json_object<JsonObjectT>();
        }

        return json_object;
    }

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xgroup_statistics_cons_data_t = xtop_group_statistics_cons_data;

int32_t operator>>(base::xstream_t & stream, xgroup_statistics_cons_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xgroup_statistics_cons_data_t const & data_object);

int32_t operator>>(base::xbuffer_t & buffer, xgroup_statistics_cons_data_t & data_object);
int32_t operator<<(base::xbuffer_t & buffer, xgroup_statistics_cons_data_t const & data_object);

/// @brief Statistics data hold by all groups in one election round.
struct xtop_election_statistics_cons_data : public xserializable_based_on<void> {
    /// @brief Statistics data grouped by the group id;
    std::map<common::xgroup_address_t, xgroup_statistics_cons_data_t> group_statistics_data;

    std::string to_json_string() const;

    template <typename JsonObjectT>
    JsonObjectT to_json_object() const {
        JsonObjectT json_object;

        for (auto const & group_data : group_statistics_data) {
            auto const & group_address = top::get<common::xgroup_address_t const>(group_data);
            auto const & group_statistics_datum = top::get<xgroup_statistics_cons_data_t>(group_data);

            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(8) << std::hex << group_address.xip().value();
            json_object[oss.str()] = group_statistics_datum.to_json_object<JsonObjectT>();
        }

        return json_object;
    }

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xelection_statistics_cons_data_t = xtop_election_statistics_cons_data;

int32_t operator>>(base::xstream_t & stream, xelection_statistics_cons_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xelection_statistics_cons_data_t const & data_object);

int32_t operator>>(base::xbuffer_t & buffer, xelection_statistics_cons_data_t & data_object);
int32_t operator<<(base::xbuffer_t & buffer, xelection_statistics_cons_data_t const & data_object);

/// @brief Statistics data grouped by election chain block height.
struct xtop_statistics_cons_data : public xserializable_based_on<void> {
    std::map<uint64_t, xelection_statistics_cons_data_t> detail;
    uint64_t   total_gas_burn{0};

    std::string to_json_string() const;

    template <typename JsonObjectT>
    JsonObjectT to_json_object() const {
        JsonObjectT json_object;

        for (auto const & table_data : detail) {
            auto const & election_blk_height = top::get<uint64_t const>(table_data);
            auto const & election_statistics_datum = top::get<xelection_statistics_cons_data_t>(table_data);

            json_object[std::to_string(election_blk_height)] = election_statistics_datum.to_json_object<JsonObjectT>();
        }

        return json_object;
    }

private:
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write(base::xstream_t & stream) const override;
};
using xstatistics_cons_data_t = xtop_statistics_cons_data;

int32_t operator>>(base::xstream_t & stream, xstatistics_cons_data_t & data_object);
int32_t operator<<(base::xstream_t & stream, xstatistics_cons_data_t const & data_object);

}  // namespace data
}  // namespace top
