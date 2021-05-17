// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <set>
#include "xbase/xobject_ptr.h"
#include "xvledger/xdataobj_base.hpp"
#include "xdata/xblock_paras.h"
#include "xdata/xblock.h"
#include "xvledger/xaccountindex.h"
#include "xdata/xblockchain.h"
#include "xdata/xtablestate.h"
#include "xdata/xblock_statistics_data.h"

NS_BEG2(top, data)

class xfulltable_block_para_t {
 public:
    xfulltable_block_para_t(const xtablestate_ptr_t & last_state, const xstatistics_data_t & statistics_data);
    ~xfulltable_block_para_t() = default;

    const xstatistics_data_t &  get_block_statistics_data() const {return m_block_statistics_data;}
    const xtablestate_ptr_t &   get_tablestate() const {return m_tablestate;}

 private:
    xtablestate_ptr_t       m_tablestate{nullptr};  // the binlog of latest light-tableblock from last full-tableblock
    xstatistics_data_t      m_block_statistics_data;
};

class xfulltable_output_entity_t final : public xventity_face_t<xfulltable_output_entity_t, xdata_type_fulltable_output_entity> {
 protected:
    static XINLINE_CONSTEXPR char const * PARA_OFFDATA_ROOT           = "0";
 public:
    xfulltable_output_entity_t() = default;
    explicit xfulltable_output_entity_t(const std::string & offdata_root);
 protected:
    ~xfulltable_output_entity_t() = default;
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;
 private:
    xfulltable_output_entity_t & operator = (const xfulltable_output_entity_t & other);
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}
 public:
    void            set_offdata_root(const std::string & root);
    std::string     get_offdata_root() const;
 private:
    std::map<std::string, std::string>  m_paras;
};

// tableindex block chain
// input: an array of newest units
// output: the root of bucket merkle tree of all unit accounts index
class xfull_tableblock_t : public xblock_t {
 protected:
    static XINLINE_CONSTEXPR char const * RESOURCE_ACCOUNT_INDEX_BINLOG     = "0";
    static XINLINE_CONSTEXPR char const * RESOURCE_RECEIPTID_PAIRS_BINLOG   = "1";
    static XINLINE_CONSTEXPR char const * RESOURCE_NODE_SIGN_STATISTICS     = "2";

 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_fulltable_block };
    static xblockbody_para_t get_blockbody_from_para(const xfulltable_block_para_t & para);
 public:
    static base::xvblock_t* create_next_block(const xfulltable_block_para_t & para, base::xvblock_t* prev_block);

 public:
    xfull_tableblock_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output);
 protected:
    virtual ~xfull_tableblock_t();
 private:
    xfull_tableblock_t();
    xfull_tableblock_t(const xfull_tableblock_t &);
    xfull_tableblock_t & operator = (const xfull_tableblock_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;

 public:
    xstatistics_data_t get_table_statistics() const;

 public:  // override base block api
    std::string get_offdata_hash() const override;
};

NS_END2
