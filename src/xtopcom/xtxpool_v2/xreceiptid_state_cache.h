// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtxpool_v2/xtxpool_face.h"
#include "xvledger/xreceiptid.h"
#include "xvledger/xvpropertyprove.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

#define xtxpool_zone_type_max (6)

class xtable_receiptid_info_t {
public:
    void update_table_receiptid_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const base::xreceiptid_state_ptr_t & receiptid_state);
    const base::xvproperty_prove_ptr_t & get_property_prove() const;
    const base::xreceiptid_state_ptr_t & get_receiptid_state() const;
    
private:
    base::xvproperty_prove_ptr_t m_property_prove_ptr{nullptr};
    base::xreceiptid_state_ptr_t m_receiptid_state{nullptr};
};

class xunconfirm_tx_nums_t {
public:
    xunconfirm_tx_nums_t();
    void add_unconfirm_tx_num(uint32_t zoneid, uint32_t subaddr, int32_t num);
    int32_t get_unconfirm_tx_num(uint32_t zoneid, uint32_t subaddr) const;
private:
    std::vector<int32_t> m_unconfirm_nums[xtxpool_zone_type_max];
};

class xreceiptid_state_cache_t {
public:
    xreceiptid_state_cache_t();
    void update_table_receiptid_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const base::xreceiptid_state_ptr_t & receiptid_state);
    uint64_t get_confirmid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_recvid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_sendid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_height(base::xtable_shortid_t table_id) const;
    base::xreceiptid_state_ptr_t get_table_receiptid_state(base::xtable_shortid_t table_id) const;
    // bool is_all_table_state_cached(const std::set<base::xtable_shortid_t> & all_table_sids) const;
    void get_unconfirm_id_section_as_sender(base::xtable_shortid_t table_id,
                                            base::xtable_shortid_t peer_table_id,
                                            uint64_t & confirm_id,
                                            uint64_t & unconfirm_id_max,
                                            bool for_pull_lacking) const;
    void get_unconfirm_id_section_as_receiver(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id, uint64_t & confirm_id, uint64_t & unconfirm_id_max) const;
    const xreceiptid_state_and_prove get_receiptid_state_and_prove(base::xtable_shortid_t self_table_id,
                                                                   base::xtable_shortid_t peer_table_id,
                                                                   uint64_t min_not_need_confirm_receiptid,
                                                                   uint64_t max_not_need_confirm_receiptid) const;
    xunconfirm_tx_nums_t get_unconfirm_tx_nums() const;

private:
    mutable std::mutex m_mutex;
    std::vector<xtable_receiptid_info_t> m_receiptid_infos[xtxpool_zone_type_max];
    xunconfirm_tx_nums_t m_unconfirm_tx_nums;
    mutable bool m_all_cached{false};
};

NS_END2
