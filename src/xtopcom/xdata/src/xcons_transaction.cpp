// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xcons_transaction.h"
#include "xdata/xdata_common.h"
#include "xdata/xgenesis_data.h"
#include "xmetrics/xmetrics.h"

namespace top { namespace data {

xcons_transaction_t::xcons_transaction_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_cons_transaction, 1);
}

xcons_transaction_t::xcons_transaction_t(xtransaction_t* tx) {
    tx->add_ref();
    m_tx.attach(tx);
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_cons_transaction, 1);
    update_transation();
}

xcons_transaction_t::xcons_transaction_t(const base::xfull_txreceipt_t & full_txreceipt) {
    if (!full_txreceipt.get_tx_org_bin().empty()) {
        xtransaction_t::set_tx_by_serialized_data(m_tx, full_txreceipt.get_tx_org_bin());
    }
    m_receipt = full_txreceipt.get_txreceipt();
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_cons_transaction, 1);
    update_transation();
}

xcons_transaction_t::~xcons_transaction_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_cons_transaction, -1);
}

bool xcons_transaction_t::set_raw_tx(xtransaction_t* raw_tx) {
    if (m_tx != nullptr) { // already set raw tx
        xassert(false);
        return false;
    }
    if (m_receipt == nullptr) { // must be receipt tx
        xassert(false);
        return false;
    }
    if (raw_tx->get_digest_str() != m_receipt->get_tx_hash()) {
        xassert(false);
        return false;
    }
    raw_tx->add_ref();
    m_tx.attach(raw_tx);
    return true;
}

xobject_ptr_t<base::xvqcert_t> xcons_transaction_t::get_receipt_prove_cert_and_account(std::string & account) const {
    if (m_receipt == nullptr) {
        xerror("no receipt");
        return nullptr;
    }
    if (m_receipt->get_prove_cert() == nullptr) {
        xerror("commit prove null");
        return nullptr;
    }

    account = get_account_addr();
    if (m_receipt->get_prove_type() == base::enum_xprove_cert_type_table_justify) {
        // change to parent account
        account = account_address_to_block_address(common::xaccount_address_t(account));
    }
    return m_receipt->get_prove_cert();
}

void xcons_transaction_t::set_tx_subtype(enum_transaction_subtype _subtype) {
    m_subtype = _subtype;
}

void xcons_transaction_t::update_transation() {
    if (m_receipt == nullptr) {
        if (m_tx->get_source_addr() == m_tx->get_target_addr() || data::is_black_hole_address(common::xaccount_address_t{m_tx->get_target_addr()})) {
            set_tx_subtype(enum_transaction_subtype_self);
        } else {
            set_tx_subtype(enum_transaction_subtype_send);
        }
    } else {
        if (m_receipt->get_tx_subtype() == enum_transaction_subtype_send) {
            set_tx_subtype(enum_transaction_subtype_recv);
        } else if (m_receipt->get_tx_subtype() == enum_transaction_subtype_recv) {
            set_tx_subtype(enum_transaction_subtype_confirm);
        } else {
            xassert(0);
        }
    }
    m_dump_str = dump();  // init dump str for performance
}

int32_t xcons_transaction_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    uint8_t has_tx = m_tx != nullptr;
    stream << has_tx;
    if (m_tx != nullptr) {
        m_tx->serialize_to(stream);
    }
    uint8_t has_receipt = m_receipt != nullptr;
    stream << has_receipt;
    if (m_receipt != nullptr) {
        m_receipt->serialize_to(stream);
    }
    return CALC_LEN();
}

int32_t xcons_transaction_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    uint8_t has_tx;
    stream >> has_tx;
    if (has_tx) {
        auto _tx = base::xdataunit_t::read_from(stream);
        xassert(_tx != nullptr);
        auto tx = dynamic_cast<xtransaction_t*>(_tx);
        m_tx.attach(tx);
    }
    uint8_t has_receipt;
    stream >> has_receipt;
    if (has_receipt) {
        base::xtx_receipt_t* _receipt = dynamic_cast<base::xtx_receipt_t*>(base::xdataunit_t::read_from(stream));
        xassert(_receipt != nullptr);
        m_receipt.attach(_receipt);
    }

    update_transation();

    return CALC_LEN();
}

