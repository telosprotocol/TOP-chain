// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xbase/xdata.h"
#include "xvblock.h"

namespace top
{
    namespace base
    {
        class xvdbstore_t : public xobject_t
        {
            friend class xvchain_t;
        public:
            static  const std::string   name(){return "xvdbstore";} //"xvblockstore"
            virtual std::string         get_obj_name() const override {return name();}

        protected:
            xvdbstore_t();
            virtual ~xvdbstore_t();
        private:
            xvdbstore_t(xvdbstore_t &&);
            xvdbstore_t(const xvdbstore_t &);
            xvdbstore_t & operator = (const xvdbstore_t &);
        public:
            //caller need to cast (void*) to related ptr
            virtual void*             query_interface(const int32_t _enum_xobject_type_) override;
            //a full path to load vblock could be  get_store_path()/create_object_path()/xvblock_t::name()
            virtual std::string       get_store_path() const = 0;//each store may has own space at DB/disk

        public://key-value manage
            virtual const std::string get_value(const std::string & key) const = 0;
            virtual bool              set_value(const std::string & key, const std::string& value) = 0;
            virtual bool              delete_value(const std::string & key) = 0;
            virtual bool              find_values(const std::string & key,std::vector<std::string> & values) = 0;//support wild search
            //delete all keys are follow by wild_key_path,usally for delete keys at same height of same account
            virtual bool              delete_mutiple_values(const std::string & wild_key_path) {return false;}

        public://old API, here just for compatible
            virtual bool             set_vblock(const std::string & store_path,xvblock_t* block) = 0;
            virtual bool             set_vblock_header(const std::string & store_path,xvblock_t* block) = 0;

            virtual base::xvblock_t* get_vblock(const std::string & store_path,const std::string & account, const uint64_t height) const = 0;
            virtual base::xvblock_t* get_vblock_header(const std::string & store_path,const std::string & account,const uint64_t height) const = 0;
            virtual bool             get_vblock_input(const std::string & store_path,xvblock_t* for_block)  const = 0;//just load input
            virtual bool             get_vblock_output(const std::string & store_path,xvblock_t* for_block) const = 0;//just load output

        public://execute_block will move to statestore soon
            virtual bool             execute_block(base::xvblock_t* block) = 0;
            virtual bool             get_vblock_offdata(const std::string & store_path,xvblock_t* for_block) const = 0;//just load offdata
            virtual bool             set_vblock_offdata(const std::string & store_path,xvblock_t* for_block) = 0;//just save offdata

        protected:
//            using xobject_t::add_ref;
//            using xobject_t::release_ref;
        };

    }//end of namespace of base
}//end of namespace top
