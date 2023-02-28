// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xbase/xdata.h"
#include "xvblock.h"

#include "xbasic/xspan.h"

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
            virtual std::string get_value(const std::string & key) const = 0;
            virtual bool              set_value(const std::string & key, const std::string& value) = 0;
            virtual bool              set_values(const std::map<std::string, std::string> & objs) = 0;
            virtual bool              delete_value(const std::string & key) = 0;
            //batch deleted keys
            virtual bool              delete_values(const std::vector<std::string> & to_deleted_keys) = 0;
            virtual bool delete_values(std::vector<xspan_t<char const>> const & to_deleted_keys) = 0;

        public://old API, here just for compatible
            virtual bool             set_vblock(const std::string & store_path,xvblock_t* block) = 0;
            virtual bool             set_vblock_header(const std::string & store_path,xvblock_t* block) = 0;

            virtual base::xvblock_t* get_vblock(const std::string & store_path,const std::string & account, const uint64_t height) const = 0;
            virtual base::xvblock_t* get_vblock_header(const std::string & store_path,const std::string & account,const uint64_t height) const = 0;
            virtual bool             get_vblock_input(const std::string & store_path,xvblock_t* for_block)  const = 0;//just load input
            virtual bool             get_vblock_output(const std::string & store_path,xvblock_t* for_block) const = 0;//just load output

        public://new api for range ops
            //prefix must start from first char of key
            virtual bool             read_range(const std::string& prefix, std::vector<std::string>& values) = 0;
            
            //note:begin_key and end_key must has same style(first char of key)
            // Removes the database entries in the range ["begin_key", "end_key"), i.e.,
            // including "begin_key" and excluding "end_key". Returns OK on success, and
            // a non-OK status on error. It is not an error if the database does not
            // contain any existing data in the range ["begin_key", "end_key").
            //
            // If "end_key" comes before "start_key" according to the user's comparator,
            // a `Status::InvalidArgument` is returned.
            virtual bool             delete_range(const std::string & begin_key,const std::string & end_key) = 0;
            
            //key must be readonly(never update after PUT),otherwise the behavior is undefined
            virtual bool             single_delete(const std::string & target_key) = 0;

            //compact whole DB if both begin_key and end_key are empty
            //note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
            virtual bool             compact_range(const std::string & begin_key,const std::string & end_key) = 0;
            virtual void             GetDBMemStatus() const = 0;
        protected:
//            using xobject_t::add_ref;
//            using xobject_t::release_ref;
        };

    }//end of namespace of base
}//end of namespace top
