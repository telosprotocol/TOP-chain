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
#include "xdata/xpropertylog.h"
#include "xdata/xblock_paras.h"
#include "xdata/xblockbody.h"
namespace top { namespace data {

// the transaction state change after execute
class xtransaction_exec_state_t : public xblockpara_base_t {
 protected:
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_SEND_TX_LOCK_TGAS              = "0";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_RECV_TX_USE_SEND_TX_TGAS       = "1";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_TX_USED_TGAS                   = "2";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_TX_USED_DEPOSIT                = "3";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_TX_USED_DISK                   = "4";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_TX_BEACON_SERVICE_FEE          = "5";
    static XINLINE_CONSTEXPR char const * XPROPERTY_FEE_TX_SELF_BURN_BALANCE           = "6";
    static XINLINE_CONSTEXPR char const * XTX_STATE_TX_EXEC_STATUS                     = "7";
    static XINLINE_CONSTEXPR char const * XTX_RECEIPT_ID                               = "8";
    static XINLINE_CONSTEXPR char const * XTX_RECEIPT_ID_TABLE_ID                      = "9";

 public:
    void        set_used_disk(uint32_t value) {set_value(XPROPERTY_FEE_TX_USED_DISK, value);}
    void        set_used_tgas(uint32_t value) {set_value(XPROPERTY_FEE_TX_USED_TGAS, value);}
    void        set_used_deposit(uint32_t value) {set_value(XPROPERTY_FEE_TX_USED_DEPOSIT, value);}
    void        set_beacon_service_fee(uint64_t value) {set_value(XPROPERTY_FEE_TX_BEACON_SERVICE_FEE, value);}
    void        set_self_burn_balance(uint64_t value) {set_value(XPROPERTY_FEE_TX_SELF_BURN_BALANCE, value);}
    void        set_send_tx_lock_tgas(uint32_t value) {set_value(XPROPERTY_FEE_SEND_TX_LOCK_TGAS, value);}
    void        set_recv_tx_use_send_tx_tgas(uint32_t value) {set_value(XPROPERTY_FEE_RECV_TX_USE_SEND_TX_TGAS, value);}
    void        set_tx_exec_status(enum_xunit_tx_exec_status value);
    void        set_receipt_id(base::xtable_shortid_t tableid, uint64_t value);

