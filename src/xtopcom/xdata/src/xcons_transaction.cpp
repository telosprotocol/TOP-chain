// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xcons_transaction.h"
#include "xdata/xdata_common.h"
#include "xdata/xgenesis_data.h"
#include "xmetrics/xmetrics.h"

namespace top { namespace data {

REG_CLS(xcons_transaction_t);

xcons_transaction_t::xcons_transaction_t() {
    XMETRICS_GAUGE(metrics::dataobject_cur_xbase_type_cons_transaction, 1);
}

xcons_transaction_t::xcons_transaction_t(xtransaction_t* tx) {
    m_tx = tx;
    m_tx->add_ref();
    xassert(!tx->digest().empty());
    XMETRICS_GAUGE(metrics::dataobject_cur_xbase_type_cons_transaction, 1);
    update_transation();
}

xcons_transaction_t::xcons_transaction_t(xtransaction_t* tx, const base::xtx_receipt_ptr_t & receipt) {
    xassert(!tx->digest().empty());
    xassert(tx->get_digest_str() == receipt->get_tx_hash());
    base::xstream_t stream(base::xcontext_t::instance());
    tx->serialize_to(stream);
    xtransaction_t* _tx = (xtransaction_t*)base::xdataunit_t::read_from(stream);  // TODO(jimmy) not copy
    m_tx.attach(_tx);
    xassert(receipt != nullptr);
    m_receipt = receipt;
    XMETRICS_GAUGE(metrics::dataobject_cur_xbase_type_cons_transaction, 1);
    update_transation();
}

xcons_transaction_t::~xcons_transaction_t() {
    XMETRICS_GAUGE(metrics::dataobject_cur_xbase_type_cons_transaction, -1);
}

const std::string & xcons_transaction_t::get_receipt_source_account()const {
    xassert(is_recv_tx() || is_confirm_tx());
    if (is_recv_tx()) {
        return get_source_addr();
    } else {
        return get_target_addr();
    }
}

const std::string & xcons_transaction_t::get_receipt_target_account()const {
    xassert(is_recv_tx() || is_confirm_tx());
    if (is_recv_tx()) {
        return get_target_addr();
    } else {
        return get_source_addr();
    }
}

void xcons_transaction_t::set_commit_prove_with_parent_cert(base::xvqcert_t* prove_cert) {
    m_receipt->set_commit_prove_with_parent_cert(prove_cert, get_unit_cert()->get_extend_data());
}

void xcons_transaction_t::set_commit_prove_with_self_cert(base::xvqcert_t* prove_cert) {
    m_receipt->set_commit_prove_with_self_cert(prove_cert);
}

bool xcons_transaction_t::is_commit_prove_cert_set() const {
    return m_receipt->is_commit_prove_cert_set();
}

bool xcons_transaction_t::get_tx_info_prove_cert_and_account(base::xvqcert_t* & cert, std::string & account) const {
    if (m_receipt == nullptr) {
        xerror("no receipt");
        return false;
    }
    if (m_receipt->get_tx_info_prove() == nullptr) {
        xerror("tx info prove null");
        return false;
    }

    account = get_receipt_source_account();
    cert = m_receipt->get_tx_info_prove()->get_prove_cert();
    return true;
}

bool xcons_transaction_t::get_commit_prove_cert_and_account(base::xvqcert_t* & cert, std::string & account) const {
    if (m_receipt == nullptr) {
        xerror("no receipt");
        return false;
    }
    if (m_receipt->get_commit_prove() == nullptr) {
        xerror("commit prove null");
        return false;
    }

    account = get_receipt_source_account();
    if (m_receipt->get_tx_info_prove()->get_prove_class() == base::xprove_cert_class_parent_cert) {
        // change to parent account
        account = account_address_to_block_address(common::xaccount_address_t(account));
    }
    cert = m_receipt->get_commit_prove()->get_prove_cert();
    return true;
}

void xcons_transaction_t::update_transation() {
    if (m_receipt == nullptr) {
        if (m_tx->get_source_addr() == m_tx->get_target_addr() || data::is_black_hole_address(common::xaccount_address_t{m_tx->get_target_addr()})) {
            m_tx->set_tx_subtype(enum_transaction_subtype_self);
        } else {
            m_tx->set_tx_subtype(enum_transaction_subtype_send);
        }
    } else {
        if (m_receipt->get_tx_subtype() == enum_transaction_subtype_send) {
            m_tx->set_tx_subtype(enum_transaction_subtype_recv);
        } else if (m_receipt->get_tx_subtype() == enum_transaction_subtype_recv) {
            m_tx->set_tx_subtype(enum_transaction_subtype_confirm);
        } else {
            xassert(0);
        }
    }
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
        xtransaction_t* _tx = (xtransaction_t*)base::xdataunit_t::read_from(stream);
        xassert(_tx != nullptr);
        m_tx.attach(_tx);
    }
    uint8_t has_receipt;
    stream >> has_receipt;
    if (has_receipt) {
        base::xtx_receipt_t* _receipt = (base::xtx_receipt_t*)base::xdataunit_t::read_from(stream);
        xassert(_receipt != nullptr);
        m_receipt.attach(_receipt);
    }

    update_transation();

    return CALC_LEN();
}

bool xcons_transaction_t::verify_cons_transaction() {
    bool ret = true;
    if (m_tx->get_tx_subtype() == data::enum_transaction_subtype_recv || m_tx->get_tx_subtype() == data::enum_transaction_subtype_confirm) {
        if (m_receipt == nullptr) {
            xerror("xcons_transaction_t::verify_cons_transaction should has receipt");
            return false;
        }
        if (m_tx->get_digest_str() != m_receipt->get_tx_hash()) {
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
    std::stringstream ss;
    ss << "{";
    ss << base::xvtxkey_t::transaction_hash_subtype_to_string(get_transaction()->get_digest_str(), get_tx_subtype());
    ss << ",id={" << base::xvaccount_t(get_transaction()->get_source_addr()).get_short_table_id();
    ss << "->" << base::xvaccount_t(get_transaction()->get_target_addr()).get_short_table_id();
    ss << ":" << get_dump_receipt_id();
    ss << "}";
    if (is_self_tx() || is_send_tx()) {
        ss << ",nonce:" << get_transaction()->get_tx_nonce();
    }
    if (detail) {
        if (is_self_tx() || is_send_tx()) {
            ss << ",fire:" << get_transaction()->get_fire_timestamp();
            ss << ",duration:" << (uint32_t)get_transaction()->get_expire_duration();
        }
        ss << ",addr:" << get_transaction()->get_source_addr();
        if (get_transaction()->get_source_addr() != get_transaction()->get_target_addr()) {
            ss << "->" << get_transaction()->get_target_addr();
        }
        // if (m_receipt != nullptr) {
        //     ss << ",txout:" << m_receipt->get_tx_info()->get_tx_exec_state().dump();
        // }
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
base::xtable_shortid_t xcons_transaction_t::get_last_action_receipt_id_tableid() const {
    if (m_receipt != nullptr) {
        // TODO(jimmy) need this property ??
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_RECEIPT_ID_TABLE_ID);
        if (!value.empty()) {
            return (base::xtable_shortid_t)base::xstring_utl::touint32(value);
        }
    }
    return 0;
}

}  // namespace data
}  // namespace top