bool xcons_transaction_t::verify_cons_transaction() {
    bool ret = true;
    if (get_tx_subtype() == data::enum_transaction_subtype_recv || get_tx_subtype() == data::enum_transaction_subtype_confirm) {
        if (m_receipt == nullptr) {
            xerror("xcons_transaction_t::verify_cons_transaction should has receipt");
            return false;
        }

        // only check digest here for process too long zec_workload contract transaction receipt
        // should recover length check at later version
        // raw tx can been load on-demand
        if (get_transaction() != nullptr && false == get_transaction()->digest_check()) {
            xerror("xcons_transaction_t::verify_cons_transaction digest check fail");
            return false;
        }

        if (get_transaction() != nullptr && get_transaction()->get_digest_str() != m_receipt->get_tx_hash()) {
            xerror("xcons_transaction_t::verify_cons_transaction tx digest not match");
            return false;
        }
        ret = m_receipt->is_valid();
        if (!ret) {
            xwarn("xcons_transaction_t::verify_cons_transaction receipt check fail");
        }
    }
    return ret;
}

uint64_t xcons_transaction_t::get_dump_receipt_id() const {
    if (is_self_tx() || is_send_tx()) {
        return get_current_receipt_id();
    } else {
        return get_last_action_receipt_id();
    }
}

std::string xcons_transaction_t::dump(bool detail) const {
    xassert(m_receipt != nullptr || m_tx != nullptr);// should not both null

    if (!m_dump_str.empty()) {
        return m_dump_str;
    }

    std::stringstream ss;
    ss << "{";
    ss << base::xvtxkey_t::transaction_hash_subtype_to_string(get_tx_hash(), get_tx_subtype());
    base::xtable_shortid_t origin_src_tableid = is_recv_tx() ? get_peer_tableid() : get_self_tableid();
    base::xtable_shortid_t origin_dst_tableid = is_recv_tx() ? get_self_tableid() : get_peer_tableid();

    ss << ",id={" << (uint32_t)origin_src_tableid;
    ss << "->" << (uint32_t)origin_dst_tableid;
    ss << ":" << get_dump_receipt_id();
    ss << "}";
    if (is_self_tx() || is_send_tx()) {
        ss << ",nonce:" << get_transaction()->get_tx_nonce();
    }
    if (is_send_tx() || is_self_tx()) {
        if (get_transaction() != nullptr) {
            // ss << ",fire:" << get_transaction()->get_fire_timestamp();
            // ss << ",duration:" << (uint32_t)get_transaction()->get_expire_duration();
            ss << ",addr:" << get_transaction()->get_source_addr();
            if (get_transaction()->get_source_addr() != get_transaction()->get_target_addr()) {
                ss << "->" << get_transaction()->get_target_addr();
            }
        }
    }
    ss << "}";
    return ss.str();
}

uint32_t xcons_transaction_t::get_last_action_used_tgas() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XPROPERTY_FEE_TX_USED_TGAS);
        if (!value.empty()) {
            return base::xstring_utl::touint32(value);
        }
    }
    return 0;
}
uint32_t xcons_transaction_t::get_last_action_used_deposit() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XPROPERTY_FEE_TX_USED_DEPOSIT);
        if (!value.empty()) {
            return base::xstring_utl::touint32(value);
        }
    }
    return 0;
}

uint32_t xcons_transaction_t::get_last_action_send_tx_lock_tgas() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XPROPERTY_FEE_SEND_TX_LOCK_TGAS);
        if (!value.empty()) {
            return base::xstring_utl::touint32(value);
        }
    }
    return 0;
}
enum_xunit_tx_exec_status xcons_transaction_t::get_last_action_exec_status() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_STATE_TX_EXEC_STATUS);
        if (!value.empty()) {
            return (enum_xunit_tx_exec_status)base::xstring_utl::touint32(value);
        }
    }
    return (enum_xunit_tx_exec_status)0;
}

