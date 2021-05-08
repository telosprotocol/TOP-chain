// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbasic/xutility.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xrootblock.h"

NS_BEG2(top, data)

REG_CLS(xfull_tableblock_t);
REG_CLS(xfulltable_output_entity_t);

xfulltable_block_para_t::xfulltable_block_para_t(const xtablestate_ptr_t & last_state, const xstatistics_data_t & statistics_data) {
    m_tablestate = last_state;
    m_block_statistics_data = statistics_data;
}

xfulltable_output_entity_t::xfulltable_output_entity_t(const std::string & offdata_root) {
    set_offdata_root(offdata_root);
}

int32_t xfulltable_output_entity_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream.write_compact_map(m_paras);
    return CALC_LEN();
}
int32_t xfulltable_output_entity_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream.read_compact_map(m_paras);
    return CALC_LEN();
}

void xfulltable_output_entity_t::set_offdata_root(const std::string & root) {
    m_paras[PARA_OFFDATA_ROOT] = root;
}
std::string xfulltable_output_entity_t::get_offdata_root() const {
    auto iter = m_paras.find(PARA_OFFDATA_ROOT);
    if (iter != m_paras.end()) {
        return iter->second;
    }
    return {};
}

xblockbody_para_t xfull_tableblock_t::get_blockbody_from_para(const xfulltable_block_para_t & para) {
    xblockbody_para_t blockbody;

    xobject_ptr_t<xdummy_entity_t> input = make_object_ptr<xdummy_entity_t>();
    blockbody.add_input_entity(input);

    const xtablestate_ptr_t & latest_state = para.get_tablestate();

    // TODO(jimmy)
    // std::string binlog_str = latest_state->serialize_to_binlog_data_string();
    // xassert(!binlog_str.empty());
    // blockbody.add_input_resource(RESOURCE_ACCOUNT_INDEX_BINLOG, binlog_str);

    // const xreceiptid_pairs_ptr_t & receiptid_binlog = latest_offstate->get_receiptid_state()->get_binlog();
    // std::string receiptid_binlog_str;
    // receiptid_binlog->serialize_to_string(receiptid_binlog_str);
    // blockbody.add_input_resource(RESOURCE_RECEIPTID_PAIRS_BINLOG, receiptid_binlog_str);

    const xstatistics_data_t & statistics_data = para.get_block_statistics_data();
    auto const & serialized_data = statistics_data.serialize_based_on<base::xstream_t>();
    blockbody.add_output_resource(RESOURCE_NODE_SIGN_STATISTICS, {std::begin(serialized_data), std::end(serialized_data) });

    latest_state->merge_new_full();
    std::string offdata_root = latest_state->build_root_hash();
    xobject_ptr_t<xfulltable_output_entity_t> output = make_object_ptr<xfulltable_output_entity_t>(offdata_root);
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

std::string     xfull_tableblock_t::get_offdata_hash() const {
    xfulltable_output_entity_t* entity = dynamic_cast<xfulltable_output_entity_t*>(get_output()->get_entitys()[0]);
    xassert(entity != nullptr);
    return entity->get_offdata_root();
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

xstatistics_data_t xfull_tableblock_t::get_table_statistics() const {
    std::string resource_str = get_output()->query_resource(RESOURCE_NODE_SIGN_STATISTICS);
    xassert(!resource_str.empty());
    xstatistics_data_t statistics_data;
    statistics_data.deserialize_based_on<base::xstream_t>({ std::begin(resource_str), std::end(resource_str) });
    return statistics_data;
}

NS_END2
