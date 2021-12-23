// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"
#include "xvledger/xvstate.h"

namespace top {
namespace data {

struct xtop_checkpoint_data {
    uint64_t height;
    std::string hash;
};
using xcheckpoint_data_t = xtop_checkpoint_data;

using xcheckpoints_t = std::map<uint64_t, xcheckpoint_data_t>;
// <table, <clock, {block_height, block_hash...}>>
using xcheckpoints_map_t = std::map<common::xaccount_address_t, xcheckpoints_t>;
// <account, state_str>
using xcheckpoints_state_map_t = std::map<common::xaccount_address_t, std::string>;

class xtop_chain_checkpoint {
public:
    /// @brief Get cp data of specific table and height.
    /// @param account Table account only.
    /// @param cp_clock Clock of checkpoint.
    /// @param ec Error code.
    /// @return Cp data which include hash and clock.
    static xcheckpoint_data_t get_checkpoint(common::xaccount_address_t const & account, const uint64_t cp_clock, std::error_code & ec);

    /// @brief Get latest cp data of specific table.
    /// @param account Table account only.
    /// @param ec Error code.
    /// @return Cp data which include hash and clock.
    static xcheckpoint_data_t get_latest_checkpoint(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Get all cp data of specific table.
    /// @param account Table account only.
    /// @param ec Error code.
    /// @return Cp data map with all heights: <clock, {block_height, block_hash...}>.
    static xcheckpoints_t get_checkpoints(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Get latest cp state of specific table.
    /// @param account Account can be either table or unit.
    /// @param ec Error code.
    /// @return Cp data which include hash and clock.
    static xobject_ptr_t<base::xvbstate_t> get_latest_checkpoint_state(common::xaccount_address_t const & account, std::error_code & ec);

private:
    static xcheckpoints_map_t m_checkpoints_map;
    static xcheckpoints_state_map_t m_checkpoints_state_map;
};
using xchain_checkpoint_t = xtop_chain_checkpoint;

}  // namespace data
}  // namespace top
