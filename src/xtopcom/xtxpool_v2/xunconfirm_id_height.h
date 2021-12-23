// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xtxpool_v2/xreceiptid_state_cache.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvtxindex.h"

#include <inttypes.h>

#include <deque>

NS_BEG2(top, xtxpool_v2)

#define resend_interval_min (60)

class xunconfirm_id_height_list_t {
public:
    xunconfirm_id_height_list_t(base::xtable_shortid_t self_table_sid, base::xtable_shortid_t peer_table_sid, bool as_sender)
      : m_self_table_sid(self_table_sid), m_peer_table_sid(peer_table_sid), m_as_sender(as_sender) {
    }
    void update_confirm_id(uint64_t confirm_id);
    void add_id_height(uint64_t receipt_id, uint64_t height, uint64_t time);
    bool get_min_height(uint64_t & min_height) const;
    bool get_height_by_id(uint64_t receipt_id, uint64_t & height) const;
    bool get_resend_id_height(uint64_t & receipt_id, uint64_t & height, uint64_t cur_time) const;
    uint32_t size() const;
    bool is_lacking() const;

private:
    uint64_t m_confirm_id{0};
    base::xtable_shortid_t m_self_table_sid;
    base::xtable_shortid_t m_peer_table_sid;
    bool m_as_sender;
    std::map<uint64_t, uint64_t> m_id_height_map;
    uint64_t m_update_time{0};
    uint64_t m_confirmed_height_max{0};
};

struct xresend_id_height_t {
    base::xtable_shortid_t table_sid;
    uint64_t receipt_id;
    uint64_t height;
};

class xtable_unconfirm_id_height_t {
public:
    xtable_unconfirm_id_height_t(base::xtable_shortid_t self_table_sid) : m_self_table_sid(self_table_sid) {
    }
    void update_confirm_id(base::xtable_shortid_t table_sid, uint64_t confirm_id);
    void add_id_height(base::xtable_shortid_t table_sid, uint64_t receipt_id, uint64_t height, uint64_t time);
    bool get_min_height(const xreceiptid_state_cache_t & receiptid_state_cache,
                        const std::set<base::xtable_shortid_t> & all_table_sids,
                        uint64_t self_height,
                        uint64_t & min_height,
                        bool check_lacking,
                        bool & is_lacking) const;
    bool get_height_by_id(base::xtable_shortid_t table_sid, uint64_t receipt_id, uint64_t & height) const;
    std::vector<xresend_id_height_t> get_resend_id_height_list(uint64_t cur_time) const;
    uint32_t size() const;

private:
    virtual void get_unconfirm_id_section(const xreceiptid_state_cache_t & receiptid_state_cache,
                                          base::xtable_shortid_t self_table_id,
                                          base::xtable_shortid_t peer_table_id,
                                          uint64_t & confirm_id,
                                          uint64_t & unconfirm_id_max) const = 0;
    virtual bool is_sender() const = 0;
    base::xtable_shortid_t m_self_table_sid;
    std::map<base::xtable_shortid_t, std::shared_ptr<xunconfirm_id_height_list_t>> m_table_sid_unconfirm_list_map;
};

class xtable_unconfirm_id_height_as_sender_t : public xtable_unconfirm_id_height_t {
public:
    xtable_unconfirm_id_height_as_sender_t(base::xtable_shortid_t self_table_sid) : xtable_unconfirm_id_height_t(self_table_sid) {
    }

private:
    void get_unconfirm_id_section(const xreceiptid_state_cache_t & receiptid_state_cache,
                                  base::xtable_shortid_t self_table_id,
                                  base::xtable_shortid_t peer_table_id,
                                  uint64_t & confirm_id,
                                  uint64_t & unconfirm_id_max) const override;
    bool is_sender() const override;
};

class xtable_unconfirm_id_height_as_receiver_t : public xtable_unconfirm_id_height_t {
public:
    xtable_unconfirm_id_height_as_receiver_t(base::xtable_shortid_t self_table_sid) : xtable_unconfirm_id_height_t(self_table_sid) {
    }

private:
    void get_unconfirm_id_section(const xreceiptid_state_cache_t & receiptid_state_cache,
                                  base::xtable_shortid_t self_table_id,
                                  base::xtable_shortid_t peer_table_id,
                                  uint64_t & confirm_id,
                                  uint64_t & unconfirm_id_max) const override;
    bool is_sender() const override;
};

class xprocessed_height_record_t {
public:
    void update_min_height(uint64_t height);
    void record_height(uint64_t height);
    bool is_record_height(uint64_t height) const;
    bool get_latest_lacking_saction(uint64_t & left_end, uint64_t & right_end, uint16_t max_lacking_num) const;
    uint32_t size() const;

private:
    void print() const;

    std::deque<uint64_t> m_bit_record;
    uint64_t m_min_height{0};         // include
    uint64_t m_max_height{0};         // not include
    uint64_t m_min_record_height{0};  // may not include
    uint64_t m_max_record_height{0};  // may not include
};

struct xtx_id_height_info {
    xtx_id_height_info(base::enum_transaction_subtype subtype, base::xtable_shortid_t peer_table_sid, uint64_t receipt_id)
      : m_subtype(subtype), m_peer_table_sid(peer_table_sid), m_receipt_id(receipt_id) {
    }
    base::enum_transaction_subtype m_subtype;
    base::xtable_shortid_t m_peer_table_sid;
    uint64_t m_receipt_id;
};

class xunconfirm_id_height {
public:
    xunconfirm_id_height(base::xtable_shortid_t self_table_sid)
      : m_self_table_sid(self_table_sid), m_sender_unconfirm_id_height(self_table_sid), m_receiver_unconfirm_id_height(self_table_sid) {
    }
    bool get_lacking_section(const xreceiptid_state_cache_t & receiptid_state_cache,
                             const std::set<base::xtable_shortid_t> & all_table_sids,
                             uint64_t & left_end,
                             uint64_t & right_end,
                             uint16_t max_lacking_num) const;
    void update_unconfirm_id_height(uint64_t table_height, uint64_t time, const std::vector<xtx_id_height_info> & tx_id_height_infos);
    void update_peer_confirm_id(base::xtable_shortid_t peer_table_sid, uint64_t confirm_id);
    bool get_sender_table_height_by_id(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t & height) const;
    bool get_receiver_table_height_by_id(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t & height) const;
    std::vector<xresend_id_height_t> get_sender_resend_id_height_list(uint64_t cur_time) const;
    std::vector<xresend_id_height_t> get_receiver_resend_id_height_list(uint64_t cur_time) const;
    void cache_status(uint32_t & sender_cache_size, uint32_t & receiver_cache_size, uint32_t & height_record_size) const;
    bool get_min_height(const xreceiptid_state_cache_t & receiptid_state_cache,
                        const std::set<base::xtable_shortid_t> & all_table_sids,
                        uint64_t & min_height,
                        bool check_lacking,
                        bool & need_sync) const;

private:
    base::xtable_shortid_t m_self_table_sid;
    xtable_unconfirm_id_height_as_sender_t m_sender_unconfirm_id_height;
    xtable_unconfirm_id_height_as_receiver_t m_receiver_unconfirm_id_height;
    mutable xprocessed_height_record_t m_processed_height_record;
    mutable std::mutex m_mutex;
};

NS_END2
