// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction.h"
#include "xtxpool_face.h"
#include "xtxpool_table.h"

#include <map>
#include <set>
#include <string>
#include <vector>

NS_BEG2(top, xtxpool)

class xtxpool_t final
  : public xtxpool_face_t
  , public base::xxtimer_t {
public:
    enum {
        enum_max_mailbox_num = 8192,
    };
    explicit xtxpool_t(const std::shared_ptr<xtxpool_resources_face> & para,
                       const xobject_ptr_t<base::xworkerpool_t> & pwork);

public:
    int32_t                                                     push_send_tx(const xtransaction_ptr_t & tx) override;
    int32_t                                                     push_send_tx(const xcons_transaction_ptr_t & tx) override;
    int32_t                                                     push_recv_tx(const xcons_transaction_ptr_t & tx) override;
    int32_t                                                     push_recv_ack_tx(const xcons_transaction_ptr_t & tx) override;
    std::map<std::string, std::vector<xcons_transaction_ptr_t>> get_txs(const std::string & tableblock_account, uint64_t commit_height) override;
    int32_t                 verify_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, std::vector<xcons_transaction_ptr_t> txs, uint64_t last_nonce, const uint256_t & last_hash) override;
    void                    on_block_confirmed(xblock_t * block) override;
    xtxpool_table_t *       get_txpool_table(const std::string & table_account) override;
    int32_t                 on_receipt(const data::xcons_transaction_ptr_t & trans) override;
    void                    set_receipt_tranceiver(std::shared_ptr<xreceipt_tranceiver_face_t> receipt_tranceiver) override;
    void                    subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override;
    void                    unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override;
    std::vector<std::string> get_accounts(const std::string & tableblock_account) override;
    std::vector<xcons_transaction_ptr_t> get_account_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, uint64_t last_nonce, const uint256_t & last_hash, uint64_t now) override;
    virtual bool            start_timer();
    virtual void            stop_timer();
    void                    dispatch(base::xcall_t & call) override;
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override;
    xaccountobj_t *         find_account(const std::string & address) override;                                                                             // for unit test
    xcons_transaction_ptr_t pop_tx_by_hash(const std::string & address, const uint256_t & hash, uint8_t subtype, int32_t result) override;  // for unit test
    bool                    is_mailbox_over_limit() override;
    xcons_transaction_ptr_t get_unconfirm_tx(const std::string source_addr, const uint256_t & hash);
protected:
    bool on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms, const int32_t start_timeout_ms, int32_t & in_out_cur_interval_ms) override;

private:
    void              on_block_to_db_event(mbus::xevent_ptr_t e);
    xtxpool_table_t * get_txpool_table(uint8_t zone, uint16_t table_id) const;
    xtxpool_table_t * get_txpool_table_by_addr(const std::string & address) const;
    void              make_receipts_and_send(xblock_t * block);

private:
    std::shared_ptr<xtxpool_resources_face>     m_para;
    xobject_ptr_t<base::xworkerpool_t>          m_work;
    mutable xtxpool_table_t *                   m_txpool_tables[enum_xtxpool_table_type_max][enum_vbucket_has_tables_count];
    uint8_t                                     m_table_cover_count[enum_xtxpool_table_type_max][enum_vbucket_has_tables_count];
    uint32_t                                    m_bus_listen_id;
    uint32_t                                    m_timer_fire_time{0};
    std::shared_ptr<xreceipt_tranceiver_face_t> m_receipt_tranceiver{nullptr};
    std::mutex                                  m_mutex[enum_xtxpool_table_type_max];
};

NS_END2
