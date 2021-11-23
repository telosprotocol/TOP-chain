// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xfullunit.h"

NS_BEG2(top, data)

void xfullunit_block_para_t::set_input_txs(const std::vector<xcons_transaction_ptr_t> & input_txs) {
    xassert(m_raw_txs.empty());
    m_raw_txs = input_txs;
    m_succ_txs.insert(m_succ_txs.end(), m_raw_txs.begin(), m_raw_txs.end());
}

void xfullunit_block_para_t::set_unchange_txs(const std::vector<xcons_transaction_ptr_t> & unchange_txs) {
    xassert(m_unchange_txs.empty());
    m_unchange_txs = unchange_txs;
    m_succ_txs.insert(m_succ_txs.end(), m_unchange_txs.begin(), m_unchange_txs.end());
}

std::vector<xcons_transaction_ptr_t> const & xfullunit_block_para_t::get_succ_txs() const {
    return m_succ_txs;
}

xfullunit_block_t::xfullunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_fullunit, 1);
}

xfullunit_block_t::xfullunit_block_t()
: xblock_t((enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_fullunit, 1);
}

xfullunit_block_t::~xfullunit_block_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_fullunit, -1);
}

base::xobject_t * xfullunit_block_t::create_object(int type) {
    (void)type;
    return new xfullunit_block_t;
}

void * xfullunit_block_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvblock_t::query_interface(_enum_xobject_type_);
}

void xfullunit_block_t::parse_to_json(xJson::Value & root, const std::string & rpc_version) {
    if (rpc_version == RPC_VERSION_V1) {
        return;
    }

    xJson::Value j_fu;
    j_fu["latest_full_unit_number"] = static_cast<unsigned int>(get_height());
    j_fu["latest_full_unit_hash"] = to_hex_str(get_block_hash());
    root["fullunit"] = j_fu;

    auto header = get_header();
    auto extra_str = header->get_extra_data();
    base::xvheader_extra he;
    he.serialize_from_string(extra_str);

    auto txs_str = he.get_val(base::HEADER_KEY_TXS);
    if (txs_str.empty()) {
        xdbg("empty header, account:%s", get_account().c_str());
        return;
    }

    base::xvtxkey_vec_t txs;
    txs.serialize_from_string(txs_str);
    auto & tx_vec = txs.get_txkeys();
    for (auto & tx : tx_vec) {
        xJson::Value jv;
        jv["tx_consensus_phase"] = tx.get_tx_subtype_str();
        jv["tx_hash"] = "0x" + tx.get_tx_hex_hash();
        root["fullunit"]["txs"].append(jv);
    }
}

NS_END2
