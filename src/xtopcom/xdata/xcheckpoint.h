// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"
#include "xvledger/xvcheckpoint.h"

namespace top {
namespace data {

using xcheckpoints_t = std::map<uint64_t, base::xcheckpoint_data_t>;
using xcheckpoints_map_t = std::map<common::xaccount_address_t, xcheckpoints_t>;

class xtop_chain_checkpoint : public base::xvcpstore_t {
public:
    static xtop_chain_checkpoint & instance() {
        static xtop_chain_checkpoint _instance;
        return _instance;
    }

    /// @brief Load all cp data and state into memory.
    void load();

    /// @brief Get latest cp data of specific table.
    /// @param account Table account only.
    /// @param ec Error code.
    /// @return Cp data which include hash and clock.
    base::xcheckpoint_data_t get_latest_checkpoint(const std::string & address, std::error_code & ec) override;

    /// @brief Get all cp data of specific table.
    /// @param account Table account only.
    /// @param ec Error code.
    /// @return Cp data map with all heights: <clock, {block_height, block_hash...}>.
    xcheckpoints_t get_checkpoints(common::xaccount_address_t const & account, std::error_code & ec);

private:
    xcheckpoints_map_t m_checkpoints_map;
};
using xchain_checkpoint_t = xtop_chain_checkpoint;

}  // namespace data
}  // namespace top
