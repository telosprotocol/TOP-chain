// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <set>
#include "xbasic/xns_macro.h"
#include "xbasic/xobject_ptr.h"
#include "xbasic/xdataobj_base.hpp"
#include "xdata/xblock_paras.h"
#include "xdata/xblock.h"
#include "xdata/xtableindex.h"
#include "xdata/xblockchain.h"

NS_BEG2(top, data)

class xfulltable_statistics_resource_t : public xbase_dataunit_t<xfulltable_statistics_resource_t, xdata_type_fulltable_statistics_resource>{
 public:
    static  const std::string   name() { return std::string("o0");}  // common output resource version#0
    xfulltable_statistics_resource_t() = default;
    explicit xfulltable_statistics_resource_t(const std::string & statistics_data)
    : m_statistics_data(statistics_data) {}

 protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    const std::string & get_statistics_data() const {return m_statistics_data;}

 private:
    std::string    m_statistics_data;
};
using xfulltable_statistics_resource_ptr_t = xobject_ptr_t<xfulltable_statistics_resource_t>;

class xfulltable_binlog_resource_t : public xbase_dataunit_t<xfulltable_binlog_resource_t, xdata_type_fulltable_binlog_resource>{
 public:
    static  const std::string   name() { return std::string("i0");}  // common input resource version#0
    xfulltable_binlog_resource_t() = default;
    explicit xfulltable_binlog_resource_t(const xtable_mbt_binlog_ptr_t & binlog)
    : m_index_binlog(binlog) {}

 protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    const xtable_mbt_binlog_ptr_t & get_binlog() const {return m_index_binlog;}

 private:
    xtable_mbt_binlog_ptr_t    m_index_binlog{nullptr};
};
using xfulltable_binlog_resource_ptr_t = xobject_ptr_t<xfulltable_binlog_resource_t>;


class xfulltable_block_para_t {
 public:
    xfulltable_block_para_t(const xtable_mbt_ptr_t & last_state, const xtable_mbt_binlog_ptr_t & highqc_binlog);
    ~xfulltable_block_para_t() = default;

    void                        set_block_statistics_data(const std::string & _data) {m_block_statistics_data = _data;}
    const std::string &         get_block_statistics_data() const {return m_block_statistics_data;}
    const xtable_mbt_binlog_ptr_t &     get_binlog() const {return m_state_binlog;}
    const std::string &         get_new_state_root() const {return m_new_full_state->get_root_hash();}
    const xtable_mbt_ptr_t &    get_new_state() const {return m_new_full_state;}

 private:
    xtable_mbt_binlog_ptr_t                     m_state_binlog{nullptr};  // the binlog of latest light-tableblock from last full-tableblock
    xtable_mbt_ptr_t                            m_new_full_state{nullptr};
    std::string                                 m_block_statistics_data;
};

class xfulltable_input_entity_t final : public xventity_face_t<xfulltable_input_entity_t, xdata_type_fulltable_input_entity> {
 public:
    xfulltable_input_entity_t() = default;
    explicit xfulltable_input_entity_t(const xtable_mbt_binlog_ptr_t & highqc_binlog);
 protected:
    ~xfulltable_input_entity_t() = default;
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;
 private:
    xfulltable_input_entity_t & operator = (const xfulltable_input_entity_t & other);
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}
 public:
    const std::string &     get_binlog_hash() const {return m_binlog_hash;}

 private:
    std::string     m_binlog_hash;
};

class xfulltable_output_entity_t final : public xventity_face_t<xfulltable_output_entity_t, xdata_type_fulltable_output_entity> {
 public:
    xfulltable_output_entity_t() = default;
    explicit xfulltable_output_entity_t(const std::string & tree_root);
 protected:
    ~xfulltable_output_entity_t() = default;
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;
 private:
    xfulltable_output_entity_t & operator = (const xfulltable_output_entity_t & other);
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}
 public:
    const std::string &     get_tree_root() const {return m_tree_root;}

 private:
    std::string     m_tree_root;  // the total bucket tree root
};

// tableindex block chain
// input: an array of newest units
// output: the root of bucket merkle tree of all unit accounts index
class xfull_tableblock_t : public xblock_t {
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
    const std::string &                     get_bucket_tree_root() const;
    xfulltable_statistics_resource_ptr_t    get_fulltable_statistics_resource() const;
    xfulltable_binlog_resource_ptr_t        get_fulltable_binlog_resource() const;

 public:  // override base block api
    bool                set_full_offstate(const xtable_mbt_ptr_t & offstate) override;
    xtable_mbt_ptr_t    get_full_offstate() const override {return m_full_offstate;}
    bool                is_full_state_block() const override;

 private:
    xtable_mbt_ptr_t                                m_full_offstate{nullptr};  // full block offstate
};

NS_END2
