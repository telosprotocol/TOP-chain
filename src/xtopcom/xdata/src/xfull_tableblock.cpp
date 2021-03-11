// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xbase/xvledger.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xrootblock.h"

NS_BEG2(top, data)

REG_CLS(xfull_tableblock_t);
REG_CLS(xfulltable_input_entity_t);
REG_CLS(xfulltable_output_entity_t);

xfulltable_block_para_t::xfulltable_block_para_t(const xtable_mbt_ptr_t & last_state, const xtable_mbt_binlog_ptr_t & highqc_binlog) {
    m_state_binlog = highqc_binlog;
    m_new_full_state = xtable_mbt_t::build_new_tree(last_state, highqc_binlog);
}

xfulltable_block_para_t::xfulltable_block_para_t(const xtable_mbt_ptr_t & last_state, const xtable_mbt_binlog_ptr_t & commit_binlog, const std::vector<xblock_ptr_t> & uncommit_blocks) {
    m_state_binlog = commit_binlog;
    for (auto & block : uncommit_blocks) {
        std::map<std::string, xaccount_index_t> changed_indexs = block->get_units_index();
        if (!changed_indexs.empty()) {
            m_state_binlog->set_accounts_index(changed_indexs);
        }
    }
    m_new_full_state = xtable_mbt_t::build_new_tree(last_state, m_state_binlog);
}

xfulltable_input_entity_t::xfulltable_input_entity_t(const std::map<std::string, xaccount_index_t> & accounts_info) {
    m_accounts_index_info = accounts_info;
}

int32_t xfulltable_input_entity_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    uint32_t count = m_accounts_index_info.size();
    stream << count;
    for (auto & v : m_accounts_index_info) {
        stream << v.first;
        v.second.do_write(stream);
    }
    return CALC_LEN();
}
int32_t xfulltable_input_entity_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    uint32_t count;
    stream >> count;
    for (uint32_t i = 0; i < count; i++) {
        std::string account;
        stream >> account;
        xaccount_index_t info;
        info.do_read(stream);
        m_accounts_index_info[account] = info;
    }
    return CALC_LEN();
}

xfulltable_output_entity_t::xfulltable_output_entity_t(const std::string & tree_root) {
    m_tree_root = tree_root;
}

int32_t xfulltable_output_entity_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_tree_root;
    return CALC_LEN();
}
int32_t xfulltable_output_entity_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_tree_root;
    return CALC_LEN();
}

xblockbody_para_t xfull_tableblock_t::get_blockbody_from_para(const xfulltable_block_para_t & para) {
    xblockbody_para_t blockbody;
    xobject_ptr_t<xfulltable_input_entity_t> input = make_object_ptr<xfulltable_input_entity_t>(para.get_accounts_index());
    xobject_ptr_t<xfulltable_output_entity_t> output = make_object_ptr<xfulltable_output_entity_t>(para.get_new_state_root());
    blockbody.add_input_entity(input);
    blockbody.add_output_entity(output);
    blockbody.create_default_input_output();
    return blockbody;
}

base::xvblock_t* xfull_tableblock_t::create_next_block(const xfulltable_block_para_t & para, base::xvblock_t* prev_block) {
    std::string last_full_block_hash;
    uint64_t    last_full_block_height;
    if (prev_block->is_genesis_block() || prev_block->get_header()->get_block_class() == base::enum_xvblock_class_full) {
        last_full_block_hash = prev_block->get_block_hash();
        last_full_block_height = prev_block->get_height();
    } else {
        last_full_block_hash = prev_block->get_last_full_block_hash();
        last_full_block_height = prev_block->get_last_full_block_height();
    }

    xblock_para_t block_para;
    block_para.chainid     = xrootblock_t::get_rootblock_chainid();
    block_para.block_level = base::enum_xvblock_level_table;
    block_para.block_class = base::enum_xvblock_class_full;
    block_para.block_type  = base::enum_xvblock_type_general;
    block_para.account     = prev_block->get_account();
    block_para.height      = prev_block->get_height() + 1;
    block_para.last_block_hash = prev_block->get_block_hash();
    block_para.justify_block_hash = std::string();  // not set here
    block_para.last_full_block_hash = last_full_block_hash;
    block_para.last_full_block_height = last_full_block_height;

    xblockbody_para_t blockbody = xfull_tableblock_t::get_blockbody_from_para(para);
    base::xauto_ptr<base::xvheader_t> _blockheader = xblockheader_t::create_blockheader(block_para);
    base::xauto_ptr<xblockcert_t> _blockcert = xblockcert_t::create_blockcert(prev_block->get_account(), prev_block->get_height() + 1, (base::enum_xconsensus_flag)0, prev_block->get_viewid() + 1, prev_block->get_clock() + 1);
    xfull_tableblock_t* block = new xfull_tableblock_t(*_blockheader, *_blockcert, blockbody.get_input(), blockbody.get_output());
    // xassert(!block->get_cert()->get_output_root_hash().empty());
    return block;
}

xfull_tableblock_t::xfull_tableblock_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {
}

xfull_tableblock_t::xfull_tableblock_t()
: xblock_t((enum_xdata_type)object_type_value) {

}
xfull_tableblock_t::~xfull_tableblock_t() {

}

const std::map<std::string, xaccount_index_t> & xfull_tableblock_t::get_accounts_index() const {
    xfulltable_input_entity_t* entity = dynamic_cast<xfulltable_input_entity_t*>(get_input()->get_entitys()[0]);
    xassert(entity != nullptr);
    return entity->get_accounts_index_info();
}
const std::string &     xfull_tableblock_t::get_bucket_tree_root() const {
    xfulltable_output_entity_t* entity = dynamic_cast<xfulltable_output_entity_t*>(get_output()->get_entitys()[0]);
    xassert(entity != nullptr);
    return entity->get_tree_root();
}

base::xobject_t * xfull_tableblock_t::create_object(int type) {
    (void)type;
    return new xfull_tableblock_t;
}

void * xfull_tableblock_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvblock_t::query_interface(_enum_xobject_type_);
}

xblockchain_ptr_t xfull_tableblock_t::execute_block(const xblockchain_ptr_t & last_state) {
    return last_state;
}

NS_END2
