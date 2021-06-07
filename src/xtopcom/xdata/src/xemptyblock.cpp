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

REG_CLS(xemptyblock_t);

base::xvblock_t* xemptyblock_t::create_emptyblock(const std::string & account,
                                        uint64_t height,
                                        base::enum_xvblock_level level,
                                        const std::string & last_block_hash,
                                        const std::string & justify_block_hash,
                                        uint64_t viewid,
                                        uint64_t clock,
                                        const std::string & last_full_block_hash,
                                        uint64_t last_full_block_height,
                                        base::enum_xvblock_type blocktype) {
    xblock_para_t block_para;
    block_para.chainid     = xrootblock_t::get_rootblock_chainid();
    block_para.block_level = level;
    block_para.block_class = base::enum_xvblock_class_nil;
    block_para.block_type  = blocktype;
    block_para.account     = account;
    block_para.height      = height;
    block_para.last_block_hash = last_block_hash;
    block_para.justify_block_hash = justify_block_hash;
    block_para.last_full_block_hash = last_full_block_hash;
    block_para.last_full_block_height = last_full_block_height;

    base::xauto_ptr<base::xvheader_t> _blockheader = xblockheader_t::create_blockheader(block_para);
    base::xauto_ptr<xblockcert_t> _blockcert = xblockcert_t::create_blockcert(account, height, (base::enum_xconsensus_flag)0, viewid, clock);
    xemptyblock_t * fullunit = new xemptyblock_t(*_blockheader, *_blockcert);
    return fullunit;
}

base::xvblock_t* xemptyblock_t::create_genesis_emptyblock(const std::string & account, base::enum_xvblock_level level) {
    return create_emptyblock(account, 0, level, xrootblock_t::get_rootblock_hash(), std::string(), 0, 0, std::string(), 0, base::enum_xvblock_type_genesis);
}
base::xvblock_t* xemptyblock_t::create_genesis_emptyunit(const std::string & account) {
    return create_genesis_emptyblock(account, base::enum_xvblock_level_unit);
}
base::xvblock_t* xemptyblock_t::create_genesis_emptytable(const std::string & account) {
    return create_genesis_emptyblock(account, base::enum_xvblock_level_table);
}
base::xvblock_t* xemptyblock_t::create_genesis_emptyroot(const std::string & account) {
    return create_genesis_emptyblock(account, base::enum_xvblock_level_root);
}
base::xvblock_t* xemptyblock_t::create_next_emptyblock(base::xvblock_t* prev_block, base::enum_xvblock_type blocktype) {
    if (prev_block->is_genesis_block() || prev_block->get_header()->get_block_class() == base::enum_xvblock_class_full) {
        return create_emptyblock(prev_block->get_account(), prev_block->get_height() + 1,
            prev_block->get_header()->get_block_level(), prev_block->get_block_hash(), std::string(), prev_block->get_viewid() + 1, prev_block->get_clock() + 1,
            prev_block->get_block_hash(), prev_block->get_height(), blocktype);
    } else {
        return create_emptyblock(prev_block->get_account(), prev_block->get_height() + 1,
            prev_block->get_header()->get_block_level(), prev_block->get_block_hash(), std::string(), prev_block->get_viewid() + 1, prev_block->get_clock() + 1,
            prev_block->get_last_full_block_hash(), prev_block->get_last_full_block_height(), blocktype);
    }
}

base::xvblock_t* xemptyblock_t::create_emptyblock(const std::string & account, uint64_t height, base::enum_xvblock_level level, uint64_t viewid, uint64_t clock, base::enum_xvblock_type blocktype) {
    return create_emptyblock(account, height, level, xrootblock_t::get_rootblock_hash(), std::string(), viewid, clock, xrootblock_t::get_rootblock_hash(), clock - 1, blocktype);
}

xemptyblock_t::xemptyblock_t(base::xvheader_t & header, base::xvqcert_t & cert)
: xblock_t(header, cert, (enum_xdata_type)object_type_value) {

}

xemptyblock_t::xemptyblock_t()
: xblock_t((enum_xdata_type)object_type_value) {

}

xemptyblock_t::~xemptyblock_t() {

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
