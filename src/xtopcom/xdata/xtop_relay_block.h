// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xvblock.h"
#include "xvledger/xreceiptid.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xblock/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xblock/xlightunit.h"
#include "xblock/xfullunit.h"
#include "xvledger/xvblockbuild.h"

NS_BEG2(top, data)

class xtop_relay_block_t : public xblock_t {
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_relay_block };
 public:
    xtop_relay_block_t();
    xtop_relay_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
    virtual ~xtop_relay_block_t();
 private:
    xtop_relay_block_t(const xtop_relay_block_t &);
    xtop_relay_block_t & operator = (const xtop_relay_block_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;

 private:
//    std::string     m_block_hash;

 public:  // implement block common api
//    virtual  const  std::string& get_block_hash()  const {return m_block_hash;}
//    void set_block_hash(const std::string& block_hash) {m_block_hash = block_hash;}
};


NS_END2
