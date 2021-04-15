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
#include "xdata/xaccount_mstate.h"
#include "xdata/xblockchain.h"

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
    explicit xfullunit_output_t(const xaccount_mstate2 & state, const std::map<std::string, std::string> & propertys);
 protected:
    virtual ~xfullunit_output_t() {}

    int32_t do_write(base::xstream_t &stream) override;
    int32_t do_read(base::xstream_t &stream) override;
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}//virtual key-value for entity
 public:
    const xaccount_mstate2 &    get_mstate() const {return m_account_state;}
    const xaccount_mstate2*     get_mstate_ptr() const {return &m_account_state;}
    const std::map<std::string, std::string> * get_propertys() const {return &m_account_propertys;}

 private:
    xaccount_mstate2                    m_account_state{};
    std::map<std::string, std::string>  m_account_propertys;
};

struct xfullunit_block_para_t {
    xaccount_mstate2                    m_account_state;
    std::map<std::string, std::string>  m_account_propertys;
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
    static base::xvblock_t* create_next_fullunit(xblockchain2_t* chain);
    static base::xvblock_t* create_next_fullunit(const xinput_ptr_t & input, const xoutput_ptr_t & output, base::xvblock_t* prev_block);
 public:
    xfullunit_block_t(base::xvheader_t & header, xblockcert_t & cert);
    xfullunit_block_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output);
    // xfullunit_block_t(base::xvheader_t & header, xblockcert_t & cert, const std::string & input, const std::string & output);
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
    const std::map<std::string, std::string> & get_property_hash_map() const override {return get_fullunit_output()->get_mstate().get_propertys_hash();}
    std::string get_property_hash(const std::string & prop_name) const override {return get_fullunit_output()->get_mstate().get_property_hash(prop_name);}
    const xnative_property_t & get_native_property() const override {return get_fullunit_output()->get_mstate().get_native_property();}
    const xaccount_mstate2*    get_fullunit_mstate() const override { return get_fullunit_output()->get_mstate_ptr();}
    const std::map<std::string, std::string> * get_fullunit_propertys() const override {return get_fullunit_output()->get_propertys();}
};


NS_END2
