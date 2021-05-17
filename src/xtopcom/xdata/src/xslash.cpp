// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <map>
#include <vector>

#include "xbasic/xversion.h"
#include "xcommon/xnode_id.h"
#include "xdata/xdata_common.h"
#include "xdata/xslash.h"
#include "xbase/xmem.h"
#include "xbase/xns_macro.h"
#include "xcommon/xnode_type.h"

NS_BEG2(top, data)

std::int32_t
xnode_vote_percent_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << block_count;
    stream << subset_count;
    return CALC_LEN();
}

std::int32_t
xnode_vote_percent_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> block_count;
    stream >> subset_count;
    return CALC_LEN();
}

std::int32_t
xunqualified_node_info_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    MAP_OBJECT_SERIALIZE2(stream, auditor_info);
    MAP_OBJECT_SERIALIZE2(stream, validator_info);
    return CALC_LEN();
}

std::int32_t
xunqualified_node_info_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    MAP_OBJECT_DESERIALZE2(stream, auditor_info);
    MAP_OBJECT_DESERIALZE2(stream, validator_info);
    return CALC_LEN();
}

std::int32_t
xunqualified_round_info_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    VECTOR_SERIALIZE_SIMPLE(stream, rounds);
    MAP_OBJECT_SERIALIZE2(stream, round_info);
    return CALC_LEN();
}

std::int32_t
xunqualified_round_info_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    VECTOR_DESERIALZE_SIMPLE(stream, rounds, uint64_t);
    MAP_OBJECT_DESERIALZE2(stream, round_info);
    return CALC_LEN();
}

std::int32_t
xunqualified_filter_info_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << node_id;
    stream << node_type;
    stream << vote_percent;
    return CALC_LEN();
}

std::int32_t
xunqualified_filter_info_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> node_id;
    stream >> node_type;
    stream >> vote_percent;
    return CALC_LEN();
}

std::int32_t
xaction_node_info_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << node_id;
    stream << node_type;
    stream << action_type;
    return CALC_LEN();
}

std::int32_t
xaction_node_info_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> node_id;
    stream >> node_type;
    stream >> action_type;
    return CALC_LEN();
}

NS_END2
