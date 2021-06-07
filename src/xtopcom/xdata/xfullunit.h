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

NS_BEG2(top, data)

class xfullunit_input_t : public xventity_face_t<xfullunit_input_t, xdata_type_fullunit_input_entity> {
 public:
    xfullunit_input_t() = default;
    xfullunit_input_t(uint64_t first_unit_height, const std::string & first_unit_hash);
 protected:
    virtual ~xfullunit_input_t() {}
    int32_t do_write(base::xstream_t &stream) override;
    int32_t do_read(base::xstream_t &stream) override;
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}//virtual key-value for entity
 public:
    uint64_t    get_first_unit_height() const {return m_first_unit_height;}
    std::string get_first_unit_hash() const {return m_first_unit_hash;}

 private:
    uint64_t        m_first_unit_height;
    std::string     m_first_unit_hash;
};

class xfullunit_output_t : public xventity_face_t<xfullunit_output_t, xdata_type_fullunit_output_entity> {
 public:
    xfullunit_output_t() = default;
    explicit xfullunit_output_t(const std::string & property_snapshot);
 protected:
    virtual ~xfullunit_output_t() {}

    int32_t do_write(base::xstream_t &stream) override;
    int32_t do_read(base::xstream_t &stream) override;
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}//virtual key-value for entity
 public:
    const std::string &         get_property_snapshot() const {return m_property_snapshot;}

 private:
    std::string                         m_property_snapshot;
};

struct xfullunit_block_para_t {
    std::string                         m_property_snapshot;
    uint64_t                            m_first_unit_height{0};
    std::string                         m_first_unit_hash;
};
class xfullunit_block_t : public xblock_t {
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_fullunit_block };
    static base::xvblock_t* create_fullunit(const std::string & account,
                                        uint64_t height,
                                        std::string last_block_hash,
                                        std::string justify_block_hash,
                                        uint64_t viewid,
                                        uint64_t clock,
                                        const std::string & last_full_block_hash,
                                        uint64_t last_full_block_height,
                                        const xfullunit_block_para_t & para);
    static xblockbody_para_t get_blockbody_from_para(const xfullunit_block_para_t & para);
 public:
    static base::xvblock_t* create_next_fullunit(const xfullunit_block_para_t & para, base::xvblock_t* prev_block);
    static base::xvblock_t* create_next_fullunit(const xinput_ptr_t & input, const xoutput_ptr_t & output, base::xvblock_t* prev_block);
 public:
    xfullunit_block_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output);
    xfullunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
 protected:
    virtual ~xfullunit_block_t();
 private:
    xfullunit_block_t();
    xfullunit_block_t(const xfullunit_block_t &);
    xfullunit_block_t & operator = (const xfullunit_block_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;
 protected:
    xfullunit_output_t*    get_fullunit_output() const {return (xfullunit_output_t*)(get_output()->get_entitys()[0]);}

 public:
    std::string     get_property_binlog() const override {return get_fullunit_output()->get_property_snapshot();}
};


NS_END2
