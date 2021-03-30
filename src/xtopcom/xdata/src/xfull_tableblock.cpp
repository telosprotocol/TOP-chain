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
REG_CLS(xfulltable_statistics_resource_t);
REG_CLS(xfulltable_binlog_resource_t);

int32_t xfulltable_statistics_resource_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream.write_compact_var(m_statistics_data);
    return CALC_LEN();
}

int32_t xfulltable_statistics_resource_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream.read_compact_var(m_statistics_data);
    return CALC_LEN();
}

int32_t xfulltable_binlog_resource_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    m_index_binlog->serialize_to(stream);
    return CALC_LEN();
}

int32_t xfulltable_binlog_resource_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    m_index_binlog = make_object_ptr<xtable_mbt_binlog_t>();
    m_index_binlog->serialize_from(stream);
    return CALC_LEN();
}

xfulltable_block_para_t::xfulltable_block_para_t(const xtable_mbt_ptr_t & last_state, const xtable_mbt_binlog_ptr_t & highqc_binlog) {
    m_state_binlog = highqc_binlog;
    m_new_full_state = xtable_mbt_t::build_new_tree(last_state, highqc_binlog);
}

xfulltable_input_entity_t::xfulltable_input_entity_t(const xtable_mbt_binlog_ptr_t & binlog) {
    m_binlog_hash = binlog->build_binlog_hash();
}

int32_t xfulltable_input_entity_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream.write_compact_var(m_binlog_hash);
    return CALC_LEN();
}
int32_t xfulltable_input_entity_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream.read_compact_var(m_binlog_hash);
    return CALC_LEN();
}

xfulltable_output_entity_t::xfulltable_output_entity_t(const std::string & tree_root) {
    m_tree_root = tree_root;
}

int32_t xfulltable_output_entity_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream.write_compact_var(m_tree_root);
    return CALC_LEN();
}
int32_t xfulltable_output_entity_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream.read_compact_var(m_tree_root);
    return CALC_LEN();
}

xblockbody_para_t xfull_tableblock_t::get_blockbody_from_para(const xfulltable_block_para_t & para) {
    xblockbody_para_t blockbody;

    const xtable_mbt_binlog_ptr_t & binlog = para.get_binlog();
    xobject_ptr_t<xfulltable_input_entity_t> input = make_object_ptr<xfulltable_input_entity_t>(binlog);
    xobject_ptr_t<xfulltable_output_entity_t> output = make_object_ptr<xfulltable_output_entity_t>(para.get_new_state_root());
    blockbody.add_input_entity(input);
    blockbody.add_output_entity(output);

    xfulltable_binlog_resource_ptr_t binlog_resource = make_object_ptr<xfulltable_binlog_resource_t>(binlog);
    std::string binlog_resource_str;
    binlog_resource->serialize_to_string(binlog_resource_str);
    blockbody.add_input_resource(xfulltable_binlog_resource_t::name(), binlog_resource_str);

    xfulltable_statistics_resource_ptr_t statistics_resource = make_object_ptr<xfulltable_statistics_resource_t>(para.get_block_statistics_data());
    std::string statistics_resource_str;
    statistics_resource->serialize_to_string(statistics_resource_str);
    blockbody.add_output_resource(xfulltable_statistics_resource_t::name(), statistics_resource_str);

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

bool xfull_tableblock_t::set_full_offstate(const xtable_mbt_ptr_t & offstate) {
    if (offstate == nullptr) {
        m_full_offstate = nullptr;
        return true;
    }

    auto mbt_root_hash = offstate->build_root_hash();
    auto root_hash = get_bucket_tree_root();
    if (root_hash == mbt_root_hash) {
        m_full_offstate = offstate;
        return true;
    } else {
        xwarn("xfull_tableblock_t::set_full_offstate root hash check fail");
        return false;
    }
}

bool xfull_tableblock_t::is_full_state_block() const {
    // full-table block need offstate
    return nullptr != m_full_offstate;
}

xfulltable_statistics_resource_ptr_t    xfull_tableblock_t::get_fulltable_statistics_resource() const {
    std::string fulltable_statistics_resource = get_output()->query_resource(xfulltable_statistics_resource_t::name());
    xassert(!fulltable_statistics_resource.empty());
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t*)fulltable_statistics_resource.data(), (uint32_t)fulltable_statistics_resource.size());
    xfulltable_statistics_resource_t* _resource_obj = dynamic_cast<xfulltable_statistics_resource_t*>(base::xdataunit_t::read_from(_stream));
    if (nullptr == _resource_obj) {
        xassert(_resource_obj != nullptr);
        return nullptr;
    }
    xfulltable_statistics_resource_ptr_t _resource_obj_ptr;
    _resource_obj_ptr.attach(_resource_obj);
    return _resource_obj_ptr;
}

xfulltable_binlog_resource_ptr_t        xfull_tableblock_t::get_fulltable_binlog_resource() const {
    std::string resource_str = get_input()->query_resource(xfulltable_binlog_resource_t::name());
    xassert(!resource_str.empty());
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t*)resource_str.data(), (uint32_t)resource_str.size());
    xfulltable_binlog_resource_t* _resource_obj = dynamic_cast<xfulltable_binlog_resource_t*>(base::xdataunit_t::read_from(_stream));
    if (nullptr == _resource_obj) {
        xassert(_resource_obj != nullptr);
        return nullptr;
    }
    xfulltable_binlog_resource_ptr_t _resource_obj_ptr;
    _resource_obj_ptr.attach(_resource_obj);
    return _resource_obj_ptr;
}

NS_END2