uint32_t xcons_transaction_t::get_last_action_recv_tx_use_send_tx_tgas() const {
    if (m_receipt != nullptr) {
        xassert(m_receipt->is_recv_tx());
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XPROPERTY_FEE_RECV_TX_USE_SEND_TX_TGAS);
        if (!value.empty()) {
            return base::xstring_utl::touint32(value);
        }
    }
    return 0;
}
uint64_t xcons_transaction_t::get_last_action_receipt_id() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_RECEIPT_ID);
        if (!value.empty()) {
            return base::xstring_utl::touint64(value);
        }
    }
    return 0;
}
uint64_t xcons_transaction_t::get_last_action_sender_confirmed_receipt_id() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_SENDER_CONFRIMED_RECEIPT_ID);
        if (!value.empty()) {
            return base::xstring_utl::touint64(value);
        }
    }
    return 0;
}
base::xtable_shortid_t xcons_transaction_t::get_last_action_self_tableid() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_RECEIPT_ID_SELF_TABLE_ID);
        if (!value.empty()) {
            return (base::xtable_shortid_t)base::xstring_utl::touint32(value);
        }
    }
    return 0;
}

base::xtable_shortid_t xcons_transaction_t::get_last_action_peer_tableid() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_RECEIPT_ID_PEER_TABLE_ID);
        if (!value.empty()) {
            return (base::xtable_shortid_t)base::xstring_utl::touint32(value);
        }
    }
    return 0;
}

std::string xcons_transaction_t::get_tx_hash() const {
    if (m_receipt != nullptr) {
        return m_receipt->get_tx_hash();
    } else if (get_transaction() != nullptr) {
        return get_transaction()->get_digest_str();
    } else {
        xassert(false);
        return {};
    }
}

uint256_t xcons_transaction_t::get_tx_hash_256() const {  // TODO(jimmy) always use string type hash
    if (get_transaction() != nullptr) {
        return get_transaction()->digest();
    } else if (m_receipt != nullptr) {
        return uint256_t((uint8_t*)m_receipt->get_tx_hash().c_str());
    } else {
        xassert(false);
        return {};
    }
}

std::string xcons_transaction_t::get_account_addr() const {
    if (get_transaction() != nullptr) {
        return is_recv_tx()? m_tx->get_target_addr() : m_tx->get_source_addr();
    } else {
        xassert(m_receipt != nullptr);
        xassert(!m_receipt->get_caller().empty());
        xassert(m_receipt->get_caller() != m_receipt->get_contract_address());
        return m_receipt->get_caller();
    }
}

base::xtable_index_t xcons_transaction_t::get_self_table_index() const {
    base::xtable_shortid_t tableid = get_self_tableid();
    return base::xtable_index_t(tableid);
}
base::xtable_index_t xcons_transaction_t::get_peer_table_index() const {
    base::xtable_shortid_t tableid = get_peer_tableid();
    return base::xtable_index_t(tableid);
}

base::xtable_shortid_t xcons_transaction_t::get_self_tableid() const {
    if (m_receipt != nullptr) {
        return get_last_action_peer_tableid();
    } else {
        xassert(m_tx != nullptr);
        xassert(is_send_tx() || is_self_tx());
        base::xvaccount_t _vaddr(get_source_addr());
        return _vaddr.get_short_table_id();
    }
}

base::xtable_shortid_t xcons_transaction_t::get_peer_tableid() const {
    if (m_receipt != nullptr) {
        return get_last_action_self_tableid();
    } else {
        xassert(m_tx != nullptr);
        xassert(is_send_tx() || is_self_tx());
        base::xvaccount_t _vaddr(get_target_addr());
        return _vaddr.get_short_table_id();
    }
}


}  // namespace data
}  // namespace top
