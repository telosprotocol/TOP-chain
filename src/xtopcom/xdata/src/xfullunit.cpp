// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xfullunit.h"

NS_BEG2(top, data)

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
    xJson::Value j_fu;
    j_fu["latest_full_unit_number"] = static_cast<unsigned int>(get_height());
    j_fu["latest_full_unit_hash"] = to_hex_str(get_block_hash());
    root["fullunit"] = j_fu;

    if (rpc_version == RPC_VERSION_V1) {
        return;
    }

    auto tx_vec = get_txkeys();
    for (auto & tx : tx_vec) {
        xJson::Value jv;
        jv["tx_consensus_phase"] = tx.get_tx_subtype_str();
        jv["tx_hash"] = "0x" + tx.get_tx_hex_hash();
        root["fullunit"]["txs"].append(jv);
    }
}

NS_END2
