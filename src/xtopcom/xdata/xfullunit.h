// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xvledger/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xblock.h"
#include "xdata/xlightunit.h"

NS_BEG2(top, data)

class xfullunit_block_para_t : public xlightunit_block_para_t {
public:
    uint64_t                            m_first_unit_height{0};
    std::string                         m_first_unit_hash;
};
class xfullunit_block_t : public xblock_t {
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_fullunit_block };
 public:
    xfullunit_block_t();
    xfullunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
 protected:
    virtual ~xfullunit_block_t();
 private:
    xfullunit_block_t(const xfullunit_block_t &);
    xfullunit_block_t & operator = (const xfullunit_block_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;
    virtual void parse_to_json(xJson::Value & root, const std::string & rpc_version) override;
};


NS_END2
