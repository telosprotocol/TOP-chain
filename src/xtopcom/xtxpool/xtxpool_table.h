// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction.h"
#include "xtxpool/xaccountobj.h"
#include "xtxpool/xtransaction_recv_cache.h"
#include "xtxpool/xtxpool_resources_face.h"
#include "xtxpool/xtxpool_lock_mgr.h"
#include "xtxpool/xunconfirm_sendtx_cache.h"
#include "xtxpool/xtxpool_receipt_receiver_counter.h"

#include <mutex>
#include <set>
#include <string>
#include <vector>

NS_BEG2(top, xtxpool)

class xtxpool_table_t final : public xtxpool_table_face_t, public tx_table {
    enum {
       enum_max_table_send_tx_num = 8,
       enum_max_table_unconfirm_tx_num = 8,
    };
 public:
    explicit xtxpool_table_t(uint8_t                                        zone_id,
                             uint16_t                                       table_id,
                             const std::shared_ptr<xtxpool_resources_face> &para);
    virtual ~xtxpool_table_t();

 public:
    void                        update_lock_blocks(const base::xblock_mptrs & latest_blocks) override;
    std::vector<std::string>    get_accounts() override;
    std::vector<xcons_transaction_ptr_t>                        get_account_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, uint64_t last_nonce, const uint256_t & last_hash, uint64_t now) override;
    void send_tx_increment(int64_t inc_num) override;
    void recv_tx_increment(int64_t inc_num) const override;
    void conf_tx_increment(int64_t inc_num) const override;
    const std::string & get_table_name() const override;
    void update_committed_table_block(xblock_t * block) override;
    void update_committed_unit_block(xblock_t * block, uint64_t account_tx_nonce, const uint256_t & account_tx_hash) override;
 public:
    int32_t                                                     push_send_tx(const xcons_transaction_ptr_t & tx);
    int32_t                                                     push_recv_tx(const xcons_transaction_ptr_t & tx);
    int32_t                                                     push_recv_ack_tx(const xcons_transaction_ptr_t & tx);
    std::map<std::string, std::vector<xcons_transaction_ptr_t>> get_txs(uint64_t commit_height, uint32_t max_account_num);
    std::vector<std::string>                                    get_accounts(uint32_t max_account_num);
    int32_t                                                     verify_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, std::vector<xcons_transaction_ptr_t> txs, uint64_t last_nonce, const uint256_t & last_hash);
    void                                                        on_block_confirmed(xblock_t * block);
    xaccountobj_t *                                             find_account(const std::string & address) const;
    void                                                        on_timer_check_cache(xreceipt_tranceiver_face_t & tranceiver, xtxpool_receipt_receiver_counter & counter);
    void                                                        init();
    void                                                        clear();
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const;
    xcons_transaction_ptr_t pop_tx_by_hash(const std::string & address, const uint256_t & hash, uint8_t subtype, int32_t result);  // for test
    xcons_transaction_ptr_t get_unconfirm_tx(const std::string source_addr, const uint256_t & hash);

private:
    xaccountobj_t * find_account_and_create_new(const std::string & address, uint64_t now);
    int32_t         txs_validity_check_leader(xaccountobj_t * account_obj,
                                              std::vector<xcons_transaction_ptr_t> & txs,
                                              uint64_t commit_height,
                                              uint64_t unit_height,
                                              uint64_t now);
    int32_t         txs_validity_check_backup(std::vector<xcons_transaction_ptr_t> & txs,
                                              uint64_t commit_height,
                                              uint64_t unit_height,
                                              uint64_t now,
                                              uint64_t last_nonce,
                                              const uint256_t & last_hash);
    int32_t         verify_send_tx(const xcons_transaction_ptr_t & tx, uint64_t now);
    int32_t         verify_receipt_tx(const xcons_transaction_ptr_t & tx);
    int32_t         verify_cons_tx(const xcons_transaction_ptr_t & tx, uint64_t now);
    bool            nonce_continous(uint64_t & last_nonce, uint256_t & last_hash, const xcons_transaction_ptr_t & tx);
    void            delete_committed_recv_txs(std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs);

 private:
    std::string                                      m_table_account;
    std::shared_ptr<xtxpool_resources_face>          m_para;
    xtransaction_recv_cache_t                        m_consensused_recvtx_cache;
    xunconfirm_sendtx_cache_t                        m_unconfirm_sendtx_cache;
    std::unordered_map<std::string, xaccountobj_t *> m_waiting_accounts;  // TODO(Nathan):order
    xtxpool_lock_mgr                                 m_accounts_lock_mgr;
    mutable std::mutex                               m_table_mutex;
    std::atomic<int32_t>                             m_send_tx_num{0};
};

NS_END2
