// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xindexstore/xindexstore_face.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvcertauth.h"
#include "xmbus/xmessage_bus.h"

#include <string>
#include <vector>

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;
using namespace top::data;

enum enum_xtx_type_socre_t {
    enum_xtx_type_socre_normal = 0,
    enum_xtx_type_socre_system = 1,
};

class xtx_para_t {
public:
    xtx_para_t() {
    }
    xtx_para_t(uint16_t charge_score, uint16_t type_score, uint64_t timestamp, uint64_t check_unit_height, std::string check_unit_hash)
      : m_charge_score(charge_score), m_type_score(type_score), m_timestamp(timestamp), m_check_unit_height(check_unit_height), m_check_unit_hash(check_unit_hash) {
    }
    void set_charge_score(uint16_t score) {
        m_charge_score = score;
    }
    uint16_t get_charge_score() const {
        return m_charge_score;
    }
    void set_tx_type_score(uint16_t score) {
        m_type_score = score;
    }
    uint16_t get_tx_type_score() const {
        return m_type_score;
    }
    void set_timestamp(uint64_t timestamp) {
        m_timestamp = timestamp;
    }
    uint64_t get_timestamp() const {
        return m_timestamp;
    }
    void set_check_unit_height(uint64_t check_unit_height) {
        m_check_unit_height = check_unit_height;
    }
    uint64_t get_check_unit_height() const {
        return m_check_unit_height;
    }
    void set_check_unit_hash(std::string check_unit_hash) {
        m_check_unit_hash = check_unit_hash;
    }
    std::string get_check_unit_hash() const {
        return m_check_unit_hash;
    }

private:
    uint16_t m_charge_score{0};
    uint16_t m_type_score{0};
    uint64_t m_timestamp{0};
    uint64_t m_check_unit_height{0};
    std::string m_check_unit_hash;
};

class xtx_entry {
public:
    xtx_entry(const xcons_transaction_ptr_t & tx, const xtx_para_t & para) : m_tx(tx), m_para(para) {
    }
    xtx_para_t & get_para() {
        return m_para;
    }
    const xcons_transaction_ptr_t & get_tx() const {
        return m_tx;
    }

private:
    xcons_transaction_ptr_t m_tx;
    xtx_para_t m_para;
};

class xready_account_t {
public:
    xready_account_t(const std::string & account) : m_account(account) {
    }
    xready_account_t(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) : m_account(account), m_txs(txs) {
    }
    const std::vector<xcons_transaction_ptr_t> & get_txs() const {
        return m_txs;
    }
    bool put_tx(const xcons_transaction_ptr_t & tx);
    const std::string & get_addr() const {
        return m_account;
    }

private:
    std::string m_account;
    mutable std::vector<xcons_transaction_ptr_t> m_txs;
};

using ready_accounts_t = std::vector<std::shared_ptr<xready_account_t>>;

class tx_info_t {
public:
    tx_info_t(const std::string & account_addr, const uint256_t & hash, base::enum_transaction_subtype subtype) : m_account_addr(account_addr), m_hash(hash), m_subtype(subtype) {
    }
    tx_info_t(const xcons_transaction_ptr_t & cons_tx)
      : m_account_addr(cons_tx->get_account_addr()), m_hash(cons_tx->get_transaction()->digest()), m_subtype(cons_tx->get_tx_subtype()) {
    }

    const std::string & get_addr() const {
        return m_account_addr;
    }
    const uint256_t & get_hash() const {
        return m_hash;
    }
    base::enum_transaction_subtype get_subtype() const {
        return m_subtype;
    }
    const std::string get_hash_str() const {
        return std::string(reinterpret_cast<char *>(m_hash.data()), m_hash.size());
    }

private:
    std::string m_account_addr;
    uint256_t m_hash;
    base::enum_transaction_subtype m_subtype;
};

