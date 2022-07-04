// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtop_relay_block.h"
#include <string>
#include "xbase/xutl.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"
#include "xdata/xlightunit.h"
#include "xdata/xrootblock.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblockbuild.h"
#include "xdata/xblockaction.h"
#include "xdata/xblocktool.h"

NS_BEG2(top, data)

xtop_relay_block_t::xtop_relay_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {
//    set_block_flag(base::enum_xvblock_flag_authenticated);
//    set_block_flag(base::enum_xvblock_flag_committed);
}

xtop_relay_block_t::xtop_relay_block_t()
: xblock_t((enum_xdata_type)object_type_value) {
//    set_block_flag(base::enum_xvblock_flag_authenticated);
//    set_block_flag(base::enum_xvblock_flag_committed);
}

xtop_relay_block_t::~xtop_relay_block_t() {
}

base::xobject_t * xtop_relay_block_t::create_object(int type) {
    (void)type;
    return new xtop_relay_block_t;
}

void * xtop_relay_block_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvblock_t::query_interface(_enum_xobject_type_);
}



NS_END2
