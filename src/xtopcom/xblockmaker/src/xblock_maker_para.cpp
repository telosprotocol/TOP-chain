// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "xblockmaker/xblock_maker_para.h"

NS_BEG2(top, blockmaker)

int32_t xunit_proposal_input_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << m_account;
    stream << m_last_block_height;
    stream << m_last_block_hash;
    SERIALIZE_CONTAINER(m_input_txs) {
        item->serialize_to(stream);
    }
    return CALC_LEN();
}
int32_t xunit_proposal_input_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_account;
    stream >> m_last_block_height;
    stream >> m_last_block_hash;
    DESERIALIZE_CONTAINER(m_input_txs) {
        xcons_transaction_ptr_t tx = make_object_ptr<xcons_transaction_t>();
        tx->serialize_from(stream);
        m_input_txs.push_back(tx);
    }
    return CALC_LEN();
}

std::string xunit_proposal_input_t::dump() const {
    std::stringstream ss;
    ss << "{";
    ss << m_account;
    ss << ",last=" << m_last_block_height << base::xstring_utl::to_hex(m_last_block_hash);
    for (auto & v : m_input_txs) {
        ss << v->dump(false);
    }
    ss << "}";
    return ss.str();
}

int32_t xtableblock_proposal_input_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    SERIALIZE_CONTAINER(m_unit_inputs) {
        item.serialize_to(stream);
    }
    return CALC_LEN();
}
int32_t xtableblock_proposal_input_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    DESERIALIZE_CONTAINER(m_unit_inputs) {
        xunit_proposal_input_t input;
        input.serialize_from(stream);
        m_unit_inputs.push_back(input);
    }
    return CALC_LEN();
}

size_t xtableblock_proposal_input_t::get_total_txs() const {
    size_t tx_count = 0;
    for (auto & v : m_unit_inputs) {
        tx_count += v.get_input_txs().size();
    }
    return tx_count;
}

std::string xtableblock_proposal_input_t::to_string() const {
    base::xstream_t stream(base::xcontext_t::instance());
    serialize_to(stream);
    return std::string((const char *)stream.data(), stream.size());
}

std::string xtableblock_proposal_input_t::dump() const {
    std::stringstream ss;
    ss << "tproposal:" << m_unit_inputs.size() << "," << get_total_txs();
    ss << "{";
    for (auto & v : m_unit_inputs) {
        ss << v.dump();
    }
    ss << "}";
    return ss.str();
}


xtable_proposal_input_t::xtable_proposal_input_t() {

}
xtable_proposal_input_t::xtable_proposal_input_t(const std::vector<xcons_transaction_ptr_t> & input_txs)
: m_input_txs(input_txs) {

}

int32_t xtable_proposal_input_t::do_write(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    uint32_t count = m_input_txs.size();
    stream.write_compact_var(count);
    for (uint32_t i = 0; i < count; i++) {
        m_input_txs[i]->serialize_to(stream);
    }

    uint32_t prove_count = m_receiptid_state_proves.size();
    if (prove_count > 0) {
        stream.write_compact_var(prove_count);
        for (uint32_t i = 0; i < prove_count; i++) {
            m_receiptid_state_proves[i]->serialize_to(stream);
        }
    }

    return (stream.size() - begin_size);
}

int32_t xtable_proposal_input_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    uint32_t count;
    stream.read_compact_var(count);
    for (uint32_t i = 0; i < count; i++) {
        xcons_transaction_ptr_t tx = make_object_ptr<xcons_transaction_t>();
        tx->serialize_from(stream);
        m_input_txs.push_back(tx);
    }

    if (stream.size() > 0) {
        uint32_t prove_count = 0;
        int32_t ret = stream.read_compact_var(prove_count);
        // for compatibility
        if (ret > 0) {
            for (uint32_t i = 0; i < prove_count; i++) {
                base::xvproperty_prove_ptr_t receiptid_state_prove = make_object_ptr<base::xvproperty_prove_t>();
                receiptid_state_prove->serialize_from(stream);
                m_receiptid_state_proves.push_back(receiptid_state_prove);
            }
        }
    }

    return (begin_size - stream.size());
}

void xtable_proposal_input_t::set_input_tx(const xcons_transaction_ptr_t & tx) {
    m_input_txs.push_back(tx);
}

void xtable_proposal_input_t::set_receiptid_state_prove(const base::xvproperty_prove_ptr_t & receiptid_state_prove) {
    m_receiptid_state_proves.push_back(receiptid_state_prove);
}

void xtable_proposal_input_t::set_other_account(const std::string & account) {
    m_other_accounts.push_back(account);
}

bool xtable_proposal_input_t::delete_fail_tx(const xcons_transaction_ptr_t & input_tx) {
    // should only delete self and send tx
    if (input_tx->is_recv_tx() || input_tx->is_confirm_tx()) {
        xassert(false);
        return false;
    }
    for (auto iter = m_input_txs.begin(); iter != m_input_txs.end(); iter++) {
        auto & tx = *iter;
        if (tx->get_tx_hash_256() == input_tx->get_tx_hash_256() && tx->get_tx_subtype() == input_tx->get_tx_subtype()) {
            m_input_txs.erase(iter);
            return true;
        }
    }
    xassert(false);
    return false;
}

NS_END2
