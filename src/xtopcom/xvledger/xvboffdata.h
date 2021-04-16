// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xns_macro.h"

namespace top
{
    namespace base
    {
        // the block offdata includes some propertys
        class xvboffdata_t : public xdataunit_t
        {
            friend class xvblock_t;
        public:
            static  const std::string   name(){ return std::string("xvboffdata");}
            virtual std::string         get_obj_name() const override {return name();}
            enum{enum_obj_type = enum_xobject_type_voffdata};//allow xbase create object from xdataunit_t::read_from()
 
        public:
            xvboffdata_t();
        protected:
            virtual ~xvboffdata_t();
        private://not implement those private construction
            xvboffdata_t(xvboffdata_t &&);
            xvboffdata_t(const xvboffdata_t &);
            xvboffdata_t & operator = (const xvboffdata_t & other);

        public:
            //caller need to cast (void*) to
            virtual void*           query_interface(const int32_t _enum_xobject_type_) override;
            
        public:
            std::string             build_root_hash(enum_xhash_type hashtype);  // build root of the whold offdata
            xobject_ptr_t<xdataunit_t>  query_offdata(const std::string & name) const;

        protected:
            virtual int32_t         do_write(base::xstream_t & stream) override;
            virtual int32_t         do_read(base::xstream_t & stream) override;
            
        private:// local member
            uint64_t        m_height{0};
            std::string     m_block_hash;
        private:
            std::map<std::string, xobject_ptr_t<xdataunit_t>>  m_propertys;  // 块下数据由多种具体数据组成
        };
    } //end of namespace base
} //end of namespace top
