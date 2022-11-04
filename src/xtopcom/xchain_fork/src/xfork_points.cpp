// Copyright (c) 2022 - present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchain_fork/xfork_points.h"

NS_BEG2(top, fork_points)

#if defined(XCHAIN_FORKED_BY_DEFAULT)
#    if (!defined(XBUILD_CI) && !defined(XBUILD_DEV) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO))
#        error "XCHAIN_FORKED_BY_DEFAULT cannot be used in mainnet"
#    endif

#    if defined(XBUILD_CI)
constexpr uint32_t BLOCK_FORK_POINT = 6000000;
#    else
constexpr uint32_t BLOCK_FORK_POINT = 0;
#    endif
optional<xfork_point_t> block_fork_point            = xfork_point_t{xfork_point_type_t::logic_time, 0, "block fork point"};
optional<xfork_point_t> V3_0_0_0_block_fork_point   = xfork_point_t{xfork_point_type_t::logic_time, BLOCK_FORK_POINT, "v3 block fork point"};
optional<xfork_point_t> tx_v2_fee_fork_point        = xfork_point_t{xfork_point_type_t::logic_time, 0, "tx v2 fee fork point"};
optional<xfork_point_t> partly_remove_confirm       = xfork_point_t{xfork_point_type_t::logic_time, 0, "partly remove confirm"};
optional<xfork_point_t> add_rsp_id                  = xfork_point_t{xfork_point_type_t::logic_time, 0, "add rsp id"};
optional<xfork_point_t> inner_table_tx              = xfork_point_t{xfork_point_type_t::logic_time, 0, "inner table tx"};
optional<xfork_point_t> eth_fork_point              = xfork_point_t{xfork_point_type_t::logic_time, 0, "enable eth shard"};
optional<xfork_point_t> relay_fork_point            = xfork_point_t{xfork_point_type_t::logic_time, 0, "enable relay"};
optional<xfork_point_t> v1_6_0_version_point        = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.6 version control"};  // for v1.6.0 version control
optional<xfork_point_t> v1_7_0_block_fork_point     = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.7 version control"};
optional<xfork_point_t> v1_7_0_sync_point           = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.7 sync protocol fork"};
optional<xfork_point_t> TEST_FORK                   = xfork_point_t{xfork_point_type_t::logic_time, 9428100, "test fork"};
#else
optional<xfork_point_t> block_fork_point            = xfork_point_t{xfork_point_type_t::logic_time, 6859080, "block fork point"};
optional<xfork_point_t> V3_0_0_0_block_fork_point   = xfork_point_t{xfork_point_type_t::logic_time, 7126740, "v3 block fork point"};
optional<xfork_point_t> tx_v2_fee_fork_point        = xfork_point_t{xfork_point_type_t::logic_time, 7221960, "tx v2 fee fork point"};
optional<xfork_point_t> partly_remove_confirm       = xfork_point_t{xfork_point_type_t::logic_time, 7473960, "partly remove confirm"};
optional<xfork_point_t> add_rsp_id                  = xfork_point_t{xfork_point_type_t::logic_time, 7714440, "add rsp id"};
optional<xfork_point_t> inner_table_tx              = xfork_point_t{xfork_point_type_t::logic_time, 7716060, "inner table tx"};  // should later than "add rsp id"
optional<xfork_point_t> eth_fork_point              = xfork_point_t{xfork_point_type_t::logic_time, 8224200, "enable eth shard"};  // 2022-06-17 10:00:00
optional<xfork_point_t> relay_fork_point            = xfork_point_t{xfork_point_type_t::logic_time, 8820360, "enable relay"};
optional<xfork_point_t> v1_6_0_version_point        = xfork_point_t{xfork_point_type_t::logic_time, 8820360, "v1.6 version control"};
optional<xfork_point_t> v1_7_0_block_fork_point     = xfork_point_t{xfork_point_type_t::logic_time, 9459720, "v1.7 version control"};
optional<xfork_point_t> v1_7_0_sync_point           = xfork_point_t{xfork_point_type_t::logic_time, 9460080, "v1.7 sync protocol fork"};
optional<xfork_point_t> TEST_FORK                   = xfork_point_t{xfork_point_type_t::logic_time, 9428100, "test fork"};
#endif

NS_END2
