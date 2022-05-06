// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xbase/xdata.h"

namespace top
{
    namespace base
    {
        struct xcheckpoint_data_t {
            uint64_t height;
            std::string hash;
        };

        class xvcpstore_t : public xobject_t
        {
        public:
            static  const std::string   name(){return "xvcpstore";} //"xvcpstore"
            virtual std::string         get_obj_name() const override {return name();}

        protected:
            xvcpstore_t();
            virtual ~xvcpstore_t();
        private:
            xvcpstore_t(xvcpstore_t &&);
            xvcpstore_t(const xvcpstore_t &);
            xvcpstore_t & operator = (const xvcpstore_t &);

        public://key-value manage
            virtual xcheckpoint_data_t get_latest_checkpoint(const std::string & address, std::error_code & ec) = 0;

        protected:
           using xobject_t::add_ref;
           using xobject_t::release_ref;
        };

    }//end of namespace of base
}//end of namespace top
