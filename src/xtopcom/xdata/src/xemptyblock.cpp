// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xrootblock.h"

NS_BEG2(top, data)

xemptyblock_t::xemptyblock_t(base::xvheader_t & header, base::xvqcert_t & cert)
: xblock_t(header, cert, (enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_empty, 1);
}

xemptyblock_t::xemptyblock_t()
: xblock_t((enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_empty, 1);
}

xemptyblock_t::~xemptyblock_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_empty, -1);
}

base::xobject_t * xemptyblock_t::create_object(int type) {
    (void)type;
    return new xemptyblock_t;
}

void * xemptyblock_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvblock_t::query_interface(_enum_xobject_type_);
}
NS_END2
