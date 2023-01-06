// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xbasic_size.hpp"
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

xcons_transaction_t::xcons_transaction_t(xtransaction_t* tx, const base::xtx_receipt_ptr_t & receipt) {
    tx->add_ref();
    m_tx.attach(tx);
    m_receipt = receipt;
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

void xcons_transaction_t::clear_modified_state() {
    m_execute_state.clear();
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

void xcons_transaction_t::set_not_need_confirm() {
    if (is_self_tx()) {
        return;
    }
    if (is_send_tx()) {
        if (data::is_sys_contract_address(common::xaccount_address_t{get_transaction()->get_source_addr()}) ||
            !data::is_sys_contract_address(common::xaccount_address_t{get_transaction()->get_target_addr()})) {
            xdbg("xcons_transaction_t::set_not_need_confirm tx:%s true", dump().c_str());
            m_execute_state.set_not_need_confirm(true);
        }
    } else {
        auto not_need_confirm = get_last_not_need_confirm();
        if (not_need_confirm) {
            xdbg("xcons_transaction_t::set_not_need_confirm tx:%s true", dump().c_str());
            m_execute_state.set_not_need_confirm(true);
        }
    }
}

void xcons_transaction_t::set_inner_table_flag() {
    if (is_send_tx()
        && (get_self_tableid() == get_peer_tableid())
        && (get_tx_type() == xtransaction_type_transfer || m_tx->is_evm_tx())) {// TODO(jimmy) only support transfer now
        xdbg("xcons_transaction_t::set_inner_table_flag tx:%s true", dump().c_str());
        m_execute_state.set_inner_table_flag(true);
    }
}
bool xcons_transaction_t::get_inner_table_flag() const {
    return m_execute_state.get_inner_table_flag();
}

void xcons_transaction_t::set_tx_subtype(enum_transaction_subtype _subtype) {
    m_subtype = _subtype;
}

void xcons_transaction_t::update_transation() {
    if (m_receipt == nullptr) {
        if (m_tx->get_source_addr() == m_tx->get_target_addr() || data::is_black_hole_address(common::xaccount_address_t{m_tx->get_target_addr()})) {
            set_tx_subtype(enum_transaction_subtype_self);
        } else if (m_tx->get_tx_version() == xtransaction_version_3 && m_tx->get_tx_type() == xtransaction_type_deploy_evm_contract) {
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

uint64_t xcons_transaction_t::get_dump_rsp_id() const {
    if (is_self_tx() || is_send_tx()) {
        return get_current_rsp_id();
    } else {
        return get_last_action_rsp_id();
    }
}

std::string xcons_transaction_t::dump(bool detail) const {
    xassert(m_receipt != nullptr || m_tx != nullptr);  // should not both null

    if (!m_dump_str.empty()) {
        return m_dump_str;
    }
    if (get_tx_hash().empty()) {
        return m_dump_str;
    }

    auto txhash = base::xstring_utl::to_hex(get_tx_hash());
    char local_param_buf[256];
    if (is_self_tx()) {
        xprintf(local_param_buf,
                sizeof(local_param_buf),
                "{%s:self,id={%d->%d},nonce:%lu,type:%d,addr:%s}",
                txhash.c_str(),
                get_self_tableid(),
                get_peer_tableid(),
                get_transaction()->get_tx_nonce(),
                get_transaction()->get_tx_type(),
                get_transaction()->get_source_addr().c_str());
    } else if (is_send_tx()) {
        xprintf(local_param_buf,
                sizeof(local_param_buf),
                "{%s:send,id={%d->%d},nonce:%lu,type:%d,addr:%s->%s}",
                txhash.c_str(),
                get_self_tableid(),
                get_peer_tableid(),
                get_transaction()->get_tx_nonce(),
                get_transaction()->get_tx_type(),
                get_transaction()->get_source_addr().c_str(),
                get_transaction()->get_target_addr().c_str());
    } else if (is_recv_tx()) {
        xprintf(local_param_buf,
                sizeof(local_param_buf),
                "{%s:recv,id={%d->%d:%lu,%lu},no_need_confirm:%d}",
                txhash.c_str(),
                get_self_tableid(),
                get_peer_tableid(),
                get_dump_receipt_id(),
                get_dump_rsp_id(),
                get_last_not_need_confirm());
    } else {
        xprintf(local_param_buf,
                sizeof(local_param_buf),
                "{%s:confirm,id={%d->%d:%lu,%lu}}",
                txhash.c_str(),
                get_self_tableid(),
                get_peer_tableid(),
                get_dump_receipt_id(),
                get_dump_rsp_id());
    }
    return local_param_buf;
}

uint64_t xcons_transaction_t::get_last_action_used_tgas() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XPROPERTY_FEE_TX_USED_TGAS);
        if (!value.empty()) {
            return base::xstring_utl::touint64(value);
        }
    }
    return 0;
}
uint64_t xcons_transaction_t::get_last_action_used_deposit() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XPROPERTY_FEE_TX_USED_DEPOSIT);
        if (!value.empty()) {
            return base::xstring_utl::touint64(value);
        }
    }
    return 0;
}

