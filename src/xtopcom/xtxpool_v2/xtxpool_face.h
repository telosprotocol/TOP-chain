// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xmessage_id.h"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtable_bstate.h"
#include "xmbus/xmessage_bus.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvcertauth.h"

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

XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_send_receipt, 0x00000001);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_recv_receipt, 0x00000002);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_recv_receipt, 0x00000003);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_confirm_receipt, 0x00000004);  // keep it for compatibility
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_push_receipt, 0x00000005);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_confirm_receipt_v2, 0x00000006);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_receipt_id_state, 0x00000007);
// XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_neighbor_sync_req, 0x00000008);
// XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_neighbor_sync_rsp, 0x00000009);

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
    tx_info_t(const xcons_transaction_ptr_t & cons_tx) : m_account_addr(cons_tx->get_account_addr()), m_hash(cons_tx->get_tx_hash_256()), m_subtype(cons_tx->get_tx_subtype()) {
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
                     const data::xtablestate_ptr_t & table_state_highqc,
                    //  const std::map<std::string, uint64_t> & locked_nonce_map,
                     uint16_t all_txs_max_num,
                     uint16_t confirm_and_recv_txs_max_num,
                     uint16_t confirm_txs_max_num)
      : m_table_addr(table_addr)
      , m_table_state_highqc(table_state_highqc)
    //   , m_locked_nonce_map(locked_nonce_map)
      , m_all_txs_max_num(all_txs_max_num)
      , m_confirm_and_recv_txs_max_num(confirm_and_recv_txs_max_num)
      , m_confirm_txs_max_num(confirm_txs_max_num) {
    }
    const std::string & get_table_addr() const {
        return m_table_addr;
    }
    const data::xtablestate_ptr_t & get_table_state_highqc() const {
        return m_table_state_highqc;
    }
    // const std::map<std::string, uint64_t> & get_locked_nonce_map() const {
    //     return m_locked_nonce_map;
    // }
    uint16_t get_all_txs_max_num() const {
        return m_all_txs_max_num;
    }
    uint16_t get_confirm_and_recv_txs_max_num() const {
        return m_confirm_and_recv_txs_max_num;
    }
    uint16_t get_confirm_txs_max_num() const {
        return m_confirm_txs_max_num;
    }

private:
    std::string m_table_addr;
    data::xtablestate_ptr_t m_table_state_highqc;
    // std::map<std::string, uint64_t> m_locked_nonce_map;
    uint16_t m_all_txs_max_num;
    uint16_t m_confirm_and_recv_txs_max_num;
    uint16_t m_confirm_txs_max_num;
};

class xtxpool_table_lacking_receipt_ids_t {
public:
    xtxpool_table_lacking_receipt_ids_t(base::xtable_shortid_t peer_sid, std::vector<uint64_t> receipt_ids) : m_peer_sid(peer_sid), m_lacking_receipt_ids(receipt_ids) {
    }
    const std::vector<uint64_t> & get_receipt_ids() const {
        return m_lacking_receipt_ids;
    }
    base::xtable_shortid_t get_peer_sid() const {
        return m_peer_sid;
    }

private:
    base::xtable_shortid_t m_peer_sid;
    std::vector<uint64_t> m_lacking_receipt_ids;
};

class xtxpool_table_lacking_confirm_tx_hashs_t {
public:
    xtxpool_table_lacking_confirm_tx_hashs_t(base::xtable_shortid_t peer_sid) : m_peer_sid(peer_sid) {
    }
    void add_receipt_id_hash(uint64_t receiptid, uint256_t tx_hash) {
        m_lacking_receipt_id_hashs[receiptid] = tx_hash;
    }
    base::xtable_shortid_t get_peer_sid() const {
        return m_peer_sid;
    }
    const std::map<uint64_t, uint256_t> & get_receipt_id_hashs() const {
        return m_lacking_receipt_id_hashs;
    }
    bool empty() const {
        return m_lacking_receipt_id_hashs.empty();
    }

private:
    base::xtable_shortid_t m_peer_sid;
    std::map<uint64_t, uint256_t> m_lacking_receipt_id_hashs;
};

class xtxpool_face_t : public base::xobject_t {
public:
    virtual int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx) = 0;
    virtual int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send, bool is_pulled) = 0;
    virtual const xcons_transaction_ptr_t pop_tx(const tx_info_t & txinfo) = 0;
    virtual ready_accounts_t get_ready_accounts(const xtxs_pack_para_t & pack_para) = 0;
    virtual std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para) = 0;
    virtual const std::shared_ptr<xtx_entry> query_tx(const std::string & account, const uint256_t & hash) const = 0;
    virtual void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) = 0;
    virtual void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) = 0;
    virtual void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) = 0;
    virtual void on_block_confirmed(xblock_t * block) = 0;
    virtual int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) = 0;
    virtual void refresh_table(uint8_t zone, uint16_t subaddr) = 0;
    // virtual void update_non_ready_accounts(uint8_t zone, uint16_t subaddr) = 0;
    virtual void update_table_state(const data::xtablestate_ptr_t & table_state) = 0;
    virtual void build_recv_tx(base::xtable_shortid_t from_table_sid,
                               base::xtable_shortid_t to_table_sid,
                               std::vector<uint64_t> receiptids,
                               std::vector<xcons_transaction_ptr_t> & receipts) = 0;
    virtual void build_confirm_tx(base::xtable_shortid_t from_table_sid,
                                  base::xtable_shortid_t to_table_sid,
                                  std::vector<uint64_t> receiptids,
                                  std::vector<xcons_transaction_ptr_t> & receipts) = 0;
    virtual const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const = 0;
    virtual const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const = 0;
    virtual bool need_sync_lacking_receipts(uint8_t zone, uint16_t subaddr) const = 0;
    virtual void print_statistic_values() const = 0;
    virtual void update_peer_receipt_id_state(const base::xreceiptid_state_ptr_t & receiptid_state) = 0;
};

class xtxpool_instance {
public:
    static xobject_ptr_t<xtxpool_face_t> create_xtxpool_inst(const observer_ptr<store::xstore_face_t> & store,
                                                             const observer_ptr<base::xvblockstore_t> & blockstore,
                                                             const observer_ptr<base::xvcertauth_t> & certauth,
                                                             const observer_ptr<mbus::xmessage_bus_face_t> & bus);
};

}  // namespace xtxpool_v2
}  // namespace top
