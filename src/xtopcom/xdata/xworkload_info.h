// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>

#include "xbasic/xdataobj_base.hpp"
#include "xbase/xobject_ptr.h"
#include "xbasic/xserializable_based_on.h"
#include "xbasic/xversion.h"
#include "xdata/xdata_common.h"
#include "xdata/xtransaction.h"

namespace top { namespace data {

class xvalidator_workload_info_t: public xserializable_based_on<void> {
public:
    std::map<std::string, uint32_t> m_leader_count;

    std::int32_t do_write(base::xstream_t & stream) const override {
        KEEP_SIZE();
        MAP_SERIALIZE_SIMPLE(stream, m_leader_count);
        return CALC_LEN();

    }
    std::int32_t do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        MAP_DESERIALIZE_SIMPLE(stream, m_leader_count);
        return CALC_LEN();
    }
};

class xauditor_workload_info_t: public xserializable_based_on<void> {
public:
    std::map<std::string, uint32_t> m_leader_count;

    std::int32_t do_write(base::xstream_t & stream) const override {
        KEEP_SIZE();
        MAP_SERIALIZE_SIMPLE(stream, m_leader_count);
        return CALC_LEN();
    }
    std::int32_t do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        MAP_DESERIALIZE_SIMPLE(stream, m_leader_count);
        return CALC_LEN();
    }
};

}  // namespace data
}  // namespace top
