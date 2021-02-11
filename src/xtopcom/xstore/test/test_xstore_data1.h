#pragma once

#include <string>
#include <msgpack.hpp>

#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"


using namespace top::base;

class xstore_data1 : public xdataobj_t
{
public:
    enum{enum_obj_type = enum_xdata_type_min + 100};    
public:    
    xstore_data1()
    :xdataobj_t(static_cast<enum_xdata_type>(enum_obj_type))
    {
        //cout << "xstore_data1 construct "<< endl;
    }
public: 
    static xobject_t* create_object(int type)
    {
        (void)type;
        return new xstore_data1;
    }
    //caller respond to cast (void*) to related  interface ptr
    virtual void* query_interface(const int32_t _enum_xobject_type_) override
    {
        if(enum_obj_type == _enum_xobject_type_)
            return this;
        return xdataobj_t::query_interface(_enum_xobject_type_);
    }    
    virtual std::string pack() const
    {
        std::stringstream buffer;
        msgpack::pack(buffer, *this);
        return buffer.str();
    }
    virtual void unpack(const std::string& str)
    {
        msgpack::object_handle oh = msgpack::unpack(str.data(), str.size());
        msgpack::object obj = oh.get();
        obj.convert(*this);
    }       
private: //not safe for multiple threads
    virtual int32_t    do_write(xstream_t & stream) override        //serialize whole object to binary
    {
        const int32_t begin_pos = stream.size();     
        stream << m_account;
        stream << m_data_1;
        stream << m_data_2;
        stream << m_data_3;
        stream << m_data_4;
        
        const int32_t end_pos = stream.size();   
        assert(begin_pos != end_pos);
        return (end_pos - begin_pos);
    }
    virtual int32_t    do_read(xstream_t & stream) override //serialize from binary and regeneate content of xdataobj_t
    {
        const int32_t begin_pos = stream.size();        
        stream >> m_account;
        stream >> m_data_1;
        stream >> m_data_2;
        stream >> m_data_3;
        stream >> m_data_4;
        
        const int32_t end_pos = stream.size();
        assert(begin_pos != end_pos);        
        return (begin_pos - end_pos);
    }     
public:
    std::string m_account;
    uint32_t m_data_1;
    uint32_t m_data_2;
    uint16_t m_data_3;
    std::string m_data_4;
    MSGPACK_DEFINE(m_account, m_data_1, m_data_2, m_data_3, m_data_4);
};

xobject_t* create_xstore_data1(int type);
void test_xstore_data1_set(xstore_data1 & d1);
void test_xstore_data1_is_valid(xstore_data1 & d1);
void test_xstore_data1_set_2(xstore_data1 & d1);
void test_xstore_data1_is_valid_2(xstore_data1 & d1);