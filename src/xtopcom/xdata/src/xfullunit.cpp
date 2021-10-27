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
}

NS_END2
