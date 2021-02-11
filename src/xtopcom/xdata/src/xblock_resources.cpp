// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xbase/xvledger.h"
#include "xbasic/xversion.h"
#include "xdata/xblock_resources.h"
#include "xdata/xdata_common.h"

NS_BEG2(top, data)

REG_CLS(xresource_wholeblock_t);
REG_CLS(xresource_origintx_t);
REG_CLS(xresource_unit_input_t);
REG_CLS(xresource_unit_output_t);

xresource_wholeblock_t::xresource_wholeblock_t(base::xvblock_t* block) {
    xassert(block->get_block_hash().empty());
    block->serialize_to_string(m_block_object);
    xassert(block->get_block_hash().empty());
    m_input_resource = block->get_input()->get_resources_data();
    m_output_resource = block->get_output()->get_resources_data();
}

int32_t xresource_wholeblock_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_block_object;
    stream << m_input_resource;
    stream << m_output_resource;
    return CALC_LEN();
}

int32_t xresource_wholeblock_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_block_object;
    stream >> m_input_resource;
    stream >> m_output_resource;
    return CALC_LEN();
}

base::xauto_ptr<base::xvblock_t> xresource_wholeblock_t::create_whole_block() const {
    base::xauto_ptr<base::xvblock_t> new_block = base::xvblockstore_t::create_block_object(m_block_object);
    xassert(new_block != nullptr);
    if (new_block != nullptr && new_block->get_header()->get_block_class() != base::enum_xvblock_class_nil) {
        if (false == new_block->set_input_resources(m_input_resource)) {
            xerror("xresource_wholeblock_t::create_whole_block set_input_resources fail");
            return nullptr;
        }
        if (false == new_block->set_output_resources(m_output_resource)) {
            xerror("xresource_wholeblock_t::create_whole_block set_output_resources fail");
            return nullptr;
        }
    }
    return new_block;
}


xresource_origintx_t::xresource_origintx_t(xtransaction_t* tx) {
    tx->serialize_to_string(m_origin_tx);
}

int32_t xresource_origintx_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_origin_tx;
    return CALC_LEN();
}

int32_t xresource_origintx_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_origin_tx;
    return CALC_LEN();
}

xtransaction_ptr_t xresource_origintx_t::create_origin_tx() const {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    tx->serialize_from_string(m_origin_tx);
    return tx;
}



xresource_unit_input_t::xresource_unit_input_t(base::xvblock_t* block) {
    block->get_header()->serialize_to_string(m_unit_header);
    block->get_input()->serialize_to_string(m_unit_input);
    m_unit_input_resources = block->get_input()->get_resources_data();
}

int32_t xresource_unit_input_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_unit_header;
    stream << m_unit_input;
    stream << m_unit_input_resources;
    return CALC_LEN();
}

int32_t xresource_unit_input_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_unit_header;
    stream >> m_unit_input;
    stream >> m_unit_input_resources;
    return CALC_LEN();
}

xresource_unit_output_t::xresource_unit_output_t(base::xvblock_t* block) {
    block->get_output()->serialize_to_string(m_unit_output);
    m_unit_output_resources = block->get_output()->get_resources_data();
    m_unit_justify_hash = block->get_cert()->get_justify_cert_hash();
}

int32_t xresource_unit_output_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_unit_output;
    stream << m_unit_output_resources;
    stream << m_unit_justify_hash;
    return CALC_LEN();
}

int32_t xresource_unit_output_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_unit_output;
    stream >> m_unit_output_resources;
    stream >> m_unit_justify_hash;
    return CALC_LEN();
}

NS_END2
