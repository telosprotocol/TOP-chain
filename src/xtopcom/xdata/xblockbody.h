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



using xentity_ptr_t = xobject_ptr_t<base::xventity_t>;

// unify all blockbody para
class xblockbody_para_t {
 public:
    xblockbody_para_t() {
        input_resource = make_object_ptr<base::xstrmap_t>();
        output_resource = make_object_ptr<base::xstrmap_t>();
    }
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
    bool add_input_entity(const xentity_ptr_t & entity) {
        base::xventity_t* _entity = entity.get();
        _entity->add_ref();
        input_entitys.push_back(_entity);
        return true;
    }
    bool add_input_resource(const std::string & key, const std::string & value) {
        input_resource->set(key, value);
        return true;
    }
    bool add_output_entity(const xentity_ptr_t & entity) {
        base::xventity_t* _entity = entity.get();
        _entity->add_ref();
        output_entitys.push_back(_entity);
        return true;
    }
    bool add_output_resource(const std::string & key, const std::string & value) {
        output_resource->set(key, value);
        return true;
    }
    const std::vector<base::xventity_t*> &  get_input_entitys() const {return input_entitys;}
    const std::vector<base::xventity_t*> &  get_output_entitys() const {return output_entitys;}
    base::xstrmap_t*                        get_input_resource() const {return input_resource.get();}
    base::xstrmap_t*                        get_output_resource() const {return output_resource.get();}

 private:
    std::vector<base::xventity_t*>  input_entitys;
    std::vector<base::xventity_t*>  output_entitys;
    xobject_ptr_t<base::xstrmap_t>  input_resource{nullptr};
    xobject_ptr_t<base::xstrmap_t>  output_resource{nullptr};
};



NS_END2
