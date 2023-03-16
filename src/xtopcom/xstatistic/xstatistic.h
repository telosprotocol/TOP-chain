// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"

#include <atomic>
#include <string>

NS_BEG2(top, xstatistic)

enum enum_statistic_class_type {
    enum_statistic_undetermined = -1,
    enum_statistic_begin = 0,
    enum_statistic_tx_v2 = enum_statistic_begin,
    enum_statistic_tx_v3,
    enum_statistic_receipt,
    enum_statistic_vqcert,
    enum_statistic_vblock,
    enum_statistic_table_bstate,
    enum_statistic_unit_bstate,
    enum_statistic_block_header,
    enum_statistic_vinput,
    enum_statistic_voutput,
    enum_statistic_vbstate,
    enum_statistic_vcanvas,
    enum_statistic_mpt_state_object,
    enum_statistic_bindex,
    enum_statistic_account_index,
    enum_statistic_receiptid_pair,
#ifndef CACHE_SIZE_STATISTIC_MORE_DETAIL
    enum_statistic_event,
    enum_statistic_event_account = enum_statistic_event,
    enum_statistic_event_behind = enum_statistic_event,
    enum_statistic_event_block = enum_statistic_event,
    enum_statistic_event_blockfetcher = enum_statistic_event,
    enum_statistic_event_consensus = enum_statistic_event,
    enum_statistic_event_sync_executor = enum_statistic_event,
    enum_statistic_event_network = enum_statistic_event,
    enum_statistic_event_role = enum_statistic_event,
    enum_statistic_event_state_sync = enum_statistic_event,
    enum_statistic_event_store = enum_statistic_event,
    enum_statistic_event_sync = enum_statistic_event,
    enum_statistic_event_timer = enum_statistic_event,
    enum_statistic_event_chain_timer = enum_statistic_event,
    enum_statistic_event_vnode = enum_statistic_event,
    enum_statistic_msg_cons,
    enum_statistic_msg_txpool,
    enum_statistic_msg_rpc,
    enum_statistic_msg_sync,
    enum_statistic_msg_block_broadcast,
    enum_statistic_msg_state,
#else
    enum_statistic_event_account,
    enum_statistic_event_behind,
    enum_statistic_event_block,
    enum_statistic_event_blockfetcher,
    enum_statistic_event_consensus,
    enum_statistic_event_sync_executor,
    enum_statistic_event_network,
    enum_statistic_event_role,
    enum_statistic_event_state_sync,
    enum_statistic_event_store,
    enum_statistic_event_sync,
    enum_statistic_event_timer,
    enum_statistic_event_chain_timer,
    enum_statistic_event_vnode,
    enum_statistic_msg_rpc_request,
    enum_statistic_msg_rpc_response,
    enum_statistic_msg_rpc_query_request,
    enum_statistic_msg_rpc_eth_request,
    enum_statistic_msg_rpc_eth_response,
    enum_statistic_msg_rpc_eth_query_request,
    enum_statistic_msg_state_trie_request,
    enum_statistic_msg_state_trie_response,
    enum_statistic_msg_state_table_request,
    enum_statistic_msg_state_table_response,
    enum_statistic_msg_state_unit_request,
    enum_statistic_msg_state_unit_response,
    enum_statistic_msg_txpool_send_receipt,
    enum_statistic_msg_txpool_recv_receipt,
    enum_statistic_msg_txpool_pull_recv_receipt,
    enum_statistic_msg_txpool_push_receipt,
    enum_statistic_msg_txpool_pull_confirm_receipt,
    enum_statistic_msg_txpool_receipt_id_state,
    enum_statistic_msg_bft,
    enum_statistic_msg_timer,
    enum_statistic_msg_relay_bft,
    enum_statistic_msg_block_broadcast,
    enum_statistic_msg_sync_gossip,
    enum_statistic_msg_sync_frozen_gossip,
    enum_statistic_msg_sync_broadcast_chain_state,
    enum_statistic_msg_sync_frozen_broadcast_chain_state,
    enum_statistic_msg_sync_response_chain_state,
    enum_statistic_msg_sync_frozen_response_chain_state,
    enum_statistic_msg_sync_cross_cluster_chain_state,
    enum_statistic_msg_sync_chain_snapshot_request,
    enum_statistic_msg_sync_chain_snapshot_response,
    enum_statistic_msg_sync_ondemand_chain_snapshot_request,
    enum_statistic_msg_sync_ondemand_chain_snapshot_response,
    enum_statistic_msg_sync_query_archive_height,
    enum_statistic_msg_sync_newblock_push,
    enum_statistic_msg_sync_block_request,
    enum_statistic_msg_sync_block_response,
#endif
    enum_statistic_max,
};

#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
class xstatistic_obj_face_t {
public:
    xstatistic_obj_face_t(enum_statistic_class_type class_type);
    xstatistic_obj_face_t(const xstatistic_obj_face_t & obj);
    xstatistic_obj_face_t & operator = (const xstatistic_obj_face_t & obj);
    void modify_class_type(enum_statistic_class_type class_type);
    void statistic_del();   // derived class must call statistic_del() in destructor, or else will core!!!

    int32_t create_time() const {return m_create_time;}
    const int32_t get_object_size() const;

    virtual int32_t get_class_type() const = 0;

private:
    virtual size_t get_object_size_real() const = 0;

private:
    // Attention: time unit is millisecond, expression range of int32 is [-2147483648, 2147483647], max time is about 596 hours.
    // It's enough for testing, but not for main net.
    int32_t m_create_time{0};
    mutable int32_t m_size{0};
};

class xstatistic_obj_comp {
public:
    bool operator()(const xstatistic_obj_face_t * left, const xstatistic_obj_face_t * right) const {
        return left->create_time() < right->create_time();
    }
};

using xnot_calc_object_set_t = std::multiset<xstatistic_obj_face_t *, xstatistic_obj_comp>;
using xnot_calc_object_map_t = std::map<xstatistic_obj_face_t *, xnot_calc_object_set_t::iterator>;

class xobject_statistic_base_t {
public:
    xobject_statistic_base_t() {}
    void add_object(xstatistic_obj_face_t * object, int32_t class_type);
    void del_object(xstatistic_obj_face_t * object, int32_t class_type);
    void refresh(int32_t class_type);
private:
    void refresh_inner(int32_t class_type, int64_t now);
    void update_metrics(int32_t type, int32_t change_num, int32_t change_size);
private:
    // use multiset only won the best performance.  for test case test_statistic.basic, multiset cost 57ms, multiset+map cost 65ms, set cost 1272ms.
    xnot_calc_object_set_t m_not_calc_object_set;
    mutable std::mutex m_mutex;
};

class xstatistic_t {
public:
    static xstatistic_t & instance();
    void add_object(xstatistic_obj_face_t * object, int32_t class_type);
    void del_object(xstatistic_obj_face_t * object, int32_t class_type);
    void refresh();
private:
    xobject_statistic_base_t m_object_statistic_arr[enum_statistic_max - enum_statistic_begin];
};
#else
class xstatistic_obj_face_t {
public:
    xstatistic_obj_face_t(enum_statistic_class_type class_type) {}
    xstatistic_obj_face_t(const xstatistic_obj_face_t & obj) {}
    void modify_class_type(enum_statistic_class_type class_type) {}
    void statistic_del() {}
    virtual int32_t get_class_type() const = 0;
private:
    virtual size_t get_object_size_real() const = 0;
};
#endif

NS_END2
