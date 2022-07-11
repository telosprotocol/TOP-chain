// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xblock/xblock.h"
#include "xblock/xblocktool.h"
#include "xdata/xcons_transaction.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, blockmaker)

xblock_maker_t::xblock_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources)
    : base::xvaccount_t(account), m_resources(resources) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xblock_maker_t, 1);
}
xblock_maker_t::~xblock_maker_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xblock_maker_t, -1);
}
void xblock_builder_face_t::alloc_tx_receiptid(const std::vector<xcons_transaction_ptr_t> & input_txs, const base::xreceiptid_state_ptr_t & receiptid_state) {
    for (auto & tx : input_txs) {
        data::xblocktool_t::alloc_transaction_receiptid(tx, receiptid_state);
    }
}
NS_END2