uint64_t xcons_transaction_t::get_last_action_send_tx_lock_tgas() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XPROPERTY_FEE_SEND_TX_LOCK_TGAS);
        if (!value.empty()) {
            return base::xstring_utl::touint64(value);
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

uint64_t xcons_transaction_t::get_last_action_recv_tx_use_send_tx_tgas() const {
    if (m_receipt != nullptr) {
        xassert(m_receipt->is_recv_tx());
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XPROPERTY_FEE_RECV_TX_USE_SEND_TX_TGAS);
        if (!value.empty()) {
            return base::xstring_utl::touint64(value);
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

data::xreceipt_data_t   xcons_transaction_t::get_last_action_receipt_data() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_RECEIPT_DATA);
        if (value.empty()) {
            return data::xreceipt_data_t{};
        }
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)value.data(), (int32_t)value.size());
        xreceipt_data_t data;
        data.serialize_from(stream);
        return data;
    }

    return data::xreceipt_data_t{};
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

uint64_t xcons_transaction_t::get_last_action_rsp_id() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_RSP_ID);
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

bool xcons_transaction_t::get_last_not_need_confirm() const {
    if (m_receipt != nullptr) {
        std::string value = m_receipt->get_tx_result_property(xtransaction_exec_state_t::XTX_FLAGS);
        if (!value.empty()) {
            auto flags = (base::xtable_shortid_t)base::xstring_utl::touint32(value);
            return flags & XTX_NOT_NEED_CONFIRM_FLAG_MASK;
        }
    }
    return false;
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

int32_t xcons_transaction_t::get_object_size() const {
#ifdef CACHE_SIZE_STATISTIC
    if (m_object_size != 0) {
        return m_object_size;
    }

    int32_t total_size = sizeof(*this);
    if (m_tx != nullptr) {
        total_size += m_tx->get_object_size();
    }

    if (m_receipt != nullptr) {
        total_size += m_receipt->get_object_size();
    }
    
    const auto & map_para = m_execute_state.get_map_para();
    xdbg("-----nathan test-----this:%p m_execute_state.get_map_para().size():%d,dump_execute_state:%s", this, m_execute_state.get_map_para().size(), dump_execute_state().c_str());
    for (auto & para : map_para) {
        total_size += get_size(para.first);
        total_size += get_size(para.second);
        xdbg("-----nathan test----- m_execute_state key:%d,value:%d", get_size(para.first), get_size(para.second));
        // total_size += 32; // alloc by each node in the map.
    }

    total_size += get_size(m_dump_str);
    xdbg("-----nathan test----- xcons_transaction_t:%d,m_dump_str:%d", sizeof(*this), get_size(m_dump_str));

    m_object_size = total_size;
    return total_size;
#else
    return 0;
#endif
}


}  // namespace data
}  // namespace top
