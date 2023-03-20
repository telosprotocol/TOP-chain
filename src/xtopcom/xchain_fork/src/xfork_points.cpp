// Copyright (c) 2022 - present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchain_fork/xfork_points.h"

NS_BEG2(top, fork_points)

#if defined(XCHAIN_FORKED_BY_DEFAULT)
#    if (!defined(XBUILD_CI) && !defined(XBUILD_DEV) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO))
#        error "XCHAIN_FORKED_BY_DEFAULT cannot be used in mainnet"
#    endif

optional<xfork_point_t> v1_7_0_block_fork_point     = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.7 version control"};
optional<xfork_point_t> v1_7_0_sync_point           = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.7 sync protocol fork"};
optional<xfork_point_t> v1_9_0_state_fork_point     = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.9 unitstate fork"};
optional<xfork_point_t> xbft_msg_upgrade            = xfork_point_t{xfork_point_type_t::logic_time, 0, "xbft msg upgrade"};
optional<xfork_point_t> v10900_upgrade_table_tickets_contract = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.9 vote contract update"};
optional<xfork_point_t> v10901_table_tickets_reset = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.9 vote contract data reset"};
optional<xfork_point_t> v10901_enable_voting = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.9 vote contract re-enabling"};
optional<xfork_point_t> v10902_table_tickets_reset = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.9.2 vote contract data reset"};
optional<xfork_point_t> v10902_enable_voting = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.9.2 vote contract re-enabling"};
optional<xfork_point_t> v11100_event = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.11.0 add event"};
optional<xfork_point_t> v11200_block_fork_point    = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.12 block optimize"};
optional<xfork_point_t> v11200_sync_big_packet = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.12 sync big packet optimize"};
optional<xfork_point_t> v11200_fullnode_elect = xfork_point_t{xfork_point_type_t::logic_time, 0, "v1.12 fullnode elect"};
#else

optional<xfork_point_t> v1_7_0_block_fork_point           = xfork_point_t{xfork_point_type_t::logic_time, 9459720, "v1.7 version control"};
optional<xfork_point_t> v1_7_0_sync_point                 = xfork_point_t{xfork_point_type_t::logic_time, 9460080, "v1.7 sync protocol fork"};
optional<xfork_point_t> v1_9_0_state_fork_point           = xfork_point_t{xfork_point_type_t::logic_time, 10038600, "v1.9 unitstate fork"};
optional<xfork_point_t> xbft_msg_upgrade                  = xfork_point_t{xfork_point_type_t::logic_time, 10038600, "xbft msg upgrade"};

optional<xfork_point_t> v10900_upgrade_table_tickets_contract = xfork_point_t{xfork_point_type_t::logic_time, 10038600, "v1.9 vote contract update"};
optional<xfork_point_t> v10901_table_tickets_reset = xfork_point_t{xfork_point_type_t::logic_time, 10087200, "v1.9 vote contract data reset"};
optional<xfork_point_t> v10901_enable_voting = xfork_point_t{xfork_point_type_t::logic_time, 10091880, "v1.9 vote contract re-enabling"};
optional<xfork_point_t> v10902_table_tickets_reset = xfork_point_t{xfork_point_type_t::logic_time, 10247400, "v1.9.2 vote contract data reset"};
optional<xfork_point_t> v10902_enable_voting = xfork_point_t{xfork_point_type_t::logic_time, 10249200, "v1.9.2 vote contract re-enabling"};
optional<xfork_point_t> v11100_event = xfork_point_t{xfork_point_type_t::logic_time, 10582920, "v1.11.0 add event"};//2023-03-17 10:00:00
optional<xfork_point_t> v11200_block_fork_point    = xfork_point_t{xfork_point_type_t::logic_time, 16898760, "v1.12 block optimize"};//2025-03-17 10:00:00 XTODO need change future
optional<xfork_point_t> v11200_sync_big_packet = xfork_point_t{xfork_point_type_t::logic_time, 16898760, "v1.12 sync big packet optimize"};//2025-03-17 10:00:00 XTODO need change future
optional<xfork_point_t> v11200_fullnode_elect = xfork_point_t{xfork_point_type_t::logic_time, 16898760, "v1.12 fullnode elect"};
#endif

std::string dump_fork_points() {
    std::string info;
    info += "v11100_event=" + std::to_string(v11100_event.value().point);
    info += ",v11200_block_fork_point=" + std::to_string(v11200_block_fork_point.value().point);
    info += ",v11200_sync_big_packet=" + std::to_string(v11200_sync_big_packet.value().point);
    return info;
}

NS_END2
