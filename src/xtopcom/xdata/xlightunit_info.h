// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xbase/xobject_ptr.h"
#include "xbasic/xversion.h"
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xvaccount.h"
#include "xdata/xtransaction.h"
#include "xdata/xblock_paras.h"
#include "xdata/xblockaction.h"

namespace top { namespace data {

// the transaction state change after execute
class xtransaction_exec_state_t : public xblockpara_base_t {
 public:
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_SEND_TX_LOCK_TGAS              = "0";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_RECV_TX_USE_SEND_TX_TGAS       = "1";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_TX_USED_TGAS                   = "2";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_TX_USED_DEPOSIT                = "3";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_TX_USED_DISK                   = "4";
    static XINLINE_CONSTEXPR char const * XTX_STATE_TX_EXEC_STATUS                     = "7";
    static XINLINE_CONSTEXPR char const * XTX_RECEIPT_ID                               = "8";
    static XINLINE_CONSTEXPR char const * XTX_RECEIPT_ID_SELF_TABLE_ID                 = "9";
    static XINLINE_CONSTEXPR char const * XTX_RECEIPT_ID_PEER_TABLE_ID                 = "a";

 public:
    xtransaction_exec_state_t();
    xtransaction_exec_state_t(const std::map<std::string, std::string> & values);

 public:
    void        set_used_disk(uint32_t value) {set_value(XPROPERTY_FEE_TX_USED_DISK, value);}
    void        set_used_tgas(uint32_t value) {set_value(XPROPERTY_FEE_TX_USED_TGAS, value);}
    void        set_used_deposit(uint32_t value) {set_value(XPROPERTY_FEE_TX_USED_DEPOSIT, value);}
    void        set_send_tx_lock_tgas(uint32_t value) {set_value(XPROPERTY_FEE_SEND_TX_LOCK_TGAS, value);}
    void        set_recv_tx_use_send_tx_tgas(uint32_t value) {set_value(XPROPERTY_FEE_RECV_TX_USE_SEND_TX_TGAS, value);}
    void        set_tx_exec_status(enum_xunit_tx_exec_status value);
    void        set_receipt_id(base::xtable_shortid_t self_tableid, base::xtable_shortid_t peer_tableid, uint64_t receiptid);

 public:
    uint32_t    get_used_disk()const {return get_value_uint32(XPROPERTY_FEE_TX_USED_DISK);}
    uint32_t    get_used_tgas()const {return get_value_uint32(XPROPERTY_FEE_TX_USED_TGAS);}
    uint32_t    get_used_deposit()const {return get_value_uint32(XPROPERTY_FEE_TX_USED_DEPOSIT);}
    uint32_t    get_send_tx_lock_tgas()const {return get_value_uint32(XPROPERTY_FEE_SEND_TX_LOCK_TGAS);}
    uint32_t    get_recv_tx_use_send_tx_tgas()const {return get_value_uint32(XPROPERTY_FEE_RECV_TX_USE_SEND_TX_TGAS);}
    enum_xunit_tx_exec_status   get_tx_exec_status() const;
    uint64_t    get_receipt_id()const {return get_value_uint64(XTX_RECEIPT_ID);}
    base::xtable_shortid_t    get_receipt_id_self_tableid()const {return get_value_uint16(XTX_RECEIPT_ID_SELF_TABLE_ID);}
    base::xtable_shortid_t    get_receipt_id_peer_tableid()const {return get_value_uint16(XTX_RECEIPT_ID_PEER_TABLE_ID);}
};

class xlightunit_tx_info_t : public xlightunit_action_t {
 public:
    xlightunit_tx_info_t(const base::xvaction_t & _action, xtransaction_t* raw_tx)
    : xlightunit_action_t(_action) {
        if (raw_tx != nullptr) {
            raw_tx->add_ref();
            m_raw_tx.attach(raw_tx);
        }
    }

 public:
    const xtransaction_ptr_t &  get_raw_tx() const {return m_raw_tx;}
    uint64_t                    get_last_trans_nonce() const;

 private:
    xtransaction_ptr_t          m_raw_tx;
};

using xlightunit_tx_info_ptr_t = std::shared_ptr<xlightunit_tx_info_t>;

}  // namespace data
}  // namespace top
