// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xdata/xlightunit_info.h"
#include "xdata/xdata_common.h"

namespace top { namespace data {

REG_CLS(xlightunit_input_entity_t);
REG_CLS(xlightunit_output_entity_t);

void xtransaction_exec_state_t::set_tx_exec_status(enum_xunit_tx_exec_status value) {
    if (value != enum_xunit_tx_exec_status_success) {  // default success status not need set
        set_value(XTX_STATE_TX_EXEC_STATUS, (uint32_t)value);
    }
}
enum_xunit_tx_exec_status xtransaction_exec_state_t::get_tx_exec_status() const {
    enum_xunit_tx_exec_status status = static_cast<enum_xunit_tx_exec_status>(get_value_uint32(XTX_STATE_TX_EXEC_STATUS));
    if (status == 0) {
        status = enum_xunit_tx_exec_status_success;  // default is success;
    }
    return status;
}

xinput_tx_propertys_t::xinput_tx_propertys_t(bool _is_contract_create, enum_xunit_tx_exec_status last_action_status, uint64_t last_tx_clock) {
    set_is_contract_create(_is_contract_create);
    set_last_tx_exec_status(last_action_status);
    set_tx_clock(last_tx_clock);
}

void xinput_tx_propertys_t::set_last_tx_exec_status(enum_xunit_tx_exec_status value) {
    if (value != enum_xunit_tx_exec_status_success) {  // default success status not need set
        set_value(PARA_LAST_TX_EXEC_STATUS, (uint32_t)value);
    }
}
enum_xunit_tx_exec_status xinput_tx_propertys_t::get_last_tx_exec_status() const {
    enum_xunit_tx_exec_status status = static_cast<enum_xunit_tx_exec_status>(get_value_uint32(PARA_LAST_TX_EXEC_STATUS));
    if (status == 0) {
        status = enum_xunit_tx_exec_status_success;  // default is success;
    }
    return status;
}


xlightunit_input_entity_t::xlightunit_input_entity_t() {
}

xlightunit_input_entity_t::xlightunit_input_entity_t(enum_transaction_subtype type,
                                                    xtransaction_t * tx,
                                                    bool _is_contract_create,
                                                    enum_xunit_tx_exec_status last_action_status,
                                                    uint64_t last_tx_clock)
    : m_tx_key(tx->get_digest_str(), type), m_inputtx_props(_is_contract_create, last_action_status, last_tx_clock) {
    xassert(get_tx_subtype() == type);
    xassert(is_contract_create() == _is_contract_create);
    xassert(get_tx_clock() == last_tx_clock);
    xassert(get_last_action_exec_status() == last_action_status);

    if (type == enum_transaction_subtype_self
        || type == enum_transaction_subtype_send
        || type == enum_transaction_subtype_recv) {
        tx->add_ref();
        m_raw_tx.attach(tx);
    }
}

xlightunit_input_entity_t::~xlightunit_input_entity_t() {
}

int32_t xlightunit_input_entity_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    m_tx_key.do_write(stream);
    m_inputtx_props.do_write(stream);
    SERIALIZE_PTR(m_raw_tx) {
        m_raw_tx->serialize_to(stream);
    }
    return CALC_LEN();
}
int32_t xlightunit_input_entity_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    m_tx_key.do_read(stream);
    m_inputtx_props.do_read(stream);
    DESERIALIZE_PTR(m_raw_tx) {
        m_raw_tx = make_object_ptr<xtransaction_t>();
        m_raw_tx->serialize_from(stream);
    }
    return CALC_LEN();
}
std::string xlightunit_input_entity_t::dump() const {
    std::stringstream ss;
    ss << ""            << get_tx_dump_key();
    ss << "inputtx="    << m_inputtx_props.dump();
    return ss.str();
}

xlightunit_output_entity_t::xlightunit_output_entity_t() {
}
xlightunit_output_entity_t::xlightunit_output_entity_t(enum_transaction_subtype type, xtransaction_t * tx, const xtransaction_exec_state_t & txstate)
: m_tx_key(tx->get_digest_str(), type) {
    m_exec_state = txstate;
}

xlightunit_output_entity_t::~xlightunit_output_entity_t() {
}

int32_t xlightunit_output_entity_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    m_tx_key.do_write(stream);
    m_exec_state.do_write(stream);
    return CALC_LEN();
}
int32_t xlightunit_output_entity_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    m_tx_key.do_read(stream);
    m_exec_state.do_read(stream);
    return CALC_LEN();
}

const std::string xlightunit_output_entity_t::get_merkle_leaf() {
    base::xstream_t stream(base::xcontext_t::instance());
    do_write(stream);
    return std::string((const char*)stream.data(), stream.size());
}

std::string xlightunit_output_entity_t::dump() const {
    std::stringstream ss;
    ss << "lightunit_output{";
    ss << ""                    << m_tx_key.get_tx_dump_key();
    ss << ","         << m_exec_state.dump();
    return ss.str();
}

uint64_t xlightunit_tx_info_t::get_last_trans_nonce() const {
    if (is_self_tx() || is_send_tx()) {
        if (m_raw_tx != nullptr) {
            return m_raw_tx->get_last_nonce();
        }
    }
    return 0;
}

std::string xlightunit_tx_info_t::dump() const {
    std::stringstream ss;
    ss << ""        << m_tx_key.get_tx_dump_key();
    ss << ",execstate="  << m_exec_state.dump();
    return ss.str();
}


}  // namespace data
}  // namespace top
