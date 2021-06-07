// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xvledger/xdataobj_base.hpp"
#include "xbase/xobject_ptr.h"
#include "xmetrics/xmetrics.h"
#include "xbase/xdata.h"
#include "xvledger/xvblock.h"
#include "xutility/xmerkle.hpp"
#include "xdata/xdata_common.h"

NS_BEG2(top, data)

XINLINE_CONSTEXPR char const * XRESOURCE_BINLOG_HASH_KEY                = "bh";  //binlog hash

template <typename T, int type_value>
class xventity_face_t : public base::xventity_t {
 protected:
    enum { object_type_value = enum_xdata_type_max - type_value };
 public:
    xventity_face_t()
    : base::xventity_t((enum_xdata_type)object_type_value) {
        XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
    }
 protected:
    virtual ~xventity_face_t() {
        XMETRICS_XBASE_DATA_CATEGORY_DELETE(object_type_value);
        // xdbg("xventity_face_t::~xventity_face_t this=%p", this);
    }
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type) {
        (void)type;
        return new T;
    }
    void *query_interface(const int32_t _enum_xobject_type_) override {
        if (object_type_value == _enum_xobject_type_)
            return this;
        return base::xdataunit_t::query_interface(_enum_xobject_type_);
    }

 public:
    std::string get_binary_string() const {
        base::xstream_t stream1(base::xcontext_t::instance());
        const_cast<xventity_face_t*>(this)->serialize_to(stream1);
        std::string binary_bin = std::string((char*)stream1.data(), stream1.size());
        return binary_bin;
    }
};

class xdummy_entity_t : public xventity_face_t<xdummy_entity_t, xdata_type_dummy_entity> {
 public:
    xdummy_entity_t() = default;
    explicit xdummy_entity_t(const std::string& dummy)
    : m_dummy(dummy) {}
 protected:
    virtual ~xdummy_entity_t() {}

    int32_t do_write(base::xstream_t &stream) override {
        const int32_t begin_size = stream.size();
        stream << m_dummy;
        return (stream.size() - begin_size);
    }
    int32_t do_read(base::xstream_t &stream) override {
        const int32_t begin_size = stream.size();
        stream >> m_dummy;
        return (begin_size - stream.size());
    }
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}//virtual key-value for entity
 private:
    std::string m_dummy;
};

class xinput_t final: public base::xvinput_t {
 protected:
    enum { object_type_value = enum_xdata_type_max - xdata_type_input };
 public:
    xinput_t();
    xinput_t(const std::vector<base::xventity_t*> & entitys, const std::string & resource_data);
    xinput_t(const std::vector<base::xventity_t*> & entitys, base::xstrmap_t & resource_obj);
 protected:
    ~xinput_t();
 private:
    xinput_t(const xinput_t &);
    xinput_t & operator = (const xinput_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type) {
        (void)type;
        return new xinput_t;
    }
 public:  // implement virtual interface
    virtual const std::string           get_root_hash() const override;

    // static std::string                  get_key(const std::string & _base_path) {return _base_path + base::xvinput_t::name();}

 public:
    std::string                         get_binary_string() const;
    bool                                calc_merkle_path(const std::string & leaf, xmerkle_path_256_t& hash_path) const;
    virtual std::string                 body_dump() const {return "empty";}

 private:
    std::vector<std::string>            get_merkle_leafs() const;
};

class xoutput_t final: public base::xvoutput_t {
 protected:
    enum { object_type_value = enum_xdata_type_max - xdata_type_output };
 public:
    xoutput_t();
    xoutput_t(const std::vector<base::xventity_t*> & entitys, const std::string & resource_data);
    xoutput_t(const std::vector<base::xventity_t*> & entitys, base::xstrmap_t & resource_obj);
 protected:
    ~xoutput_t();
 private:
    xoutput_t(const xoutput_t &);
    xoutput_t & operator = (const xoutput_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type) {
        (void)type;
        return new xoutput_t;
    }
 public:
    virtual const std::string           get_root_hash() const override;

    // static std::string                  get_key(const std::string & _base_path) {return _base_path + base::xvoutput_t::name();}

    std::string                         get_binary_string() const;
    bool                                calc_merkle_path(const std::string & leaf, xmerkle_path_256_t& hash_path) const;
    virtual std::string                 body_dump() const {return "empty";}
    virtual const std::string           get_binlog_hash() override;
 private:
    std::vector<std::string>            get_merkle_leafs() const;
};

using xentity_ptr_t = xobject_ptr_t<base::xventity_t>;
using xinput_ptr_t = xobject_ptr_t<xinput_t>;
using xoutput_ptr_t = xobject_ptr_t<xoutput_t>;



class xblock_resource_t {
 public:
    xblock_resource_t() {
        m_resource = make_object_ptr<base::xstrmap_t>();
    }
    void add_resource(const std::string & key, const std::string & value) {
        m_resource->set(key, value);
    }
    const xstrmap_ptr_t & get_resource() const {return m_resource;}

 private:
    xstrmap_ptr_t    m_resource{nullptr};
};


// unify all blockbody para
class xblockbody_para_t {
 public:
    xblockbody_para_t() = default;
    ~xblockbody_para_t() {
        for (auto & v : input_entitys) {
            v->release_ref();
        }
        input_entitys.clear();
        for (auto & v : output_entitys) {
            v->release_ref();
        }
        output_entitys.clear();
    }
    void create_default_input_output();
    bool add_input_entity(const xentity_ptr_t & entity) {
        base::xventity_t* _entity = entity.get();
        _entity->add_ref();
        input_entitys.push_back(_entity);
        return true;
    }
    bool add_input_resource(const std::string & key, const std::string & value) {
        input_resource.add_resource(key, value);
        return true;
    }
    bool add_output_entity(const xentity_ptr_t & entity) {
        base::xventity_t* _entity = entity.get();
        _entity->add_ref();
        output_entitys.push_back(_entity);
        return true;
    }
    bool add_output_resource(const std::string & key, const std::string & value) {
        output_resource.add_resource(key, value);
        return true;
    }
    const xinput_ptr_t &    get_input() const {
        xassert(m_input != nullptr);
        return m_input;
    }
    const xoutput_ptr_t &   get_output() const {
        xassert(m_output != nullptr);
        return m_output;
    }

 private:
    std::vector<base::xventity_t*>  input_entitys;
    std::vector<base::xventity_t*>  output_entitys;
    xblock_resource_t               input_resource;
    xblock_resource_t               output_resource;
    xinput_ptr_t                    m_input{nullptr};
    xoutput_ptr_t                   m_output{nullptr};
};



NS_END2
