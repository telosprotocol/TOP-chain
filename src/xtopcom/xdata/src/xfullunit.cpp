// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xfullunit.h"
#include "xbasic/xdataobj_base.hpp"

NS_BEG2(top, data)

REG_CLS(xfullunit_input_t);
REG_CLS(xfullunit_output_t);
REG_CLS(xfullunit_block_t);

xfullunit_input_t::xfullunit_input_t(uint64_t first_unit_height, const std::string & first_unit_hash)
: m_first_unit_height(first_unit_height), m_first_unit_hash(first_unit_hash) {

}

int32_t xfullunit_input_t::do_write(base::xstream_t &stream) {
    KEEP_SIZE();
    stream << m_first_unit_height;
    stream << m_first_unit_hash;
    return CALC_LEN();
}
int32_t xfullunit_input_t::do_read(base::xstream_t &stream) {
    KEEP_SIZE();
    stream >> m_first_unit_height;
    stream >> m_first_unit_hash;
    return CALC_LEN();
}

xfullunit_output_t::xfullunit_output_t(const xaccount_mstate2 & state, const std::map<std::string, std::string> & propertys)
: m_account_state(state), m_account_propertys(propertys) {

}

int32_t xfullunit_output_t::do_write(base::xstream_t &stream) {
    KEEP_SIZE();
    m_account_state.serialize_to(stream);
    SERIALIZE_FIELD_BT(m_account_propertys);
    return CALC_LEN();
}
int32_t xfullunit_output_t::do_read(base::xstream_t &stream) {
    KEEP_SIZE();
    m_account_state.serialize_from(stream);
    DESERIALIZE_FIELD_BT(m_account_propertys);
    return CALC_LEN();
}

// std::string xfullunit_output_t::body_dump() const {
//     std::stringstream ss;
//     ss << "{";
//     ss << m_account_state.dump();
//     ss << ",";
//     if (!m_account_propertys.empty()) {
//         ss << "prop_count=" << m_account_propertys.size() << ":";
//         for (auto & v : m_account_propertys) {
//             ss << v.first << " ";
//             ss << v.second.size() << ";";
//         }
//     }
//     ss << "}";
//     return ss.str();
// }

xblockbody_para_t xfullunit_block_t::get_blockbody_from_para(const xfullunit_block_para_t & para) {
    xblockbody_para_t blockbody;
    xobject_ptr_t<xfullunit_input_t> input = make_object_ptr<xfullunit_input_t>(para.m_first_unit_height, para.m_first_unit_hash);
    xobject_ptr_t<xfullunit_output_t> output = make_object_ptr<xfullunit_output_t>(para.m_account_state, para.m_account_propertys);
    blockbody.add_input_entity(input);
    blockbody.add_output_entity(output);
    blockbody.create_default_input_output();
    return blockbody;
}

base::xvblock_t* xfullunit_block_t::create_fullunit(const std::string & account,
                                            uint64_t height,
                                            std::string last_block_hash,
                                            std::string justify_block_hash,
                                            uint64_t viewid,
                                            uint64_t clock,
                                            const std::string & last_full_block_hash,
                                            uint64_t last_full_block_height,
                                            const xfullunit_block_para_t & para) {
    xblockbody_para_t blockbody = xfullunit_block_t::get_blockbody_from_para(para);
    base::xauto_ptr<base::xvheader_t> _blockheader = xblockheader_t::create_fullunit_header(account, height, last_block_hash, last_full_block_hash,
            justify_block_hash, last_full_block_height);
    base::xauto_ptr<xblockcert_t> _blockcert = xblockcert_t::create_blockcert(account, height, base::enum_xconsensus_flag_extend_cert, viewid, clock);
    xfullunit_block_t* fullunit = new xfullunit_block_t(*_blockheader, *_blockcert, blockbody.get_input(), blockbody.get_output());
    return fullunit;
}

base::xvblock_t* xfullunit_block_t::create_next_fullunit(const xinput_ptr_t & input, const xoutput_ptr_t & output, base::xvblock_t* prev_block) {
    base::xauto_ptr<base::xvheader_t> _blockheader = xblockheader_t::create_fullunit_header(prev_block->get_account(), prev_block->get_height() + 1,
                                                            prev_block->get_block_hash(), prev_block->get_last_full_block_hash(), std::string(),
                                                            prev_block->get_last_full_block_height());
    base::xauto_ptr<xblockcert_t> _blockcert = xblockcert_t::create_blockcert(prev_block->get_account(), _blockheader->get_height(),
        base::enum_xconsensus_flag_extend_cert, prev_block->get_viewid() + 1, prev_block->get_clock() + 1);
    xfullunit_block_t* fullunit = new xfullunit_block_t(*_blockheader, *_blockcert, input, output);
    return fullunit;
}

base::xvblock_t* xfullunit_block_t::create_next_fullunit(const xfullunit_block_para_t & para, base::xvblock_t* prev_block) {
    if (prev_block->is_genesis_block() || prev_block->get_block_class() == base::enum_xvblock_class_full) {
        return create_fullunit(prev_block->get_account(), prev_block->get_height() + 1,
            prev_block->get_block_hash(), std::string(), prev_block->get_viewid() + 1, prev_block->get_clock() + 1,
            prev_block->get_block_hash(), prev_block->get_height(), para);
    } else {
        return create_fullunit(prev_block->get_account(), prev_block->get_height() + 1,
            prev_block->get_block_hash(), std::string(), prev_block->get_viewid() + 1, prev_block->get_clock() + 1,
            prev_block->get_last_full_block_hash(), prev_block->get_last_full_block_height(), para);
    }
}
base::xvblock_t* xfullunit_block_t::create_next_fullunit(xblockchain2_t* chain) {
    xfullunit_block_para_t para;
    para.m_account_state = chain->get_account_mstate();
    para.m_first_unit_hash = chain->get_last_full_unit_hash();
    para.m_first_unit_height = chain->get_last_full_unit_height();
    return create_fullunit(chain->get_account(), chain->get_chain_height() + 1,
        chain->get_last_block_hash(), std::string(), 1, 1,
        chain->get_last_full_unit_hash(), chain->get_last_full_unit_height(), para);
}
xfullunit_block_t::xfullunit_block_t(base::xvheader_t & header, xblockcert_t & cert)
: xblock_t(header, cert, (enum_xdata_type)object_type_value) {

}
xfullunit_block_t::xfullunit_block_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {

}
// xfullunit_block_t::xfullunit_block_t(base::xvheader_t & header, xblockcert_t & cert, const std::string & input, const std::string & output)
// : xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {

// }
xfullunit_block_t::xfullunit_block_t()
: xblock_t((enum_xdata_type)object_type_value) {

}

xfullunit_block_t::~xfullunit_block_t() {

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


NS_END2
