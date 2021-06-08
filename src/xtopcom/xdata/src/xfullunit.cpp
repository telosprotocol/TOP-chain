// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xfullunit.h"

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

xfullunit_output_t::xfullunit_output_t(const std::string & property_snapshot)
: m_property_snapshot(property_snapshot) {

}

int32_t xfullunit_output_t::do_write(base::xstream_t &stream) {
    KEEP_SIZE();
    stream << m_property_snapshot;
    return CALC_LEN();
}
int32_t xfullunit_output_t::do_read(base::xstream_t &stream) {
    KEEP_SIZE();
    stream >> m_property_snapshot;
    return CALC_LEN();
}

// std::string xfullunit_output_t::body_dump() const {
//     std::stringstream ss;
//     ss << "{";
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

xfullunit_block_t::xfullunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {

}

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
