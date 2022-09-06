// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
 
#include "xbase/xobject.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xvevent.h"
#include "xvbindex.h"

NS_BEG2(top, mbus)
using xevent_ptr_t = xobject_ptr_t<xevent_t>;
NS_END2

namespace top
{
    namespace base
    {
        class xveventbus_t : public xobject_t
        {
        protected:
            xveventbus_t();
            virtual ~xveventbus_t();
        private:
            xveventbus_t(xveventbus_t &&);
            xveventbus_t(const xveventbus_t &);
            xveventbus_t & operator = (const xveventbus_t &);
            
        public:
            //caller need to cast (void*) to related ptr
            virtual void*  query_interface(const int32_t _enum_xobject_type_) override;
            
        public: //api for event
            virtual bool   handle_event(const xvevent_t & ev) override; //convert then push_event
            virtual void   push_event(const mbus::xevent_ptr_t& e) = 0; //push event into mbus system
            
        public://declares clasic events
            virtual mbus::xevent_ptr_t  create_event_for_store_index_to_db(base::xvbindex_t * target_index) = 0;
            virtual mbus::xevent_ptr_t  create_event_for_revoke_index_to_db(base::xvbindex_t * target_index)= 0;
            virtual mbus::xevent_ptr_t  create_event_for_store_block_to_db(base::xvblock_t * target_block) = 0;
            virtual mbus::xevent_ptr_t  create_event_for_store_committed_block(base::xvbindex_t * target_index) = 0;
        };
    
    }//end of namespace of base
}//end of namespace top
