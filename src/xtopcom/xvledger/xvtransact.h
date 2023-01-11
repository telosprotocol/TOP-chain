// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xvtxindex.h"
#include "xvaction.h"

namespace top
{
    namespace base
    {
        //virtual transaction object
        class xvtransact_t : public xdataunit_t
        {
        protected:
            xvtransact_t();
            virtual ~xvtransact_t();
        private:
            xvtransact_t(xvtransact_t &&);
            xvtransact_t(const xvtransact_t &);
            xvtransact_t & operator = (xvtransact_t &&);
            xvtransact_t & operator = (const xvtransact_t &);

        public:
            virtual bool               is_valid()   const = 0; //verify signature,content,format etc
            virtual const std::string  get_hash()   const = 0; //get tx hash
            virtual const xvaction_t&  get_action() const = 0; //build and return
        };

    }//end of namespace of base

}//end of namespace top
