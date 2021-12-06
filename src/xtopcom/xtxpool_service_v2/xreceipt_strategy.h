// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xblock.h"
NS_BEG2(top, xtxpool_service_v2)

#define block_clock_height_fall_behind_max (30)

class xreceipt_strategy_t {
public:
    static bool is_time_for_refresh_table(uint64_t now);
    static bool is_selected_receipt_pull_msg_processor(uint64_t now, uint32_t table_id, uint16_t shard_size, uint16_t node_id);
    static bool is_selected_receipt_pull_msg_sender(const std::string & table_addr, uint64_t now, uint16_t node_id, uint16_t shard_size);
    static bool is_selected_pos(uint32_t pos, uint32_t rand_pos, uint32_t select_num, uint32_t size);
    static bool is_time_for_node_pull_lacking_receipts(uint64_t now, uint32_t table_id, uint16_t self_node_id);
    static bool is_receiptid_state_sender_for_talbe(uint64_t now, uint32_t table_id, uint16_t shard_size, uint16_t node_id);
};

NS_END2
