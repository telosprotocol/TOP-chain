// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xbasic/xns_macro.h"
#include "xbasic/xdataobj_base.hpp"
#include "xbase/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xblock.h"
#include "xdata/xblockchain.h"

NS_BEG2(top, data)

class xemptyblock_t : public xblock_t {
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_empty_block };
    static base::xvblock_t * create_emptyblock(const std::string & account,
                                               uint64_t height,
                                               base::enum_xvblock_level level,
                                               const std::string & last_block_hash,
                                               const std::string & justify_block_hash,
                                               uint64_t viewid,
                                               uint64_t clock,
                                               const std::string & last_full_block_hash,
                                               uint64_t last_full_block_height,
                                               base::enum_xvblock_type blocktype);

public:
    static base::xvblock_t* create_genesis_emptyblock(const std::string & account, base::enum_xvblock_level level);
    static base::xvblock_t* create_genesis_emptyunit(const std::string & account);
    static base::xvblock_t* create_genesis_emptytable(const std::string & account);
    static base::xvblock_t* create_genesis_emptyroot(const std::string & account);

    static base::xvblock_t* create_next_emptyblock(base::xvblock_t* prev_block, base::enum_xvblock_type blocktype = base::enum_xvblock_type_general);
    static base::xvblock_t* create_next_emptyblock(xblockchain2_t* chain, base::enum_xvblock_type blocktype = base::enum_xvblock_type_general);
    static base::xvblock_t* create_emptyblock(const std::string & account,
                                              uint64_t height,
                                              base::enum_xvblock_level level,
                                              uint64_t viewid,
                                              uint64_t clock,
                                              base::enum_xvblock_type blocktype);

    xemptyblock_t();
    xemptyblock_t(base::xvheader_t & header, xblockcert_t & cert);
    virtual ~xemptyblock_t();
 private:
    xemptyblock_t(const xemptyblock_t &);
    xemptyblock_t & operator = (const xemptyblock_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;
};


NS_END2
