// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xfullunit.h"
#include "xdata/xunit_bstate.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvstate.h"

NS_BEG2(top, data)

void xfullunit_block_para_t::set_input_txs(const std::vector<xcons_transaction_ptr_t> & input_txs) {
    xassert(m_raw_txs.empty());
    m_raw_txs = input_txs;
}

xfullunit_block_t::xfullunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE(metrics::dataobject_block_fullunit, 1);
}

xfullunit_block_t::xfullunit_block_t()
: xblock_t((enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE(metrics::dataobject_block_fullunit, 1);
}

xfullunit_block_t::~xfullunit_block_t() {
    XMETRICS_GAUGE(metrics::dataobject_block_fullunit, -1);
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
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(this, metrics::statestore_access_from_rpc_set_fullunit);
    xassert(bstate != nullptr);
    data::xunit_bstate_t unitstate(bstate.get());
    xJson::Value j_fu;
    j_fu["latest_full_unit_number"] = static_cast<unsigned int>(get_height());
    j_fu["latest_full_unit_hash"] = to_hex_str(get_block_hash());
    j_fu["latest_send_trans_number"] = static_cast<unsigned int>(unitstate.account_send_trans_number());
    j_fu["latest_send_trans_hash"] = to_hex_str(unitstate.account_send_trans_hash());
    j_fu["latest_recv_trans_number"] = static_cast<unsigned int>(unitstate.account_recv_trans_number());
    j_fu["account_balance"] = static_cast<unsigned int>(unitstate.balance());
    j_fu["burned_amount_change"] = static_cast<unsigned int>(unitstate.burn_balance());
    j_fu["account_create_time"] = static_cast<unsigned int>(unitstate.get_account_create_time());
    root["fullunit"] = j_fu;

    // need input
    base::xvaccount_t _vaccount(get_account());
    base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, this, metrics::blockstore_access_from_rpc_get_block_set_table);

    auto header = get_header();
    auto extra_str = header->get_extra_data();
    base::xvheader_extra he;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)extra_str.data(), extra_str.size());
    he.do_read(stream);
    auto txs_str = he.get_val("txs");
    if (txs_str.empty()) {
        xdbg("empty header, account:%s", get_account().c_str());
        return;
    }

    base::xstream_t stream1(base::xcontext_t::instance(), (uint8_t *)txs_str.data(), txs_str.size());
    base::xvtxkey_vec_t txs;
    auto size = txs.do_read(stream1);
    auto & tx_vec = txs.get_txkeys();
    for (auto & tx : tx_vec) {
        xJson::Value jv;
        jv["tx_consensus_phase"] = tx.get_tx_subtype_str();
        jv["tx_hash"] = "0x" + tx.get_tx_hex_hash();
        root["fullunit"]["txs"].append(jv);
    }
}

NS_END2