class xtxs_pack_para_t {
public:
    xtxs_pack_para_t(const std::string & table_addr,
                     const base::xreceiptid_state_ptr_t receiptid_state_highqc,
                     uint16_t send_txs_max_num,
                     uint16_t recv_txs_max_num,
                     uint16_t confirm_txs_max_num)
      : m_table_addr(table_addr)
      , m_receiptid_state_highqc(receiptid_state_highqc)
      , m_send_txs_max_num(send_txs_max_num)
      , m_recv_txs_max_num(recv_txs_max_num)
      , m_confirm_txs_max_num(confirm_txs_max_num) {
    }
    const std::string & get_table_addr() const {
        return m_table_addr;
    }
    const base::xreceiptid_state_ptr_t & get_receiptid_state_highqc() const {
        return m_receiptid_state_highqc;
    }
    uint16_t get_send_txs_max_num() const {
        return m_send_txs_max_num;
    }
    uint16_t get_recv_txs_max_num() const {
        return m_recv_txs_max_num;
    }
    uint16_t get_confirm_txs_max_num() const {
        return m_confirm_txs_max_num;
    }

private:
    std::string m_table_addr;
    base::xreceiptid_state_ptr_t m_receiptid_state_highqc;
    uint16_t m_send_txs_max_num;
    uint16_t m_recv_txs_max_num;
    uint16_t m_confirm_txs_max_num;
};

class xtxpool_face_t : public base::xobject_t {
public:
    virtual int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx) = 0;
    virtual int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx) = 0;
    virtual const xcons_transaction_ptr_t pop_tx(const tx_info_t & txinfo) = 0;
    virtual ready_accounts_t pop_ready_accounts(const std::string & table_addr, uint32_t count) = 0;
    virtual ready_accounts_t get_ready_accounts(const std::string & table_addr, uint32_t count) = 0;
    virtual ready_accounts_t get_ready_accounts(const xtxs_pack_para_t & pack_para) = 0;
    virtual std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para) = 0;
    virtual std::vector<xcons_transaction_ptr_t> get_ready_txs(const std::string & table_addr, uint32_t count) = 0;
    virtual const std::shared_ptr<xtx_entry> query_tx(const std::string & account, const uint256_t & hash) const = 0;
    virtual void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce, const uint256_t & latest_hash) = 0;
    virtual void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) = 0;
    virtual void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) = 0;
    virtual void on_block_confirmed(xblock_t * block) = 0;
    virtual xcons_transaction_ptr_t get_unconfirm_tx(const std::string source_addr, const uint256_t & hash) const = 0;
    virtual int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs, uint64_t latest_commit_unit_height) = 0;
    virtual int32_t reject(const xcons_transaction_ptr_t & tx, uint64_t latest_commit_unit_height, bool & deny) = 0;
    virtual const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint8_t zone, uint16_t subaddr, uint64_t now) = 0;
    virtual void update_unconfirm_accounts(uint8_t zone, uint16_t subaddr) = 0;
    virtual void update_non_ready_accounts(uint8_t zone, uint16_t subaddr) = 0;
    virtual void update_locked_txs(const std::string & table_addr, const std::vector<tx_info_t> & locked_tx_vec, const base::xreceiptid_state_ptr_t & receiptid_state) = 0;
    virtual void update_receiptid_state(const std::string & table_addr, const base::xreceiptid_state_ptr_t & receiptid_state) = 0;
};

class xtxpool_instance {
public:
    static xobject_ptr_t<xtxpool_face_t> create_xtxpool_inst(const observer_ptr<store::xstore_face_t> & store,
                                                             const observer_ptr<base::xvblockstore_t> & blockstore,
                                                             const observer_ptr<base::xvcertauth_t> & certauth,
                                                             const observer_ptr<store::xindexstorehub_t> & indexstorehub,
                                                             const observer_ptr<mbus::xmessage_bus_face_t> & bus);
};

}  // namespace xtxpool_v2
}  // namespace top
