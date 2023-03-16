// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service_v2/xreceipt_strategy.h"

#include "xdata/xtableblock.h"
NS_BEG2(top, xtxpool_service_v2)

#define refresh_table_interval (64)             // every 64 seconds refresh table.
#define receipt_resend_interval (64)            // every 64 seconds resend once
#define pull_missing_receipt_interval (64)      // every 64 seconds pull missing receipts once
#define receipt_sender_select_num (2)           // select 2 nodes to send receipt at the first time
#define receipt_pull_msg_sender_select_num (1)  // select 1 node to send recept pull msg
#define receipt_resender_select_num (1)         // select 1 nodes to resend receipt
#define receiptid_state_send_interval (64)     // every 128 seconds send receipt id state of a table

bool xreceipt_strategy_t::is_time_for_refresh_table(uint64_t now) {
    return (now % refresh_table_interval) == 0;
}

bool xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(uint64_t now, uint32_t table_id, uint16_t shard_size, uint16_t node_id) {
    // different table send at different time by different advance node
    uint64_t random_num = now + (uint64_t)table_id;
    bool is_time_for_resend = ((random_num % receiptid_state_send_interval) == 0);
    uint16_t resend_node_pos = ((now / receiptid_state_send_interval) + (uint64_t)table_id) % shard_size;
    xdbg("xreceipt_strategy_t::is_receiptid_state_sender_for_talbe table:%d,now:%llu,interval0x%x,is_time_for_resend:%d,shard_size:%d,resend_node_pos:%d,node_id:%d",
         table_id,
         now,
         receiptid_state_send_interval,
         is_time_for_resend,
         shard_size,
         resend_node_pos,
         node_id);
    return (is_time_for_resend && resend_node_pos == node_id);
}

bool xreceipt_strategy_t::is_time_for_node_pull_lacking_receipts(uint64_t now, uint32_t table_id, uint16_t self_node_id) {
    uint64_t random_num = now + (uint64_t)table_id + (uint64_t)self_node_id;
    return (random_num % pull_missing_receipt_interval) == 0;
}

bool xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(uint64_t now, uint32_t table_id, uint16_t shard_size, uint16_t node_id) {
    // select 2 auditor to send the receipt
    uint32_t select_num = receipt_pull_msg_sender_select_num;
    // calculate a random position that means which node is selected to send the receipt
    // the random position change by resend_time for rotate the selected node, to avoid same node is selected continuously.

    uint32_t rand_pos = (table_id + now) % shard_size;
    bool ret = is_selected_pos(node_id, rand_pos, select_num, shard_size);
    xdbg("xreceipt_strategy_t::is_selected_receipt_pull_msg_processor ret:%d table:%u rand_pos:%u select_num:%u node_id:%u shard_size:%u now:%llu",
         ret,
         table_id,
         rand_pos,
         select_num,
         node_id,
         shard_size,
         now);
    return ret;
}

bool xreceipt_strategy_t::is_selected_receipt_pull_msg_sender(const std::string & table_addr, uint64_t now, uint16_t node_id, uint16_t shard_size) {
    // select 2 auditor to send the receipt
    uint32_t select_num = receipt_pull_msg_sender_select_num;
    // calculate a random position that means which node is selected to send the receipt
    // the random position change by resend_time for rotate the selected node, to avoid same node is selected continuously.

    uint32_t time_pos = now / pull_missing_receipt_interval;
    uint32_t rand_pos = (base::xhash32_t::digest(table_addr) + time_pos) % shard_size;
    bool ret = is_selected_pos(node_id, rand_pos, select_num, shard_size);
    xinfo("xreceipt_strategy_t::is_selected_receipt_pull_msg_sender ret:%d table:%s rand_pos:%u select_num:%u node_id:%u shard_size:%u now:%llu time_pos:%u",
          ret,
          table_addr.c_str(),
          rand_pos,
          select_num,
          node_id,
          shard_size,
          now,
          time_pos);
    return ret;
}

#if 0
uint32_t xreceipt_strategy_t::calc_resend_time(uint64_t tx_cert_time, uint64_t now) {
    return (now - tx_cert_time) / receipt_resend_interval;
}
#endif

bool xreceipt_strategy_t::is_selected_pos(uint32_t pos, uint32_t rand_pos, uint32_t select_num, uint32_t size) {
    // bool ret = false;
    xassert((select_num < size) && (pos < size));
    if (pos >= rand_pos) {
        return pos < rand_pos + select_num;
    } else {
        return (rand_pos + select_num > size) && (pos < (rand_pos + select_num) % size);
    }
}

NS_END2