 public:
    uint32_t    get_used_disk()const {return get_value_uint32(XPROPERTY_FEE_TX_USED_DISK);}
    uint32_t    get_used_tgas()const {return get_value_uint32(XPROPERTY_FEE_TX_USED_TGAS);}
    uint32_t    get_used_deposit()const {return get_value_uint32(XPROPERTY_FEE_TX_USED_DEPOSIT);}
    uint64_t    get_beacon_service_fee()const {return get_value_uint64(XPROPERTY_FEE_TX_BEACON_SERVICE_FEE);}
    uint64_t    get_self_burn_balance()const {return get_value_uint64(XPROPERTY_FEE_TX_SELF_BURN_BALANCE);}
    uint32_t    get_send_tx_lock_tgas()const {return get_value_uint32(XPROPERTY_FEE_SEND_TX_LOCK_TGAS);}
    uint32_t    get_recv_tx_use_send_tx_tgas()const {return get_value_uint32(XPROPERTY_FEE_RECV_TX_USE_SEND_TX_TGAS);}
    enum_xunit_tx_exec_status   get_tx_exec_status() const;
    uint64_t    get_receipt_id()const {return get_value_uint64(XTX_RECEIPT_ID);}
    base::xtable_shortid_t    get_receipt_id_tableid()const {return get_value_uint16(XTX_RECEIPT_ID_TABLE_ID);}
};

// input tx of lightunit has some propertys, such as tx_type, tx_exec_status, tx_clock.
class xinput_tx_propertys_t : public xblockpara_base_t {
 protected:
    static XINLINE_CONSTEXPR char const * PARA_CONTRACT_CREATE              = "0";
    static XINLINE_CONSTEXPR char const * PARA_LAST_TX_EXEC_STATUS          = "1";
    static XINLINE_CONSTEXPR char const * PARA_LAST_TX_CLOCK                = "2";
 public:
    xinput_tx_propertys_t() = default;
    xinput_tx_propertys_t(bool _is_contract_create, enum_xunit_tx_exec_status last_action_status, uint64_t last_tx_clock);
 public:
    void        set_is_contract_create(bool value) {set_value(PARA_CONTRACT_CREATE, value);}
    void        set_last_tx_exec_status(enum_xunit_tx_exec_status value);
    void        set_tx_clock(uint64_t value) {set_value(PARA_LAST_TX_CLOCK, value);}
 public:
    bool                        is_contract_create() const {return get_value_bool(PARA_CONTRACT_CREATE);}
    enum_xunit_tx_exec_status   get_last_tx_exec_status() const;
    uint64_t                    get_tx_clock() const {return get_value_uint64(PARA_LAST_TX_CLOCK);}
};

class xlightunit_input_entity_t final: public xventity_face_t<xlightunit_input_entity_t, xdata_type_lightunit_input_entity> {
 public:
    xlightunit_input_entity_t();
    explicit xlightunit_input_entity_t(base::enum_transaction_subtype type,
                                        xtransaction_t * tx,
                                        bool _is_contract_create,
                                        enum_xunit_tx_exec_status last_action_status,
                                        uint64_t last_tx_clock);
 protected:
    ~xlightunit_input_entity_t();
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;
 private:
    xlightunit_input_entity_t & operator = (const xlightunit_input_entity_t & other);
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}
 public:
    const std::string &         get_tx_hash() const {return m_tx_key.get_tx_hash();}
    bool                        is_self_tx() const {return m_tx_key.is_self_tx();}
    bool                        is_send_tx() const {return m_tx_key.is_send_tx();}
    bool                        is_recv_tx() const {return m_tx_key.is_recv_tx();}
    bool                        is_confirm_tx() const {return m_tx_key.is_confirm_tx();}
    base::enum_transaction_subtype    get_tx_subtype() const {return m_tx_key.get_tx_subtype();}
    std::string                 get_tx_dump_key() const {return m_tx_key.get_tx_dump_key();}
    const base::xvtxkey_t &    get_tx_key() const {return m_tx_key;}

    bool                        is_contract_create() const {return m_inputtx_props.is_contract_create();}
    enum_xunit_tx_exec_status   get_last_action_exec_status() const {return m_inputtx_props.get_last_tx_exec_status();}
    uint64_t                    get_tx_clock() const {return m_inputtx_props.get_tx_clock();}
    const xinput_tx_propertys_t & get_input_tx_propertys() const {return m_inputtx_props;}
    const xtransaction_ptr_t &  get_raw_tx() const {return m_raw_tx;}

    std::string                 dump() const;

 private:
    base::xvtxkey_t             m_tx_key;
    xinput_tx_propertys_t       m_inputtx_props;
    xtransaction_ptr_t          m_raw_tx{nullptr};  // TODO(jimmy) raw tx is a resource
};
using xlightunit_input_entity_ptr_t = xobject_ptr_t<xlightunit_input_entity_t>;

class xlightunit_output_entity_t final: public xventity_face_t<xlightunit_output_entity_t, xdata_type_lightunit_output_entity> {
 public:
    xlightunit_output_entity_t();
    xlightunit_output_entity_t(base::enum_transaction_subtype type, xtransaction_t * tx, const xtransaction_exec_state_t & txstate);
 protected:
    ~xlightunit_output_entity_t();
    int32_t     do_write(base::xstream_t & stream) override;
    int32_t     do_read(base::xstream_t & stream) override;
 public:
    virtual const std::string query_value(const std::string & key) override {return get_merkle_leaf();}
 public:
    const std::string &         get_tx_hash() const {return m_tx_key.get_tx_hash();}
    bool                        is_self_tx() const {return m_tx_key.is_self_tx();}
    bool                        is_send_tx() const {return m_tx_key.is_send_tx();}
    bool                        is_recv_tx() const {return m_tx_key.is_recv_tx();}
    bool                        is_confirm_tx() const {return m_tx_key.is_confirm_tx();}
    base::enum_transaction_subtype    get_tx_subtype() const {return m_tx_key.get_tx_subtype();}
    std::string                 get_tx_dump_key() const {return m_tx_key.get_tx_dump_key();}
    uint256_t                   get_tx_hash_256() const {return m_tx_key.get_tx_hash_256();}
    std::string                 get_tx_hex_hash() const {return m_tx_key.get_tx_hex_hash();}

