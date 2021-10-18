// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xvledger/xtxreceipt.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvtxindex.h"
#include "xdata/xtransaction.h"
#include "xdata/xlightunit_info.h"

namespace top { namespace data {

using base::enum_transaction_subtype;
using base::enum_transaction_subtype_self;
using base::enum_transaction_subtype_send;
using base::enum_transaction_subtype_recv;
using base::enum_transaction_subtype_confirm;

class xcons_transaction_t : public xbase_dataunit_t<xcons_transaction_t, xdata_type_cons_transaction> {
 public:
    xcons_transaction_t();
    xcons_transaction_t(xtransaction_t* raw_tx);
    // xcons_transaction_t(xtransaction_t* tx, const base::xtx_receipt_ptr_t & receipt);
    xcons_transaction_t(const base::xfull_txreceipt_t & full_txreceipt);
 protected:
    virtual ~xcons_transaction_t();
 private:
    xcons_transaction_t & operator = (const xcons_transaction_t & other);

 protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    bool                            set_raw_tx(xtransaction_t* raw_tx);  //on-demand load and set raw tx
 public:
    std::string                     dump(bool detail = false) const;
    std::string                     dump_execute_state() const {return m_execute_state.dump();}
    inline xtransaction_t*          get_transaction() const {return m_tx.get();}
    std::string                     get_tx_hash() const;
    uint256_t                       get_tx_hash_256() const;
    uint16_t                        get_tx_type() const {return m_tx->get_tx_type();}
    bool                            verify_cons_transaction();

    base::xtable_index_t            get_self_table_index() const;
    base::xtable_index_t            get_peer_table_index() const;
    base::xtable_shortid_t          get_self_tableid() const;
    base::xtable_shortid_t          get_peer_tableid() const;

    const std::string &     get_source_addr()const {return m_tx->get_source_addr();}
    std::string             get_account_addr() const;
    const std::string &     get_target_addr()const {return m_tx->get_target_addr();}
    uint64_t                get_tx_nonce()const {return m_tx->get_tx_nonce();}
    uint64_t                get_tx_last_nonce()const {return m_tx->get_last_nonce();}

    enum_transaction_subtype  get_tx_subtype() const {return m_subtype;}
    std::string             get_tx_subtype_str() const {return base::xvtxkey_t::transaction_subtype_to_string(m_subtype);}
    std::string             get_tx_dump_key() const {return base::xvtxkey_t::transaction_hash_subtype_to_string(m_tx->get_digest_str(), get_tx_subtype());}
    bool                    is_self_tx() const {return get_tx_subtype() == enum_transaction_subtype_self;}
    bool                    is_send_tx() const {return get_tx_subtype() == enum_transaction_subtype_send;}
    bool                    is_recv_tx() const {return get_tx_subtype() == enum_transaction_subtype_recv;}
    bool                    is_confirm_tx() const {return get_tx_subtype() == enum_transaction_subtype_confirm;}
    std::string             get_digest_hex_str() const {return m_tx->get_digest_hex_str();}
    uint32_t                get_last_action_used_tgas() const;
    uint32_t                get_last_action_used_deposit() const;
    uint32_t                get_last_action_send_tx_lock_tgas() const;
    uint32_t                get_last_action_recv_tx_use_send_tx_tgas() const;
    enum_xunit_tx_exec_status   get_last_action_exec_status() const;
    uint64_t                get_last_action_receipt_id() const;
    uint64_t                get_last_action_sender_confirmed_receipt_id() const;
    data::xreceipt_data_t   get_last_action_receipt_data() const;

 public:
    const xtransaction_exec_state_t & get_tx_execute_state() const {return m_execute_state;}
    void                    set_current_used_disk(uint32_t disk) {m_execute_state.set_used_disk(disk);}
    uint32_t                get_current_used_disk() const {return m_execute_state.get_used_disk();}
    void                    set_current_used_tgas(uint32_t tgas) {m_execute_state.set_used_tgas(tgas);}
    uint32_t                get_current_used_tgas() const {return m_execute_state.get_used_tgas();}
    void                    set_current_used_deposit(uint32_t deposit) {m_execute_state.set_used_deposit(deposit);}
    uint32_t                get_current_used_deposit() const {return m_execute_state.get_used_deposit();}
    void                    set_current_send_tx_lock_tgas(uint32_t tgas) {m_execute_state.set_send_tx_lock_tgas(tgas);}
    uint32_t                get_current_send_tx_lock_tgas() const {return m_execute_state.get_send_tx_lock_tgas();}
    void                    set_current_recv_tx_use_send_tx_tgas(uint32_t tgas) {m_execute_state.set_recv_tx_use_send_tx_tgas(tgas);}
    uint32_t                get_current_recv_tx_use_send_tx_tgas() const {return m_execute_state.get_recv_tx_use_send_tx_tgas();}
    void                    set_current_exec_status(enum_xunit_tx_exec_status status) {m_execute_state.set_tx_exec_status(status);}
    enum_xunit_tx_exec_status   get_current_exec_status() const {return m_execute_state.get_tx_exec_status();}
    uint32_t                get_current_receipt_id() const {return m_execute_state.get_receipt_id();}
    void                    set_current_receipt_id(base::xtable_shortid_t self_tableid, base::xtable_shortid_t peer_tableid, uint64_t receiptid) {m_execute_state.set_receipt_id(self_tableid, peer_tableid, receiptid);}
    void                    set_current_sender_confirmed_receipt_id(uint64_t receiptid) {m_execute_state.set_sender_confirmed_receipt_id(receiptid);}

    uint64_t                get_receipt_clock() const {return get_prove_cert()->get_clock();}
    uint64_t                get_receipt_gmtime() const {return get_prove_cert()->get_gmtime();}
    bool                    is_receipt_valid() const {return m_receipt->is_valid();}
    void                    set_receipt_data(data::xreceipt_data_t data) {return m_execute_state.set_receipt_data(data);}
    data::xreceipt_data_t   get_receipt_data() const {return m_execute_state.get_receipt_data();}

 public:  // for debug use
    void                    set_push_pool_timestamp(uint64_t push_pool_timestamp) {m_push_pool_timestamp = push_pool_timestamp;};
    uint64_t                get_push_pool_timestamp() const {return m_push_pool_timestamp;}

 public:
    xobject_ptr_t<base::xvqcert_t> get_receipt_prove_cert_and_account(std::string & account) const;

 private:
    void                    set_tx_subtype(enum_transaction_subtype _subtype);
    void                    update_transation();
    uint64_t                get_dump_receipt_id() const;
    const xobject_ptr_t<base::xvqcert_t> &  get_prove_cert() const {return m_receipt->get_prove_cert();}
    base::xtable_shortid_t  get_last_action_self_tableid() const;
    base::xtable_shortid_t  get_last_action_peer_tableid() const;

 private:
    xtransaction_ptr_t          m_tx{nullptr};
    base::xtx_receipt_ptr_t     m_receipt{nullptr};

 private:  // local member, should not serialize
    enum_transaction_subtype    m_subtype{base::enum_transaction_subtype_invalid};
    uint64_t                    m_push_pool_timestamp{0};
    xtransaction_exec_state_t   m_execute_state;
    std::string                 m_dump_str;
};

using xcons_transaction_ptr_t = xobject_ptr_t<xcons_transaction_t>;

}  // namespace data
}  // namespace top
