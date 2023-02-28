// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvledger/xdataobj_base.hpp"
#include "xdata/xblock.h"
#include "xvledger/xvblockbuild.h"

NS_BEG2(top, data)

class xtable_block_t : public xblock_t {
 protected:
#if defined(XCXX17)
     static inline constexpr int object_type_value = static_cast<int>(enum_xdata_type_max) - static_cast<int>(xdata_type_table_block);
#else
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_table_block };
#endif
 public:
    xtable_block_t();
    xtable_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
    virtual ~xtable_block_t();
 private:
    xtable_block_t(const xtable_block_t &);
    xtable_block_t & operator = (const xtable_block_t &);
    void parse_to_json_v1(Json::Value & root);
    void parse_to_json_v2(Json::Value & root);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;
    virtual void parse_to_json(Json::Value & root, const std::string & rpc_version) override;
    virtual std::vector<base::xvsubblock_index_t> get_subblocks_index() const override;

 public:  // implement block common api
    virtual bool    extract_sub_blocks(std::vector<xobject_ptr_t<base::xvblock_t>> & sub_blocks) override;
    bool extract_sub_txs(std::vector<base::xvtxindex_ptr> & sub_txs) override;
 private:
    std::vector<xvheader_ptr_t> get_sub_block_headers() const;
};


NS_END2
