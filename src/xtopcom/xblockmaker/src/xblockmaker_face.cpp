// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
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

NS_END2
