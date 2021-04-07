// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction.h"
#include "xstore/xstore_face.h"
#include "xtxpool/xaccountobj.h"
#include "xtxpool/xtxpool_resources_face.h"

#include <string>

NS_BEG2(top, xtxpool)

using store::xstore_face_ptr_t;

enum enum_xtxpool_table_type { enum_xtxpool_table_type_max = 3 };

#define send_tx_receipt_first_retry_timeout (200)
#define send_tx_receipt_common_retry_timeout (800)

struct xaccount_txs_info_t {
    std::string                          m_address;
    uint64_t                             m_unit_height;
    std::vector<xcons_transaction_ptr_t> m_txs;
};

class xreceipt_tranceiver_face_t {
public:
    virtual void send_receipt(const xcons_transaction_ptr_t & receipt, uint32_t resend_time) = 0;
};

class xtxpool_table_face_t {
public:
    // virtual std::map<std::string, std::vector<xcons_transaction_ptr_t>> get_txs(const std::string & tableblock_account, const uint64_t & commit_height) = 0;
    virtual void    update_lock_blocks(const base::xblock_mptrs & latest_blocks) = 0;
    virtual std::vector<std::string> get_accounts() = 0;
    virtual std::vector<xcons_transaction_ptr_t> get_account_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, uint64_t last_nonce, const uint256_t & last_hash, uint64_t now) = 0;
    virtual void update_committed_table_block(xblock_t * block) = 0;
    virtual void update_committed_unit_block(xblock_t * block, uint64_t account_tx_nonce, const uint256_t & account_tx_hash) = 0;
};

// xtxpool_face_t interface of txpool
class xtxpool_face_t : public base::xobject_t {
public:
    virtual int32_t                                                     push_send_tx(const xtransaction_ptr_t & tx) = 0;
    virtual int32_t                                                     push_send_tx(const xcons_transaction_ptr_t & tx) = 0;
    virtual int32_t                                                     push_recv_tx(const xcons_transaction_ptr_t & tx) = 0;
    virtual int32_t                                                     push_recv_ack_tx(const xcons_transaction_ptr_t & tx) = 0;
    virtual std::map<std::string, std::vector<xcons_transaction_ptr_t>> get_txs(const std::string & tableblock_account, uint64_t commit_height) = 0;
    virtual int32_t                 verify_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, std::vector<xcons_transaction_ptr_t> txs, uint64_t last_nonce, const uint256_t & last_hash) = 0;
    virtual void                    on_block_confirmed(xblock_t * block) = 0;
    virtual xaccountobj_t *         find_account(const std::string & address) = 0;                                                                             // for unit test
    virtual xcons_transaction_ptr_t pop_tx_by_hash(const std::string & address, const uint256_t & hash, uint8_t subtype, int32_t result) = 0;  // for unit test
    virtual xtxpool_table_face_t *  get_txpool_table(const std::string & table_account) = 0;
    virtual int32_t                 on_receipt(const data::xcons_transaction_ptr_t & cons_tx) = 0;
    virtual void                    set_receipt_tranceiver(std::shared_ptr<xreceipt_tranceiver_face_t> receipt_tranceiver) = 0;
    virtual void                    subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) = 0;
    virtual void                    unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) = 0;
    virtual std::vector<std::string> get_accounts(const std::string & tableblock_account) = 0;
    virtual std::vector<xcons_transaction_ptr_t> get_account_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, uint64_t last_nonce, const uint256_t & last_hash, uint64_t now) = 0;
    virtual void                    dispatch(base::xcall_t & call) = 0;
    virtual xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const = 0;
    virtual bool                    is_mailbox_over_limit() = 0;
    virtual xcons_transaction_ptr_t get_unconfirm_tx(const std::string source_addr, const uint256_t & hash) = 0;
};

class xtxpool_instance {
public:
    static xobject_ptr_t<xtxpool_face_t> create_xtxpool_inst(const observer_ptr<store::xstore_face_t> &     store,
                                                             xobject_ptr_t<base::xvblockstore_t> &          blockstore,
                                                             const observer_ptr<mbus::xmessage_bus_face_t> &bus,
                                                             const xobject_ptr_t<base::xvcertauth_t> &      certauth,
                                                             const observer_ptr<time::xchain_time_face_t> & clock = nullptr,
                                                             enum_xtxpool_order_strategy                    strategy = enum_xtxpool_order_strategy_default);
};

NS_END2