    const xtransaction_exec_state_t &   get_tx_exec_state() const {return m_exec_state;}

 public:
    std::string dump() const override;
    const std::string get_merkle_leaf();

 private:
    base::xvtxkey_t             m_tx_key;
    xtransaction_exec_state_t   m_exec_state;
};
using xlightunit_output_entity_ptr_t = xobject_ptr_t<xlightunit_output_entity_t>;

class xlightunit_tx_info_t {
 public:
    xlightunit_tx_info_t(const base::xvtxkey_t & txkey,
                    xtransaction_t* raw_tx,
                    const xinput_tx_propertys_t & inputtx_props,
                    const xtransaction_exec_state_t & exec_state) {
        m_tx_key = txkey;
        if (raw_tx != nullptr) {
            raw_tx->add_ref();
            m_raw_tx.attach(raw_tx);
        }
        m_inputtx_props = inputtx_props;
        m_exec_state = exec_state;
    }

 public:
    const base::xvtxkey_t &     get_tx_key() const {return m_tx_key;}
    const std::string &         get_tx_hash() const {return m_tx_key.get_tx_hash();}
    bool                        is_self_tx() const {return m_tx_key.is_self_tx();}
    bool                        is_send_tx() const {return m_tx_key.is_send_tx();}
    bool                        is_recv_tx() const {return m_tx_key.is_recv_tx();}
    bool                        is_confirm_tx() const {return m_tx_key.is_confirm_tx();}
    base::enum_transaction_subtype    get_tx_subtype() const {return m_tx_key.get_tx_subtype();}
    std::string                 get_tx_dump_key() const {return m_tx_key.get_tx_dump_key();}
    uint256_t                   get_tx_hash_256() const {return m_tx_key.get_tx_hash_256();}
    std::string                 get_tx_hex_hash() const {return m_tx_key.get_tx_hex_hash();}
    std::string                 get_tx_subtype_str() const {return m_tx_key.get_tx_subtype_str();}

    bool                        is_contract_create() const {return m_inputtx_props.is_contract_create();}
    const xtransaction_ptr_t &  get_raw_tx() const {return m_raw_tx;}
    const xtransaction_exec_state_t &   get_exec_state() const {return m_exec_state;}
    uint32_t                    get_used_disk()const {return get_exec_state().get_used_disk();}
    uint32_t                    get_used_tgas()const {return get_exec_state().get_used_tgas();}
    uint32_t                    get_used_deposit()const {return get_exec_state().get_used_deposit();}
    uint64_t                    get_beacon_service_fee()const {return get_exec_state().get_beacon_service_fee();}
    uint32_t                    get_send_tx_lock_tgas()const {return get_exec_state().get_send_tx_lock_tgas();}
    uint32_t                    get_recv_tx_use_send_tx_tgas()const {return get_exec_state().get_recv_tx_use_send_tx_tgas();}
    enum_xunit_tx_exec_status   get_tx_exec_status() const {return get_exec_state().get_tx_exec_status();}
    uint64_t                    get_receipt_id() const {return get_exec_state().get_receipt_id();}
    base::xtable_shortid_t       get_receipt_id_tableid()const {return get_exec_state().get_receipt_id_tableid();}
    enum_xunit_tx_exec_status   get_last_action_exec_status() const {return m_inputtx_props.get_last_tx_exec_status();}
    uint64_t                    get_tx_clock() const {return m_inputtx_props.get_tx_clock();}
    uint64_t                    get_last_trans_nonce() const;
    std::string                 dump() const;

 private:
    base::xvtxkey_t             m_tx_key;
    xinput_tx_propertys_t       m_inputtx_props;
    xtransaction_ptr_t          m_raw_tx;
    xtransaction_exec_state_t   m_exec_state;
};

using xlightunit_tx_info_ptr_t = std::shared_ptr<xlightunit_tx_info_t>;

}  // namespace data
}  // namespace top
