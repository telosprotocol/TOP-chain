// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xdata/xtransaction.h"
#include "xdata/xtxreceipt.h"
#include "xdata/xblockbody.h"

namespace top { namespace data {

class xcons_transaction_t : public xbase_dataunit_t<xcons_transaction_t, xdata_type_cons_transaction> {
 public:
    xcons_transaction_t() = default;
    xcons_transaction_t(xtransaction_t* raw_tx);
    xcons_transaction_t(xtransaction_t* tx, xtx_receipt_ptr_t receipt);
 protected:
    virtual ~xcons_transaction_t();
 private:
    xcons_transaction_t & operator = (const xcons_transaction_t & other);

 protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    std::string                     dump(bool detail = false) const;
    std::string                     dump_execute_state() const {return m_execute_state.dump();}
    inline xtransaction_t*          get_transaction() const {return m_tx;}
    const xlightunit_output_entity_t*     get_tx_info() const {return m_receipt->get_tx_info();}
    bool                            verify_cons_transaction();
    void                            set_commit_prove_with_parent_cert(base::xvqcert_t* prove_cert);
    void                            set_commit_prove_with_self_cert(base::xvqcert_t* prove_cert);
    bool                            is_commit_prove_cert_set() const;

    const std::string &     get_source_addr()const {return m_tx->get_source_addr();}
    const std::string &     get_account_addr() const {return is_recv_tx()? m_tx->get_target_addr() : m_tx->get_source_addr();}
    const std::string &     get_target_addr()const {return m_tx->get_target_addr();}
    const std::string &     get_receipt_source_account()const;
    const std::string &     get_receipt_target_account()const;
    const xaction_t &       get_source_action()const {return m_tx->get_source_action();}
    const xaction_t &       get_target_action()const {return m_tx->get_target_action();}

    enum_transaction_subtype  get_tx_subtype() const {return (enum_transaction_subtype)m_tx->get_tx_subtype();}
    std::string             get_tx_subtype_str() const {return m_tx->get_tx_subtype_str();}
    std::string             get_tx_dump_key() const {return base::xvtxkey_t::transaction_hash_subtype_to_string(m_tx->get_digest_str(), get_tx_subtype());}
    bool                    is_self_tx() const {return m_tx->get_tx_subtype() == enum_transaction_subtype_self;}
    bool                    is_send_tx() const {return m_tx->get_tx_subtype() == enum_transaction_subtype_send;}
    bool                    is_recv_tx() const {return m_tx->get_tx_subtype() == enum_transaction_subtype_recv;}
    bool                    is_confirm_tx() const {return m_tx->get_tx_subtype() == enum_transaction_subtype_confirm;}
    uint64_t                get_clock() const {return m_receipt->get_unit_cert()->get_clock();}
    std::string             get_digest_hex_str() const {return m_tx->get_digest_hex_str();}
    uint32_t                get_last_action_used_tgas() const;
    uint32_t                get_last_action_used_deposit() const;
    uint32_t                get_last_action_used_disk() const;
    uint32_t                get_last_action_send_tx_lock_tgas() const;
    uint32_t                get_last_action_recv_tx_use_send_tx_tgas() const;
    enum_xunit_tx_exec_status   get_last_action_exec_status() const;
    uint64_t                get_last_action_receipt_id() const;
    base::xtable_shortid_t  get_last_action_receipt_id_tableid() const;

 public:
    const xtransaction_exec_state_t & get_tx_execute_state() const {return m_execute_state;}
    void                    set_current_used_disk(uint32_t disk) {m_execute_state.set_used_disk(disk);}
    uint32_t                get_current_used_disk() const {return m_execute_state.get_used_disk();}
    void                    set_current_used_tgas(uint32_t tgas) {m_execute_state.set_used_tgas(tgas);}
    uint32_t                get_current_used_tgas() const {return m_execute_state.get_used_tgas();}
    void                    set_current_used_deposit(uint32_t deposit) {m_execute_state.set_used_deposit(deposit);}
    uint32_t                get_current_used_deposit() const {return m_execute_state.get_used_deposit();}
    void                    set_current_beacon_service_fee(uint64_t to_burn) {m_execute_state.set_beacon_service_fee(to_burn);}
    void                    set_current_send_tx_lock_tgas(uint32_t tgas) {m_execute_state.set_send_tx_lock_tgas(tgas);}
    uint32_t                get_current_send_tx_lock_tgas() const {return m_execute_state.get_send_tx_lock_tgas();}
    void                    set_current_recv_tx_use_send_tx_tgas(uint32_t tgas) {m_execute_state.set_recv_tx_use_send_tx_tgas(tgas);}
    uint32_t                get_current_recv_tx_use_send_tx_tgas() const {return m_execute_state.get_recv_tx_use_send_tx_tgas();}
    void                    set_current_exec_status(enum_xunit_tx_exec_status status) {m_execute_state.set_tx_exec_status(status);}
    void                    set_self_burn_balance(uint64_t value) {m_execute_state.set_self_burn_balance(value);}
    uint32_t                get_current_receipt_id() const {return m_execute_state.get_receipt_id();}
    void                    set_current_receipt_id(base::xtable_shortid_t tableid, uint64_t value) {m_execute_state.set_receipt_id(tableid, value);}

    void                    set_unit_height(uint64_t unit_height) {m_unit_height = unit_height;}
    uint64_t                get_unit_height() const noexcept {return m_unit_height;}


    // enum_xunit_tx_exec_status   get_current_exec_status() const {return m_execute_state.get_exec_status();}
    const base::xvqcert_t*  get_unit_cert() const {return m_receipt->get_unit_cert();}
    xtx_receipt_ptr_t       get_receipt() const {return m_receipt;}

 public:
    bool                    get_tx_info_prove_cert_and_account(base::xvqcert_t* & cert, std::string & account) const;
    bool                    get_commit_prove_cert_and_account(base::xvqcert_t* & cert, std::string & account) const;

 private:
    void                    update_transation();
    uint64_t                get_dump_receipt_id() const;

 private:
    xtransaction_t*     m_tx{nullptr};
    xtx_receipt_ptr_t   m_receipt{nullptr};

 private:  // local member, should not serialize
    xtransaction_exec_state_t    m_execute_state;
    uint64_t                     m_unit_height{0};
};

using xcons_transaction_ptr_t = xobject_ptr_t<xcons_transaction_t>;

}  // namespace data
}  // namespace top
